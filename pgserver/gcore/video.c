/* $Id: video.c,v 1.51 2001/12/29 21:49:30 micahjd Exp $
 *
 * video.c - handles loading/switching video drivers, provides
 *           default implementations for video functions
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/input.h>
#include <pgserver/divtree.h>
#include <pgserver/widget.h>

/******************************************** Utils */

/* Vidlib vars */
struct vidlib *vid, *vidwrap;
struct vidlib vidlib_static;
struct vidlib vidwrap_static;
struct sprite *spritelist;
s16 upd_x;
s16 upd_y;
s16 upd_w;
s16 upd_h;

/* Statically allocated memory that the driver can use for vid->display */
struct stdbitmap static_display;

/* Trig table used in hwr_gradient (sin*256 for theta from 0 to 90) */
unsigned char trigtab[] = {
  0x00,0x04,0x08,0x0D,0x11,0x16,0x1A,0x1F,0x23,0x28,
  0x2C,0x30,0x35,0x39,0x3D,0x42,0x46,0x4A,0x4F,0x53,
  0x57,0x5B,0x5F,0x64,0x68,0x6C,0x70,0x74,0x78,0x7C,
  0x80,0x83,0x87,0x8B,0x8F,0x92,0x96,0x9A,0x9D,0xA1,
  0xA4,0xA7,0xAB,0xAE,0xB1,0xB5,0xB8,0xBB,0xBE,0xC1,
  0xC4,0xC6,0xC9,0xCC,0xCF,0xD1,0xD4,0xD6,0xD9,0xDB,
  0xDD,0xDF,0xE2,0xE4,0xE6,0xE8,0xE9,0xEB,0xED,0xEE,
  0xF0,0xF2,0xF3,0xF4,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,
  0xFC,0xFC,0xFD,0xFE,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF
};

/* Cursor blanking ctrl */
int cursor_blanking_enabled = 1;

/* Sprite helper functions */
g_error new_sprite(struct sprite **ps,s16 w,s16 h) {
  g_error e;
  
  e = g_malloc((void**)ps,sizeof(struct sprite));
  errorcheck;
  memset(*ps,0,sizeof(struct sprite));
  (*ps)->ox = -1;
  (*ps)->w = w;
  (*ps)->h = h;
  VID(bitmap_new) (&(*ps)->backbuffer,w,h);
  (*ps)->next = spritelist;
  (*ps)->visible = 1;
  spritelist = *ps;

  return success;
}

void free_sprite(struct sprite *s) {
  struct sprite *n;

  VID(sprite_hide) (s);
   
  /* Remove from the sprite list */
  if (s==spritelist)
    spritelist = s->next;
  else {
    n = spritelist;
    while (n->next) {
      if (n->next == s) {
	n->next = s->next;
	break;
      }
      n = n->next;
    }
  }

  VID(bitmap_free) (s->backbuffer);
  g_free(s);
}

/******************************************** Vidlib admin functions */

/* Let the driver register itself, and initialize things */
g_error load_vidlib(g_error (*regfunc)(struct vidlib *v),
		    s16 xres,s16 yres,s16 bpp,u32 flags) {
  g_error e;

  /* Unload */
  if (vid) 
    VID(close) ();

  /* Clear it */
  vid = &vidlib_static;
  vidwrap = vid;         /* No transforms */
  memset(vid,0,sizeof(struct vidlib));
  vid->close = &emulate_dos;
  vid->update = &def_update;
  vid->setmode = &def_setmode;
   
  /* Device registration */
  e = (*regfunc)(vid);
  if (iserror(e)) {
    vid = NULL;
    return e;
  }

  inlib_main = NULL;

  /* By default use a static vid->display */
  vid->display = (hwrbitmap) &static_display; 

  /* Load new driver */
  e = VID(init)();
  if (iserror(e)) {
    vid = NULL;
    return e;
  }

  /* Set the initial mode */
  return video_setmode(xres,yres,bpp,PG_FM_SET,flags);
}

/* Set the video mode using the current driver. This is the implementation
 * of the setmode client request */
g_error video_setmode(u16 xres,u16 yres,u16 bpp,u16 flagmode,u32 flags) {
   g_error e;
   struct divtree *tree;
   struct sprite *spr;
   u8 converting_mode,oldbpp;

   /* Must be done first */
   if (vidwrap->exitmode) {
      e = VID(exitmode)();
      errorcheck;
   }
   
   /* If the new bpp is different, use modeconvert/modeunconvert */
   oldbpp = vid->bpp;
   converting_mode = (bpp != vid->bpp);
   if (converting_mode) {
      e = bitmap_iterate(vid->bitmap_modeunconvert);
      errorcheck;
      e = handle_iterate(PG_TYPE_PALETTE,&array_hwrtopg);
      errorcheck;
   }
      
   /* Default values, combine flags */
   if (!xres) xres = vid->xres;
   if (!yres) yres = vid->yres;
   if (!bpp)  bpp  = vid->bpp;
   switch (flagmode) {
    case PG_FM_ON:
      flags |= vid->flags;
      break;      
    case PG_FM_OFF:
      flags = vid->flags & (~flags);
      break;
    case PG_FM_TOGGLE:
      flags ^= vid->flags;
      break;
   }
   vid->flags = flags;

   /* Might want to tell the driver! */
   e = (*vid->setmode) (xres,yres,bpp,flags);
   errorcheck;

   /* By default logical coordinates are also physical */
   vid->lxres = vid->xres;
   vid->lyres = vid->yres;
   
   /* Synchronize vid->display info automatically if we're using stdbitmap */
   if (vid->display && vid->bitmap_getsize==def_bitmap_getsize) {
     if (((struct stdbitmap *)vid->display)->rend)
       g_free(((struct stdbitmap *)vid->display)->rend);
     ((struct stdbitmap *)vid->display)->freebits = 0;
     ((struct stdbitmap *)vid->display)->w = vid->xres;
     ((struct stdbitmap *)vid->display)->h = vid->yres;
   }
      
   /* Reset wrapper library (before using VID macro) */
   vidwrap_static = vidlib_static;
   vidwrap = &vidwrap_static;
   
   {
     /* Generate text colors table */

     u32 *tc;
     int i;

     /* Allocate space for textcolors if we haven't already */
     if (!default_textcolors) {
       u32 *ptr;
       e = g_malloc((void**)&ptr,sizeof(u32)*17);
       errorcheck;
       ptr[0] = 16;
       e = mkhandle(&default_textcolors,PG_TYPE_PALETTE,-1,(void*)ptr);
       errorcheck;
     }

     e = rdhandle((void **) &tc, PG_TYPE_PALETTE, -1, default_textcolors);
     errorcheck;
      
     /* VGA 16-color palette */
     for (i=0;i<tc[0];i++)
       tc[i+1] = ( (i & 0x08) ?
		   (((i & 0x04) ? 0xFF0000 : 0) |
		    ((i & 0x02) ? 0x00FF00 : 0) |
		    ((i & 0x01) ? 0x0000FF : 0)) :
		   (((i & 0x04) ? 0x800000 : 0) |
		    ((i & 0x02) ? 0x008000 : 0) |
		    ((i & 0x01) ? 0x000080 : 0)) );
     
     /* If we won't be doing it anyway later, go ahead and
      * convert these to hwrcolors
      */
     if (!converting_mode) {
       e = array_pgtohwr(&tc);
       errorcheck;
     }
   }
     
   /* Add wrapper libraries if necessary. At compile-time, support adding
    * 90, 180, or 270 degrees as a hardware rotation
    */

#ifdef CONFIG_ROTATIONBASE_90                  /***** 90 degree default */
#ifdef CONFIG_ROTATE180
   if (vid->flags & PG_VID_ROTATE90) {
      vidwrap_rotate180(vidwrap);
      vid->lxres = vid->xres;
      vid->lyres = vid->yres;
   }
#endif   
#ifdef CONFIG_ROTATE270
   if (vid->flags & PG_VID_ROTATE180) {
      vidwrap_rotate270(vidwrap);
      vid->lxres = vid->yres;
      vid->lyres = vid->xres;
   }
#endif   
#ifdef CONFIG_ROTATE
   if (!(vid->flags & PG_VID_ROTATEMASK)) {
      vidwrap_rotate90(vidwrap);
      vid->lxres = vid->yres;
      vid->lyres = vid->xres;
   }
#endif
#else
#ifdef CONFIG_ROTATIONBASE_180                  /***** 180 degree default */
#ifdef CONFIG_ROTATE
   if (vid->flags & PG_VID_ROTATE270) {
      vidwrap_rotate90(vidwrap);
      vid->lxres = vid->yres;
      vid->lyres = vid->xres;
   }
#endif   
#ifdef CONFIG_ROTATE270
   if (vid->flags & PG_VID_ROTATE90) {
      vidwrap_rotate270(vidwrap);
      vid->lxres = vid->yres;
      vid->lyres = vid->xres;
   }
#endif   
#ifdef CONFIG_ROTATE180
   if (!(vid->flags & PG_VID_ROTATEMASK)) {
      vidwrap_rotate180(vidwrap);
      vid->lxres = vid->xres;
      vid->lyres = vid->yres;
   }
#endif
#else
#ifdef CONFIG_ROTATIONBASE_270                  /***** 270 degree default */
#ifdef CONFIG_ROTATE
   if (vid->flags & PG_VID_ROTATE180) {
      vidwrap_rotate90(vidwrap);
      vid->lxres = vid->yres;
      vid->lyres = vid->xres;
   }
#endif   
#ifdef CONFIG_ROTATE180
   if (vid->flags & PG_VID_ROTATE270) {
      vidwrap_rotate180(vidwrap);
      vid->lxres = vid->xres;
      vid->lyres = vid->yres;
   }
#endif   
#ifdef CONFIG_ROTATE270
   if (!(vid->flags & PG_VID_ROTATEMASK)) {
      vidwrap_rotate270(vidwrap);
      vid->lxres = vid->yres;
      vid->lyres = vid->xres;
   }
#endif
#else                                           /***** 0 degree default */
#ifdef CONFIG_ROTATE
   if (vid->flags & PG_VID_ROTATE90) {
      vidwrap_rotate90(vidwrap);
      vid->lxres = vid->yres;
      vid->lyres = vid->xres;
   }
#endif   
#ifdef CONFIG_ROTATE180
   if (vid->flags & PG_VID_ROTATE180) {
      vidwrap_rotate180(vidwrap);
      vid->lxres = vid->xres;
      vid->lyres = vid->yres;
   }
#endif   
#ifdef CONFIG_ROTATE270
   if (vid->flags & PG_VID_ROTATE270) {
      vidwrap_rotate270(vidwrap);
      vid->lxres = vid->yres;
      vid->lyres = vid->xres;
   }
#endif   
#endif
#endif
#endif

   /* Should we ignore the base screen rotation for keyboard events? */
#ifdef CONFIG_ROTATIONBASE_NOKEYS
   vidwrap->coord_keyrotate = &def_coord_keyrotate;
#ifdef CONFIG_ROTATE
   if (vid->flags & PG_VID_ROTATE90)
     vidwrap->coord_keyrotate = &rotate90_coord_keyrotate;
#endif   
#ifdef CONFIG_ROTATE180
   if (vid->flags & PG_VID_ROTATE180)
     vidwrap->coord_keyrotate = &rotate180_coord_keyrotate;
#endif   
#ifdef CONFIG_ROTATE270
   if (vid->flags & PG_VID_ROTATE270)
     vidwrap->coord_keyrotate = &rotate270_coord_keyrotate;
#endif
#endif /* CONFIG_ROTATIONBASE_NOKEYS */

   /* Since changing video modes pretty much obliterates all onscreen
    * sprites, and the previous location might be offscreen now,
    * reset the backbuffer on all sprites */
   for (spr=spritelist;spr;spr=spr->next)
     spr->onscreen = 0;
   
   /* Resize the root divnodes of all divtrees in the dtstack */
   if (dts)   /* (if this is in early init, dtstack isn't here yet) */
     for (tree=dts->top;tree;tree=tree->next) {
	tree->head->w = vid->lxres;
	tree->head->h = vid->lyres;
	tree->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
	tree->flags |= DIVTREE_NEED_RECALC | DIVTREE_ALL_REDRAW;
	
	/* More work for us if this is a popup layer... 
	 * Need to reclip the popup so it doesn't go off the
	 * edge of the screen.
	 */
	if (tree->head->next && tree->head->next->owner &&
	    tree->head->next->owner->type == PG_WIDGET_POPUP)
	  clip_popup(tree->head->next->div);
     }

   /* Convert to the new color depth if necessary */
   if (converting_mode) {
      e = bitmap_iterate(vid->bitmap_modeconvert);
      errorcheck;
      e = handle_iterate(PG_TYPE_PALETTE,&array_pgtohwr);
      errorcheck;
   }
   
   /* Done swtiching the mode, give the driver a shot at changing
    * things around */
   e = VID(entermode)();
   errorcheck;
   
   /* Reload things when increasing color depth */
   if (oldbpp < bpp) {
    
      /* Reload any themes the server is responsible for */
      e = reload_initial_themes();
      errorcheck;
      
      /* FIXME: notify all clients that they can reload here */    
   }
  
   return success;
}

g_error (*find_videodriver(const char *name))(struct vidlib *v) {
  struct vidinfo *p = videodrivers;
  while (p->name) {
    if (!strcmp(name,p->name))
      return p->regfunc;
    p++;
  }
  return NULL;
}

void add_updarea(s16 x,s16 y,s16 w,s16 h) {

  /* Clip to logical display */
  if (x<0) {
    w -= x;
    x = 0;
  }
  if (y<0) {
    h -= y;
    y = 0;
  }
  if ((x+w)>vid->lxres)
    w = vid->lxres-x;
  if ((y+h)>vid->lyres)
    h = vid->lyres-y;

  /* Is this a bogus update rectangle? */
  if (w<=0 || h<=0)
    return;

  if (upd_w) {
    if (x < upd_x) {
      upd_w += upd_x - x;
      upd_x = x;
    }
    if (y < upd_y) {
      upd_h += upd_y - y;
      upd_y = y;
    }
    if ((w+x) > (upd_x+upd_w))
      upd_w = w+x-upd_x;
    if ((h+y) > (upd_y+upd_h))
      upd_h = h+y-upd_y;
  }
  else {
    upd_x = x;
    upd_y = y;
    upd_w = w;
    upd_h = h;
  }
}

/* Update and reset the update rectangle */
void realize_updareas(void) {
  /* This lock is an effort to fix a bug observed while running in SDL:
   * while blitting the update rectangles, another event is recieved,
   * causing this to be entered twice. Apparently X doesn't like that :)
   */
  static unsigned char lock = 0;

  if (lock) return;
  lock = 1;

   if (upd_w) {
      if (upd_x<0) {
	 upd_w += upd_x;
	 upd_x = 0;
      }
      if (upd_y<0) {
	 upd_h += upd_y;
	 upd_y = 0;
      }
      if ((upd_x+upd_w)>vid->lxres)
	upd_w = vid->lxres-upd_x;
      if ((upd_y+upd_h)>vid->lyres)
	upd_h = vid->lyres-upd_y;
#ifdef DEBUG_VIDEO
      /* Show update rectangles */
      /*
	VID(rect) (vid->display,
		 upd_x,upd_y,upd_w,upd_h,(*vid->color_pgtohwr)(0xFF0000),
		 PG_LGOP_STIPPLE);
      */
#endif
      VID(update) (upd_x,upd_y,upd_w,upd_h);
      upd_x = upd_y = upd_w = upd_h = 0;
   } 
   
   lock = 0;
}

g_error bitmap_iterate(g_error (*iterator)(hwrbitmap *pbit)) {   
   
   /* Rotate all bitmaps with handles, the default cursor,
    * and all sprite backbuffers */
   struct sprite *spr;
   g_error e;
   
   e = handle_iterate(PG_TYPE_BITMAP,(g_error(*)(void**)) iterator);
   errorcheck;
   
   if (defaultcursor_bitmap) {           /* If we are rotating by default
					  * this might be during init
					  * before cursor is alloc'd */
      e = (*iterator)(&defaultcursor_bitmap);
      errorcheck;
      e = (*iterator)(&defaultcursor_bitmask);
      errorcheck;
   }
   for (spr=spritelist;spr;spr=spr->next) {
      e = (*iterator)(&spr->backbuffer);
      errorcheck;
   }
      
   return success;
}

/* Iterators for converting between pgcolor and hwrcolor arrays */
g_error array_pgtohwr(u32 **array) {
  u32 *p  = (*array)+1;
  u32 len = **array;
  for (;len;len--,p++)
    *p = VID(color_pgtohwr)(*p);
  return success;
}
g_error array_hwrtopg(u32 **array) {
  u32 *p  = (*array)+1;
  u32 len = **array;
  for (;len;len--,p++)
    *p = VID(color_hwrtopg)(*p);
  return success;
}

/* Convert a handle from array to palette */
g_error array_palettize(handle h, int owner) {
  g_error e;
  u32 *arr;

  /* If it is a palette already, we're done */
  if (iserror(rdhandle((void**) &arr, PG_TYPE_ARRAY, owner, h)))
    return rdhandle((void**) &arr, PG_TYPE_PALETTE, owner, h);
  
  /* Convert data */
  e = array_pgtohwr(&arr);
  errorcheck;

  /* Convert type */
  return rehandle(h,(void*) arr, PG_TYPE_PALETTE);
}

/* Send the message to all loaded drivers */
void drivermessage(u32 message, u32 param, u32 *ret) {
  struct inlib *p;
  
  if (!ret)
    ret = alloca(sizeof(u32));
  *ret = 0;

  /* Current video driver */
  if (vid->message)
    (*vid->message)(message,param,ret);

  /* All input drivers loaded */
  p = inlib_list;
  while (p) {
    if (p->message)
      (*p->message)(message,param,ret);
    p = p->next;
  }

  /* Some stuff we can handle here */
  switch (message) {

  case PGDM_CURSORVISIBLE:
    cursor->visible = (param != 0);
    if (cursor->visible)
      VID(sprite_show)(cursor);
    else
      VID(sprite_hide)(cursor);
    realize_updareas();
    break;

  case PGDM_CURSORBLKEN:
    cursor_blanking_enabled = (param != 0);
    break;
  }
}

/* The End */

