/* $Id$
 *
 * video_drivers.c - handles loading/switching video drivers and modes
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <stdlib.h>
#include <string.h>

#include <pgserver/common.h>
#include <pgserver/configfile.h>
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

g_error (*external_regfunc)(struct vidlib *v) = NULL;
s16 external_xres;
s16 external_yres;
s16 external_bpp;
s16 external_flags;

/* Trig table (sin*256 for theta from 0 to 90) 
 * This is used in the default implementation of gradient,
 * and may be used by other VBL functions or drivers.
 */
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

/* Sprite helper functions */
g_error new_sprite(struct sprite **ps,struct divtree *dt,s16 w,s16 h) {
  g_error e;
  
  e = g_malloc((void**)ps,sizeof(struct sprite));
  errorcheck;
  memset(*ps,0,sizeof(struct sprite));
  (*ps)->ox = -1;
  (*ps)->w = w;
  (*ps)->h = h;
  VID(bitmap_new) (&(*ps)->backbuffer,w,h,vid->bpp);
  (*ps)->next = spritelist;
  (*ps)->visible = 1;
  (*ps)->lgop = PG_LGOP_NONE;
  (*ps)->dt = hlookup(dt,NULL);
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

g_error video_init(void) {
  g_error e;
  int vidw,vidh,vidd,vidf;
  const char *str;
  g_error (*viddriver)(struct vidlib *v);

#ifdef CONFIG_PAL8_CUSTOM
  /* Load a custom palette if necessary */
  e = load_custom_palette(get_param_str("pgserver","palette",NULL));
  errorcheck;
#endif
  
  /* Process video driver config options */
  vidw = get_param_int("pgserver","width",0);
  vidh = get_param_int("pgserver","height",0);
  vidd = get_param_int("pgserver","depth",0);
  vidf = get_param_int("pgserver","vidflags",0);
  sscanf(get_param_str("pgserver","mode",""),"%dx%dx%d",&vidw,&vidh,&vidd);
  
  /* Add rotation flags */
  switch (get_param_int("pgserver","rotate",0)) {
  case 90:
    vidf |= PG_VID_ROTATE90;
    break;
  case 180:
    vidf |= PG_VID_ROTATE180;
    break;
  case 270:
    vidf |= PG_VID_ROTATE270;
    break;
  }
  
  /* Force an external video driver? */
  if (external_regfunc)
  {
    if (iserror(load_vidlib(external_regfunc,
        external_xres, external_yres,
        external_bpp, external_flags)))
      return mkerror(PG_ERRT_IO,78); /* @@@ Prolly some other error*/
  } else
  /* Force a specific video driver? */
  if ((str = get_param_str("pgserver","video",NULL))) {
    if (!(viddriver = find_videodriver(str)))
      return mkerror(PG_ERRT_BADPARAM,77);;
    e = load_vidlib(viddriver,vidw,vidh,vidd,vidf);
    errorcheck;
  }
  else {
    /* Try to detect a driver (see driverinfo.c) */
    struct vidinfo *p = videodrivers;
    
    while (p->name) {
      if (!iserror(load_vidlib(p->regfunc,vidw,vidh,vidd,vidf)))
	/* Yay, found one that works */
	break;
      p++;
    }
    if (!p->name) {
      /* Oh well... */
      return mkerror(PG_ERRT_IO,78);
    }
  }

  return success;
}

void video_shutdown(void) {
  if (vid) {
    if (vid->display && vid->bitmap_getsize==def_bitmap_getsize && 
	((struct stdbitmap *)vid->display)->rend)
      g_free(((struct stdbitmap *)vid->display)->rend);
    VID(close) ();
  }
}

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

   /* If the new bpp is different, use modeconvert/modeunconvert */
   oldbpp = vid->bpp;
   converting_mode = (bpp != vid->bpp);
   if (converting_mode) {
      e = bitmap_iterate((handle_iterator)vid->bitmap_modeunconvert, NULL);
      errorcheck;
      e = handle_iterate(PG_TYPE_PALETTE,(handle_iterator)array_hwrtopg, NULL);
      errorcheck;
   }
      
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
     ((struct stdbitmap *)vid->display)->bpp = vid->bpp;
   }
      
   /* Reset wrapper library (before using VID macro) */
   vidwrap_static = vidlib_static;
   vidwrap = &vidwrap_static;
   
   {
     /* Generate text colors table */

     u32 *tc;
     int i;

     /* Default VGA 16-color palette, copied from the Linux console */
     static const u32 default_palette[] = {
       0x000000, 0xaa0000, 0x00aa00, 0xaaaa00, 0x0000aa, 0xaa00aa, 0x00aaaa, 0xaaaaaa,
       0x555555, 0xff5555, 0x55ff55, 0xffff55, 0x5555ff, 0xff55ff, 0x55ffff, 0xffffff,
     };

     /* Allocate space for textcolors if we haven't already */
     if (!res[PGRES_DEFAULT_TEXTCOLORS]) {
       u32 *ptr;
       e = g_malloc((void**)&ptr,sizeof(u32)*17);
       errorcheck;
       ptr[0] = 16;
       e = mkhandle(&res[PGRES_DEFAULT_TEXTCOLORS],PG_TYPE_PALETTE,-1,(void*)ptr);
       errorcheck;
     }

     e = rdhandle((void **) &tc, PG_TYPE_PALETTE, -1, res[PGRES_DEFAULT_TEXTCOLORS]);
     errorcheck;
      
     /* Transcribe our static palette */
     memcpy(tc+1, default_palette, sizeof(default_palette));
     
     /* If we won't be doing it anyway later, go ahead and convert these to hwrcolors */
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

   /* Ignore the base screen rotation for pointing events? */
#ifdef CONFIG_ROTATIONBASE_NOPOINTING
   vidwrap->coord_physicalize = &def_coord_logicalize;
   vidwrap->coord_logicalize = &def_coord_logicalize;
#ifdef CONFIG_ROTATE
   if (vid->flags & PG_VID_ROTATE90) {
     vidwrap->coord_physicalize = &rotate90_coord_physicalize;
     vidwrap->coord_logicalize = &rotate90_coord_logicalize;
   }
#endif   
#ifdef CONFIG_ROTATE180
   if (vid->flags & PG_VID_ROTATE180) {
     vidwrap->coord_physicalize = &rotate180_coord_logicalize;
     vidwrap->coord_logicalize = &rotate180_coord_logicalize;
   }
#endif   
#ifdef CONFIG_ROTATE270
   if (vid->flags & PG_VID_ROTATE270) {
     vidwrap->coord_physicalize = &rotate270_coord_physicalize;
     vidwrap->coord_logicalize = &rotate270_coord_logicalize;
   }
#endif
#endif /* CONFIG_ROTATIONBASE_NOPOINTING */

   /* Since changing video modes pretty much obliterates all onscreen
    * sprites, and the previous location might be offscreen now,
    * reset the backbuffer on all sprites */
   for (spr=spritelist;spr;spr=spr->next)
     spr->onscreen = 0;
   
   /* Resize the root divnodes of all divtrees in the dtstack */
   if (dts)   /* (if this is in early init, dtstack isn't here yet) */
     for (tree=dts->top;tree;tree=tree->next) {
	tree->head->r.w = vid->lxres;
	tree->head->r.h = vid->lyres;
	tree->head->calc.w = vid->lxres;
	tree->head->calc.h = vid->lyres;
	tree->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_FORCE_CHILD_RECALC | DIVNODE_NEED_REBUILD;
	tree->flags |= DIVTREE_NEED_RECALC | DIVTREE_ALL_REDRAW | DIVTREE_CLIP_POPUP;
     }

   /* Convert to the new color depth if necessary */
   if (converting_mode) {
      e = bitmap_iterate((handle_iterator)vid->bitmap_modeconvert, NULL);
      errorcheck;
      e = handle_iterate(PG_TYPE_PALETTE,(handle_iterator)array_pgtohwr, NULL);
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

g_error (*find_videodriver(const u8 *name))(struct vidlib *v) {
  struct vidinfo *p = videodrivers;
  while (p->name) {
    if (!strcmp(name,p->name))
      return p->regfunc;
    p++;
  }
  return NULL;
}

/* Rotate an existing bitmap by the given angle, reallocating it.
 * (angle is not modified, it is a pointer so this can be used with bitmap_iterate)
 */
g_error bitmap_rotate(hwrbitmap *pbit, s16 angle) {
  hwrbitmap old,new;
  g_error e;
  s16 old_w,old_h;
  s16 new_w,new_h;
  s16 new_x,new_y;
  struct pgquad clip;

  old = *pbit;  
  e = vid->bitmap_getsize(old,&old_w,&old_h);
  errorcheck;

  switch (angle) {
  case 0:
    return success;

  case 90:
    new_w = old_h;
    new_h = old_w;
    new_x = 0;
    new_y = new_h-1;
    break;

  case 180:
    new_w = old_w;
    new_h = old_h;
    new_x = new_w-1;
    new_y = new_h-1;
    break;

  case 270:
    new_w = old_h;
    new_h = old_w;
    new_x = new_w-1;
    new_y = 0;
    break;
  }

  clip.x1 = 0;
  clip.y1 = 0;
  clip.x2 = new_w-1;
  clip.y2 = new_h-1;

  e = vid->bitmap_new(&new,new_w,new_h,old->bpp);
  errorcheck;

  vid->rotateblit(new,new_x,new_y,old,0,0,old_w,old_h,
		  &clip,angle,PG_LGOP_NONE);
  *pbit = new;

  vid->bitmap_free(old);
  return success;
}


/* Run the given function on _all_ bitmaps */
g_error bitmap_iterate(handle_iterator iterator, void *extra) {
  struct sprite *spr;
  g_error e;
  
  /* All bitmaps with handles */
  e = handle_iterate(PG_TYPE_BITMAP,iterator, extra);
  errorcheck;
  
  /* All sprite backbuffers */
  for (spr=spritelist;spr;spr=spr->next) {
    e = (*iterator)((const void**) &spr->backbuffer,extra);
    errorcheck;
  }
  return success;
}

/* Adapter that lets us use bitmap_rotate as an interator */
g_error bitmap_rotate_iterator(hwrbitmap *pbit, s16 *angle) {
  return bitmap_rotate(pbit,*angle);
}

/* Rotate _all_ loaded bitmaps by the given angle */
g_error bitmap_rotate_all(s16 angle) {
  return bitmap_iterate((handle_iterator) bitmap_rotate_iterator,&angle);
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
}

/* The End */

