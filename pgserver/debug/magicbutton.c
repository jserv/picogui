/* $Id: magicbutton.c,v 1.7 2002/02/05 01:51:34 micahjd Exp $
 *
 * magicbutton.c - CTRL-ALT-foo is magical
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 * pgCreateWidget & pgAttachWidget functionality added by RidgeRun Inc.
 * Copyright (C) 2001 RidgeRun, Inc.  All rights reserved.
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

#include <string.h>
#include <pgserver/common.h>
#include <pgserver/video.h>
#include <pgserver/render.h>
#include <pgserver/font.h>
#include <pgserver/appmgr.h>
#include <pgserver/widget.h>
#include <pgserver/hotspot.h>

#ifdef DEBUG_KEYS
   
int db_x,db_y,db_h;
extern struct hotspot *hotspotlist;
   
g_error debug_bitmaps(const void **pobj) {
   hwrbitmap bmp = (hwrbitmap) *pobj;
   s16 w,h;
   
   VID(bitmap_getsize) (bmp,&w,&h);
   if (db_x+10+w>vid->lxres) {
     db_x = 0;
     db_y += db_h+8;
     db_h = 0;
   }
   if (h>db_h)
     db_h = h;
   
   if (db_y+45+h>vid->lyres) {
      struct fontdesc *df=NULL;
      struct quad screenclip;
      screenclip.x1 = screenclip.y1 = 0;
      screenclip.x2 = vid->lxres-1;
      screenclip.y2 = vid->lyres-1;
      rdhandle((void**)&df,PG_TYPE_FONTDESC,-1,defaultfont);

      outtext(vid->display,df,10,vid->lyres-df->font->h*3,VID(color_pgtohwr) (0xFFFF00),
	      "Too many bitmaps for this screen.\nChange video mode and try again",
	      &screenclip,PG_LGOP_NONE,0);

      return success;   /* Lies! :) */
   }
   
   VID(rect) (vid->display,db_x+3,db_y+38,w+4,h+4,
	      VID(color_pgtohwr)(0xFFFFFF),PG_LGOP_NONE);
   VID(rect) (vid->display,db_x+4,db_y+39,w+2,h+2,
	      VID(color_pgtohwr)(0x000000),PG_LGOP_NONE);
   VID(blit) (vid->display,db_x+5,db_y+40,w,h,bmp,0,0,PG_LGOP_NONE);

   db_x += w+8;
   return success;
}
   
   /* Utility functions to implement CTRL-ALT-N gropnode dump */
void r_grop_dump(struct divnode *div) {
   struct gropnode *n;
   int i;
   
   if (!div) return;
   if (div->grop) {
      printf("Divnode %p at (%d,%d,%d,%d): ",div,
	     div->x,div->y,div->w,div->h);
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
	  " calc=(%d,%d,%d,%d) nextline=%p divscroll=%p trans=(%d,%d)\n",
	  div,div->flags,div->split,div->pw,div->ph,
	  div->cw,div->ch,
	  div->x,div->y,div->w,div->h,
	  div->calcx,div->calcy,div->calcw,div->calch,
	  div->nextline, div->divscroll,
	  div->tx, div->ty);

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

  if (!div)
    return;

  memset(&r,0,sizeof(r));
  memset(&n,0,sizeof(n));

  /* The scroll container must be visible */
  if (div->divscroll && !(div->divscroll->calcw && div->divscroll->calch))
    return;

  /* Set up rendering... */
  r.output = vid->display;
  n.r.x = div->x;
  n.r.y = div->y;
  n.r.w = div->w;
  n.r.h = div->h;
  r.clip.x1 = 0;
  r.clip.y1 = 0;
  r.clip.x2 = vid->lxres-1;
  r.clip.y2 = vid->lyres-1;

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

  /* Set up rendering...
   */
  memset(&r,0,sizeof(r));
  memset(&n,0,sizeof(n));
  r.output = vid->display;
  r.clip.x1 = 0;
  r.clip.y1 = 0;
  r.clip.x2 = vid->lxres-1;
  r.clip.y2 = vid->lyres-1;
  r.lgop = PG_LGOP_NONE;

  /* Draw arrows for all the directions in the graph
   */
  for (i=0;i<=HOTSPOTMAX;i++)
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
      n.r.x = spot->div->divscroll->calcx;
      n.r.y = spot->div->divscroll->calcy;
      n.r.w = spot->div->divscroll->calcw;
      n.r.h = spot->div->divscroll->calch;
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
   
void magic_button(s16 key) {
  switch (key) {
    
  case PGKEY_SLASH:       /* CTRL-ALT-SLASH exits */
    request_quit();
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
    guru("GURU MEDITATION #%08X\n\nCongratulations!\n"
	 "    Either you have read the source code or\n"
	 "    you have very persistently banged your\n"
	 "    head on the keyboard ;-)",div_under_crsr);
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
    VID(rect)   (vid->display, 0,0,vid->lxres,vid->lyres, 
		 VID(color_pgtohwr) (0),PG_LGOP_NONE);
    VID(update) (0,0,vid->lxres,vid->lyres);
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
      /* Push through the black screen */
      VID(rect)   (vid->display, 0,0,vid->lxres,vid->lyres, 
		   VID(color_pgtohwr) (0),PG_LGOP_NONE);
      VID(update) (0,0,vid->lxres,vid->lyres);
      /* Force redrawing everything to the backbuffer */
      p = dts->top;
      while (p) {
	p->flags |= DIVTREE_ALL_REDRAW;
	p = p->next;
      }
      update(NULL,0);  /* Note the zero flag! */
      /* Clear the update rectangle, breaking the
	 pipeline that usually works so well :) */
      upd_w = 0;
      /* The above zero flag left sprites off.
	 With sprites off it's tough to use the mouse! */
      VID(sprite_showall) ();
    }
    return;
    
  case PGKEY_u:           /* CTRL-ALT-u makes a blue screen */
    VID(rect) (vid->display,0,0,vid->lxres,vid->lyres,
	       VID(color_pgtohwr) (0x0000FF), PG_LGOP_NONE);
    VID(update) (0,0,vid->lxres,vid->lyres);
    return;
    
  case PGKEY_p:           /* CTRL-ALT-p shows all loaded bitmaps */
    guru("Table of loaded bitmaps:");
    /* Reset evil globals :) */
    db_x = db_y = db_h = 0;
    handle_iterate(PG_TYPE_BITMAP,&debug_bitmaps);
    VID(update) (0,0,vid->lxres,vid->lyres);
    return;
    
  case PGKEY_o:           /* CTRL-ALT-o traces all divnodes */
    r_divnode_trace(dts->top->head);
    VID(update) (0,0,vid->lxres,vid->lyres);
    return;

  case PGKEY_a:           /* CTRL-ALT-a shows application info */
    {
      struct app_info *a;
      const char *name;

      for (a=applist;a;a=a->next) {
	if (iserror(rdhandle((void**)&name,PG_TYPE_STRING,-1,a->name)))
	  name = "(error reading handle)";
	printf("app: '%s' type=%d owner=%d\n",name,a->type,a->owner);
      }
    }
    return;

  case PGKEY_r:           /* CTRL-ALT-r draws the hotspot graph */
    {
      struct hotspot *p;
      for (p=hotspotlist;p;p=p->next)
	hotspot_draw(p);
      VID(update) (0,0,vid->lxres,vid->lyres);
    }    
    return;

#endif /* DEBUG_KEYS */
    
  }
}

/* The End */








