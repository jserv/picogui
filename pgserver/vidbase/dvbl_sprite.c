/* $Id: dvbl_sprite.c,v 1.1 2002/04/03 08:08:41 micahjd Exp $
 *
 * dvbl_sprite.c - This file is part of the Default Video Base Library,
 *                 providing the basic video functionality in picogui but
 *                 easily overridden by drivers.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
 */

#include <pgserver/common.h>

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/render.h>

void def_sprite_show(struct sprite *spr) {
  static struct quad cr;

  //  DBG("spr = %p\n",spr);

  if (disable_output)
    return;
   
  if (spr->onscreen || !spr->visible || !vid->xres) return;
   
  /* Clip to a divnode */
  if (spr->clip_to) {
    if (spr->x < spr->clip_to->x) spr->x = spr->clip_to->x;
    if (spr->y < spr->clip_to->y) spr->y = spr->clip_to->y;
    if (spr->x+spr->w > spr->clip_to->x+spr->clip_to->w)
      spr->x = spr->clip_to->x + spr->clip_to->w - spr->w;
    if (spr->y+spr->h > spr->clip_to->y+spr->clip_to->h)
      spr->y = spr->clip_to->y + spr->clip_to->h - spr->h;

    spr->ow = spr->w; spr->oh = spr->h;
  }
  else {
    /* Clip to screen edge, cursor style. For correct mouse cursor
     * functionality and for sanity. 
     */
    if (spr->x<0) 
       spr->x = 0;
    else if (spr->x>(vid->lxres-1))
       spr->x = vid->lxres-1;
     
    if (spr->y<0)
       spr->y = 0;
    else if (spr->y>(vid->lyres-1))
       spr->y = vid->lyres-1;
    
    spr->ow = vid->lxres - spr->x;
    if (spr->ow > spr->w) spr->ow = spr->w;
    spr->oh = vid->lyres - spr->y;
    if (spr->oh > spr->h) spr->oh = spr->h;     
  }

  /* Update coordinates */
  spr->ox = spr->x; spr->oy = spr->y;

  cr.x1 = spr->x;
  cr.y1 = spr->y;
  cr.x2 = spr->x+spr->w-1;
  cr.y2 = spr->y+spr->h-1;
   
  /* Protect that area of the screen */
  def_sprite_protectarea(&cr,spr->next);
  
  /* Grab a new backbuffer */
  VID(blit) (spr->backbuffer,0,0,spr->ow,spr->oh,
	     vid->display,spr->x,spr->y,PG_LGOP_NONE);

  /* Display the sprite */
  if (spr->mask && *spr->mask) {
     VID(blit) (vid->display,spr->x,spr->y,spr->ow,spr->oh,
		*spr->mask,0,0,PG_LGOP_AND);
     VID(blit) (vid->display,spr->x,spr->y,spr->ow,spr->oh,
		*spr->bitmap,0,0,PG_LGOP_OR);
  }
   else
     VID(blit) (vid->display,spr->x,spr->y,spr->ow,spr->oh,
		*spr->bitmap,0,0,spr->lgop);
   
  add_updarea(spr->x,spr->y,spr->ow,spr->oh);

  spr->onscreen = 1;
   
   /**** Debuggative Cruft - something I used to test line clipping ****/
/*
    {
	int xp[] = {
	   55,-5,-55,5,30,0,-30,0
	};
	int yp[] = {
	   5,55,-5,-55,0,-30,0,30
	};
	struct divnode d;
	struct gropnode g;
	int i;
	memset(&d,0,sizeof(d));
	memset(&g,0,sizeof(g));
	d.x = 100;
	d.y = 100;
	d.w = 93;
	d.h = 72;
	d.grop = &g;
	g.type = PG_GROP_LINE;
	g.param[0] = (*vid->color_pgtohwr) (0xFFFF00);
	VID(rect) (d.x,d.y,d.w,d.h,(*vid->color_pgtohwr)(0x004000));
	g.x = spr->x-d.x;
	g.y = spr->y-d.y;
	for (i=0;i<8;i++) {
	   g.x += g.w; 
	   g.y += g.h;
	   g.w = xp[i];
	   g.h = yp[i];
	   grop_render(&d);
	}
	VID(update) (d.x,d.y,d.w,d.h);
     }
*/

   /**** A very similar debuggative cruft to test text clipping ****/
  /*
    {
      struct quad cr;
      struct fontdesc fd;

      memset(&fd,0,sizeof(fd));
      fd.fs = fontstyles;
      fd.font = fd.fs->normal;
      fd.hline = -1;
      fd.decoder = decode_ascii;
 
      cr.x1 = 100;
      cr.y1 = 100;
      cr.x2 = 150;
      cr.y2 = 180;
      VID(rect) (vid->display,cr.x1,cr.y1,cr.x2-cr.x1+1,cr.y2-cr.y1+1,(*vid->color_pgtohwr)(0x004000),PG_LGOP_NONE);
      outtext(vid->display,&fd,spr->x,spr->y,(*vid->color_pgtohwr) (0xFFFF80),"Hello,\nWorld!",&cr,PG_LGOP_NONE,0);
      //      outtext_v(&fd,spr->x,spr->y,(*vid->color_pgtohwr) (0xFFFF80),"Hello,\nWorld!",&cr);
//      outtext_v(&fd,spr->x,spr->y,(*vid->color_pgtohwr) (0xFFFF80),"E",&cr);
      VID(update) (0,0,vid->lxres,vid->lyres);
    }
  */
   
}

void def_sprite_hide(struct sprite *spr) {
  static struct quad cr;

  //  DBG("spr = %p\n",spr);

  if (disable_output)
    return;
   
  if ( (!spr->onscreen) ||
       (spr->ox == -1) )
     return;

  cr.x1 = spr->x;
  cr.y1 = spr->y;
  cr.x2 = spr->x+spr->w-1;
  cr.y2 = spr->y+spr->h-1;
   
  /* Protect that area of the screen */
  def_sprite_protectarea(&cr,spr->next);
   
  /* Put back the old image */
  VID(blit) (vid->display,spr->ox,spr->oy,spr->ow,spr->oh,
	     spr->backbuffer,0,0,PG_LGOP_NONE);
  add_updarea(spr->ox,spr->oy,spr->ow,spr->oh);

  spr->onscreen = 0;
}

void def_sprite_update(struct sprite *spr) {
  //  DBG("spr = %p\n",spr);

  (*vid->sprite_hide) (spr);
  (*vid->sprite_show) (spr);

  /* Redraw */
  realize_updareas();
}

/* Traverse first -> last, showing sprites */
void def_sprite_showall(void) {
  struct sprite *p = spritelist;

  DBG("\n");

  while (p) {
    (*vid->sprite_show) (p);
    p = p->next;
  }
}

/* Traverse last -> first, hiding sprites */
void r_spritehide(struct sprite *s) {
  if (!s) return;
  r_spritehide(s->next);
  (*vid->sprite_hide) (s);
}
void def_sprite_hideall(void) {
  DBG("\n");
  r_spritehide(spritelist);
}

/* Hide necessary sprites in a given area */
void def_sprite_protectarea(struct quad *in,struct sprite *from) {
   /* Base case: from is null */
   if (!from) return;

   //   DBG("quad(%d %d %d %d)\n",in->x1,in->y1,in->x2,in->y2);

   /* Load this all on the stack so we go backwards */
   def_sprite_protectarea(in,from->next);
   
   /* Hide this sprite if necessary */
   if ( ((from->x+from->w) >= in->x1) &&
        ((from->y+from->h) >= in->y1) &&
        (from->x <= in->x2) &&
        (from->y <= in->y2) )
     (*vid->sprite_hide) (from);
}

/* The End */
