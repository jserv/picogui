/* $Id$
 *
 * if_magic.c - Trap magic debug keys
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

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/video.h>
#include <pgserver/render.h>
#include <pgserver/font.h>
#include <pgserver/appmgr.h>
#include <pgserver/widget.h>
#include <pgserver/hotspot.h>
#include <pgserver/os.h>
#include <pgserver/pgnet.h>
#include <string.h>

void magic_button(s16 key);

/********************************************** Input filter ****/

void infilter_magic_handler(struct infilter *self, u32 trigger, union trigparam *param) {

  /* If a CTRL-ALT key was pressed, the keydown event triggers magic_button. 
   * Otherwise, pass on the key.
   */

  if ((param->kbd.mods & PGMOD_CTRL) && (param->kbd.mods & PGMOD_ALT)) {
    if (trigger==PG_TRIGGER_KEYDOWN)
      magic_button(param->kbd.key);
  }
  else
    infilter_send(self,trigger,param);
}

struct infilter infilter_magic = {
  /*accept_trigs:  */PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP,
  /*absorb_trigs:  */PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP,
       /*handler:  */&infilter_magic_handler
};

/********************************************** Debug-only code ****/
#ifdef DEBUG_KEYS

/* Get the window the cursor is in, falling back on the debug window */
static hwrbitmap magic_cursor_display(void) {
  struct cursor *c = cursor_get_default();
  struct divtree *dt;

  if (!c)
    return VID(window_debug)();
  if (iserror(rdhandle((void**)&dt, PG_TYPE_DIVTREE, -1, c->divtree)))
    return VID(window_debug)();

  return dt->display;
}

struct debug_bitmaps_data {   
  int db_x,db_y,db_h;
};
extern struct hotspot *hotspotlist;
   
g_error debug_bitmaps(const void **pobj, void *extra) {
  struct debug_bitmaps_data *data = (struct debug_bitmaps_data *) extra;
  hwrbitmap bmp = (hwrbitmap) *pobj;
  s16 w,h;
  int has_alpha, i;
  struct font_descriptor *df=NULL;
  struct pgquad screenclip;
  struct font_metrics m;
  hwrbitmap debugwin;
  s16 lxres,lyres;
  VID(bitmap_getsize)(VID(window_debug)(), &lxres, &lyres);

  screenclip.x1 = screenclip.y1 = 0;
  screenclip.x2 = lxres-1;
  screenclip.y2 = lyres-1;
  rdhandle((void**)&df,PG_TYPE_FONTDESC,-1,res[PGRES_DEFAULT_FONT]);
  df->lib->getmetrics(df,&m);

  VID(bitmap_getsize) (bmp,&w,&h);
  if (data->db_x+10+w>lxres) {
    data->db_x = 0;
    data->db_y += data->db_h+8;
    data->db_h = 0;
  }
  if (h>data->db_h)
    data->db_h = h;
   
  if (data->db_y+45+h>lyres) {
    df->lib->draw_string(df,VID(window_debug)(),xy_to_pair(10,lyres-m.charcell.h*3),
			 VID(color_pgtohwr) (0xFFFF00),
			 pgstring_tmpwrap("Too many bitmaps for this screen.\n"
					  "Change video mode and try again"),
			 &screenclip,PG_LGOP_NONE,0);

    return mkerror(PG_ERRT_INTERNAL,0);  /* Dummy error to get us to abort */
  }
   
  VID(rect) (VID(window_debug)(),data->db_x+3,data->db_y+38,w+4,h+4,
	     VID(color_pgtohwr)(0xFFFFFF),PG_LGOP_NONE);
  VID(rect) (VID(window_debug)(),data->db_x+4,data->db_y+39,w+2,h+2,
	     VID(color_pgtohwr)(0x000000),PG_LGOP_NONE);

  has_alpha = w && h && (VID(getpixel)(bmp,0,0) & PGCF_ALPHA);
     
  /* If we have an alpha channel, draw a interlacy background so we can see the alpha */
  if (has_alpha) {
    for (i=0;i<h;i++)
      VID(slab) (VID(window_debug)(), data->db_x+5, data->db_y+40+i, w,
		 VID(color_pgtohwr)(i&1 ? 0xFFFFFF : 0xCCCCCC), PG_LGOP_NONE);

    df->lib->draw_string(df,VID(window_debug)(),xy_to_pair(data->db_x+5,data->db_y+40),
			 VID(color_pgtohwr)(0x000000),
			 pgstring_tmpwrap("Alpha"), &screenclip, PG_LGOP_NONE,0);
  }

  VID(blit) (VID(window_debug)(),data->db_x+5,data->db_y+40,w,h,bmp,0,0,
	     has_alpha ? PG_LGOP_ALPHA : PG_LGOP_NONE);

  data->db_x += w+8;
  return success;
}
   
/* Utility functions to implement CTRL-ALT-N gropnode dump */
void r_grop_dump(struct divnode *div) {
  struct gropnode *n;
  int i;
   
  if (!div) return;
  if (div->grop) {
    printf("Divnode %p at (%d,%d,%d,%d): ",div,
	   div->r.x,div->r.y,div->r.w,div->r.h);
    if (div->owner)
      printf("Owned by widget %p, type %d\n",div->owner,div->owner->type);
    else
      printf("Unowned\n");
      
    for (n=div->grop;n;n=n->next) {
      printf("  Gropnode: type 0x%04X flags 0x%04X at (%d,%d,%d,%d) params: ",
	     n->type,n->flags,n->r.x,n->r.y,n->r.w,n->r.h);
      for (i=0;i<PG_GROPPARAMS(n->type);i++)
	printf("%d ",n->param[i]);
      printf("\n");
    }
  }
  r_grop_dump(div->div);
  r_grop_dump(div->next);
}
void grop_dump(void) {
  struct divtree *dt;
  printf("---------------- Begin grop tree dump\n");
  for (dt=dts->top;dt;dt=dt->next)
    r_grop_dump(dt->head);
  printf("---------------- End grop tree dump\n");
}
     
/* Utility functions to implement CTRL-ALT-D divnode dump */
void r_div_dump(struct divnode *div, const char *label, int level) {
  int i;
   
  if (!div)
    return;

  printf(label);
  for (i=0;i<level;i++)
    printf("\t");
  printf("Div %p: flags=0x%04X split=%d prefer=(%d,%d) child=(%d,%d) rect=(%d,%d,%d,%d)"
	 " calc=(%d,%d,%d,%d) divscroll=%p trans=(%d,%d)\n",
	 div,div->flags,div->split,div->preferred.w,div->preferred.h,
	 div->child.w,div->child.h,
	 div->r.x,div->r.y,div->r.w,div->r.h,
	 div->calc.x,div->calc.y,div->calc.w,div->calc.h,
	 div->divscroll,
	 div->translation.x, div->translation.y);

  r_div_dump(div->div," Div:",level+1);
  r_div_dump(div->next,"Next:",level+1);
}
void div_dump(void) {
  struct divtree *dt;
  printf("---------------- Begin div tree dump\n");
  for (dt=dts->top;dt;dt=dt->next)
    r_div_dump(dt->head,"Root:",0);
  printf("---------------- End div tree dump\n");
}

/* Trace the outlines of all divnodes onscreen */
void r_divnode_trace(struct divnode *div) {
  struct groprender r;
  struct gropnode n;
  s16 lxres,lyres;
  VID(bitmap_getsize)(magic_cursor_display(), &lxres, &lyres);

  if (!div)
    return;

  memset(&r,0,sizeof(r));
  memset(&n,0,sizeof(n));

  /* The scroll container must be visible */
  if (div->divscroll && !(div->divscroll->calc.w && div->divscroll->calc.h))
    return;

  /* Set up rendering... */
  r.output = magic_cursor_display();
  n.r = div->r;
  r.clip.x1 = 0;
  r.clip.y1 = 0;
  r.clip.x2 = lxres-1;
  r.clip.y2 = lyres-1;

  /* Green shading for leaf divnodes */
  if (!div->div && !div->next) {
    r.color = VID(color_pgtohwr)(0xA0FFA0);
    r.lgop = PG_LGOP_MULTIPLY;
    n.type = PG_GROP_RECT;
    gropnode_clip(&r,&n);
    gropnode_draw(&r,&n);
  }

  /* yellow box around all divnodes */
  r.color = VID(color_pgtohwr)(0xFFFF00);
  r.lgop = PG_LGOP_NONE;
  n.type = PG_GROP_FRAME;
  gropnode_clip(&r,&n);
  gropnode_draw(&r,&n);

  r_divnode_trace(div->div);
  r_divnode_trace(div->next);
} 

/* Graphically display a hotspot */
void hotspot_draw(struct hotspot *spot) {
  struct groprender r;
  struct gropnode n;
  int i;
  /* How to represent all the directions we can traverse */
  const static struct {
    s16 x,y;
    pgcolor c;
  } directiontab[] = {
    /* left  */ {-3, 0, 0xFFFF00},
    /* right */ { 3, 0, 0xFFFF00},
    /* up    */ { 0,-3, 0xFFFF00},
    /* down  */ { 0, 3, 0xFFFF00},
    /* next  */ { 3, 0, 0x00FF00},
    /* prev  */ {-3, 0, 0x00FF00},
  };
  s16 lxres,lyres;
  VID(bitmap_getsize)(magic_cursor_display(), &lxres, &lyres);

  /* Set up rendering...
   */
  memset(&r,0,sizeof(r));
  memset(&n,0,sizeof(n));
  r.output = magic_cursor_display();
  r.clip.x1 = 0;
  r.clip.y1 = 0;
  r.clip.x2 = lxres-1;
  r.clip.y2 = lyres-1;
  r.lgop = PG_LGOP_NONE;

  /* Draw arrows for all the directions in the graph
   */
  for (i=0;i<HOTSPOTNUM;i++)
    if (spot->graph[i]) {
      r.color = VID(color_pgtohwr)(directiontab[i].c);
      n.type = PG_GROP_LINE;
      n.r.x = spot->x + directiontab[i].x;
      n.r.y = spot->y + directiontab[i].y;
      n.r.w = spot->graph[i]->x - n.r.x;
      n.r.h = spot->graph[i]->y - n.r.y;
      gropnode_clip(&r,&n);
      gropnode_draw(&r,&n);
    }      

  /* If there's a scroll container, mark it with a frame */
  if (spot->div && spot->div->divscroll) {
    r.color = VID(color_pgtohwr)(0xFF0000);
    n.type = PG_GROP_FRAME;
    n.r = spot->div->divscroll->calc;
    gropnode_clip(&r,&n);
    gropnode_draw(&r,&n);
  }
  
  /* Every hotspot gets a red crosshairs 
   */
  r.color = VID(color_pgtohwr)(0xFF0000);
  n.type = PG_GROP_LINE;
  n.r.x = spot->x - 2;
  n.r.y = spot->y;
  n.r.w = 4;
  n.r.h = 0;
  gropnode_clip(&r,&n);
  gropnode_draw(&r,&n);
  n.type = PG_GROP_LINE;
  n.r.x = spot->x;
  n.r.y = spot->y - 2;
  n.r.w = 0;
  n.r.h = 4;
  gropnode_clip(&r,&n);
  gropnode_draw(&r,&n);
}     

#endif /* DEBUG_KEYS */

/********************************************** Debug/non-debug code ****/
   
void magic_button(s16 key) {
  switch (key) {
    
  case PGKEY_SLASH:       /* CTRL-ALT-SLASH exits */
    pgserver_mainloop_stop();
    return;
    
#ifdef DEBUG_KEYS           /* The rest only work in debug mode */
    
  case PGKEY_d:           /* CTRL-ALT-d lists all debugging commands */
    guru("Someone set up us the bomb!\n"
	 "All your divnode are belong to us!\n"
	 "\n"
	 "Debugging keys:\n"
	 "  CTRL-ALT-H: [H]andle tree dump to stdout\n"
	 "  CTRL-ALT-S: [S]tring dump to stdout\n"
	 "  CTRL-ALT-T: Div[t]ree dump to stdout\n"
	 "  CTRL-ALT-M: [M]emory use profile\n"
	 "  CTRL-ALT-B: [B]lack screen\n"
	 "  CTRL-ALT-Y: Uns[y]nchronize screen buffers\n"
	 "  CTRL-ALT-U: Bl[u]e screen\n"
	 "  CTRL-ALT-P: Bitma[p] dump to video display\n"
	 "  CTRL-ALT-O: Divn[o]de outline\n"
	 "  CTRL-ALT-A: [A]pplication dump to stdout\n"
	 "  CTRL-ALT-R: Hotspot g[r]aph\n" 
	 "  CTRL-ALT-I: Mode [I]nfo\n"
	 );
    return;
    
  case PGKEY_h:           /* CTRL-ALT-h dumps the handle tree */
    handle_dump();
    return;
    
  case PGKEY_s:           /* CTRL-ALT-s dumps all strings */
    string_dump();
    return;
    
  case PGKEY_n:           /* CTRL-ALT-n dumps all gropnodes */
    grop_dump();
    return;
    
  case PGKEY_t:           /* CTRL-ALT-t dumps all divnodes */
    div_dump();
    return;
    
  case PGKEY_g:           /* Just for fun :) */
    guru("GURU MEDITATION #3263827\n\nCongratulations!\n"
	 "    Either you have read the source code or\n"
	 "    you have very persistently banged your\n"
	 "    head on the keyboard ;-)");
    return;
    
  case PGKEY_m:           /* CTRL-ALT-m displays a memory profile */
    guru("Memory Profile\n\n"
	 "Total memory use: %d bytes in %d allocations\n\n"
	 "%d bytes in %d gropnodes\n"
	 "%d bytes in %d zombie gropnodes\n"
	 "%d bytes in %d divnodes\n"
	 "%d bytes in %d widgets\n"
	 "%d bytes in %d handle nodes",
	 memamt,memref,
	 num_grops*sizeof(struct gropnode),num_grops,
	 grop_zombie_count*sizeof(struct gropnode),grop_zombie_count,
	 num_divs*sizeof(struct divnode),num_divs,
	 num_widgets*sizeof(struct widget),num_widgets,
	 num_handles*sizeof(struct handlenode),num_handles);
    return;
    
  case PGKEY_b:           /* CTRL-ALT-b blanks the screen */
    {
      s16 lxres,lyres;
      VID(bitmap_getsize)(magic_cursor_display(), &lxres, &lyres);

      VID(rect)   (magic_cursor_display(), 0,0,lxres,lyres, 
		   VID(color_pgtohwr) (0),PG_LGOP_NONE);
      VID(update) (magic_cursor_display(),0,0,lxres,lyres);
    }
    return;
    
  case PGKEY_y:           /* CTRL-ALT-y unsynchronizes the screen buffers */
    {
      /* The buffers in PicoGUI normally like to be synchronized.
       * Data flows from the divtree to the backbuffer to the screen.
       * The purpose of this debugging key is to put a different
       * image on the screen (a black rectangle) than is in the rest
       * of the pipeline, so that by watching the data ooze out one
       * can tell if the correct update regions are being used and
       * in general prod at the video driver.
       * This would be very simple if not for the fact that only the video
       * driver has access to the screen's buffer. The procedure here is
       * to pump the black screen all the way through, then reinitializing
       * the backbuffer while being very carefull not to update right away
       * or mess up the sprites.
       */
      
      struct divtree *p;
      s16 lxres,lyres;
      VID(bitmap_getsize)(magic_cursor_display(), &lxres, &lyres);

      /* Push through the black screen */
      VID(rect)   (magic_cursor_display(), 0,0,lxres,lyres, 
		   VID(color_pgtohwr) (0),PG_LGOP_NONE);
      VID(update) (magic_cursor_display(),0,0,lxres,lyres);
      /* Force redrawing everything to the backbuffer */
      for (p=dts->top;p;p=p->next)
	p->flags |= DIVTREE_ALL_REDRAW;
      update(NULL,0);  /* Note the zero flag! */
      /* Clear the update rectangle, breaking the
	 pipeline that usually works so well :) */
      for (p=dts->top;p;p=p->next)
	p->update_rect.w = 0;
      /* The above zero flag left sprites off.
	 With sprites off it's tough to use the mouse! */
      VID(sprite_showall) ();
    }
    return;
    
  case PGKEY_u:           /* CTRL-ALT-u makes a blue screen */
    {
      s16 lxres,lyres;
      VID(bitmap_getsize)(magic_cursor_display(), &lxres, &lyres);
      
      VID(rect) (magic_cursor_display(),0,0,lxres,lyres,
		 VID(color_pgtohwr) (0x0000FF), PG_LGOP_NONE);
      VID(update) (magic_cursor_display(),0,0,lxres,lyres);
    }
    return;
    
  case PGKEY_p:           /* CTRL-ALT-p shows all loaded bitmaps */
    {
      struct debug_bitmaps_data data;
      s16 lxres,lyres;
      VID(bitmap_getsize)(VID(window_debug)(), &lxres, &lyres);
      memset(&data,0,sizeof(data));

      guru("Table of loaded bitmaps:");
      handle_iterate(PG_TYPE_BITMAP,&debug_bitmaps,&data);
      VID(update) (VID(window_debug)(),0,0,lxres,lyres);
    }
    return;
    
  case PGKEY_o:           /* CTRL-ALT-o traces all divnodes */
    {
      s16 lxres,lyres;
      VID(bitmap_getsize)(magic_cursor_display(), &lxres, &lyres);
      r_divnode_trace(dts->top->head);
      VID(update) (magic_cursor_display(),0,0,lxres,lyres);
    }
    return;

  case PGKEY_a:           /* CTRL-ALT-a shows application info */
    {
      struct app_info *a;
      const struct pgstring *name;

      for (a=applist;a;a=a->next) {
	if (iserror(rdhandle((void**)&name,PG_TYPE_PGSTRING,-1,a->name)))
	  name = pgstring_tmpwrap("(error reading handle)");
	printf("app: '");
	pgstring_print(name);
	printf("' type=%d owner=%d\n",a->type,a->owner);
      }
    }
    return;

  case PGKEY_r:           /* CTRL-ALT-r draws the hotspot graph */
    {
      struct hotspot *p;
      s16 lxres,lyres;
      VID(bitmap_getsize)(magic_cursor_display(), &lxres, &lyres);

      for (p=hotspotlist;p;p=p->next)
	hotspot_draw(p);
      VID(update) (magic_cursor_display(),0,0,lxres,lyres);
    }    
    return;

  case PGKEY_i:           /* CTRL-ALT-i gets video mode info */
    {
      int x,y,celw,celh, pixelx,pixely;
      hwrcolor i, white, black, numcolors;
      s16 lxres,lyres;
      VID(bitmap_getsize)(VID(window_debug)(), &lxres, &lyres);

      guru("Video mode:\n"
	   "Logical %dx%d, physical %dx%d, %d-bit color\n"
	   "\n"
	   "Color palette:\n",
	   vid->lxres, vid->lyres, vid->xres, vid->yres, vid->bpp);      

      if (vid->bpp <= 8) {
	/* Actual palette display */

	i = 0;
	celw = (lxres-20) >> 4;
	celh = (lyres-70) >> 4;
	if (celw < celh)
	  celh = celw;
	else
	  celw = celh;

	white = VID(color_pgtohwr)(PGC_WHITE);
	black = VID(color_pgtohwr)(PGC_BLACK);
	numcolors = 1 << vid->bpp;
	
	for (y=0;y<16;y++)
	  for (x=0;x<16 && i<numcolors;x++,i++) {
	    pixelx = x*celw+10;
	    pixely = y*celh+60;
	    
	    /* Display a rectangle of the color with black and white borders.
	     * This is a lot like what the scribble app does, but we don't have
	     * the convenience of a "frame" gropnode at this low level.
	     */
	    VID(slab)(VID(window_debug)(),pixelx,pixely,celw,white,PG_LGOP_NONE);
	    VID(slab)(VID(window_debug)(),pixelx,pixely+celh,celw,white,PG_LGOP_NONE);
	    VID(bar)(VID(window_debug)(),pixelx,pixely,celh,white,PG_LGOP_NONE);
	    VID(bar)(VID(window_debug)(),pixelx+celw,pixely,celh,white,PG_LGOP_NONE);
	    VID(slab)(VID(window_debug)(),pixelx+1,pixely+1,celw-2, black, PG_LGOP_NONE);
	    VID(slab)(VID(window_debug)(),pixelx+1,pixely-1+celh,celw-2, black, PG_LGOP_NONE);
	    VID(bar)(VID(window_debug)(),pixelx+1,pixely+1,celh-2, black, PG_LGOP_NONE);
	    VID(bar)(VID(window_debug)(),pixelx-1+celh,pixely+1,celh-2, black, PG_LGOP_NONE);
	    VID(rect)(VID(window_debug)(),pixelx+2,pixely+2,celw-3,celh-3, i    , PG_LGOP_NONE);
	  }
      }
      else {
	/* Just some RGB gradients */

        y = 60;
	VID(gradient)(VID(window_debug)(),10,y,lxres-20,20,0,
		      0x000000,0xFFFFFF, PG_LGOP_NONE);
	y += 30;
	VID(gradient)(VID(window_debug)(),10,y,lxres-20,20,0,
		      0x000000,0xFF0000, PG_LGOP_NONE);
	y += 30;
	VID(gradient)(VID(window_debug)(),10,y,lxres-20,20,0,
		      0x000000,0x00FF00, PG_LGOP_NONE);
	y += 30;
	VID(gradient)(VID(window_debug)(),10,y,lxres-20,20,0,
		      0x000000,0x0000FF, PG_LGOP_NONE);
	y += 30;
      }

      VID(update) (VID(window_debug)(),0,0,lxres,lyres);
    }
    return;

  case PGKEY_z:           /* Just for fun :) */
    guru("Void* courses through my veins like Giant Radioactive Rubber Pants!\n"
	 "The pants command me!\n"
	 "Do not ignore my veins!");
    return;

#endif /* DEBUG_KEYS */
    
  }
}

/* The End */






