/* $Id$
 *
 * dvbl_sprite.c - This file is part of the Default Video Base Library,
 *                 providing the basic video functionality in picogui but
 *                 easily overridden by drivers.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/divtree.h>

void def_sprite_hide_above(struct sprite *spr);
int def_sprite_test_overlap(struct sprite *spr,  struct pgquad *r);
struct pgquad *def_sprite_quad(struct sprite *spr);

/******************************************** Public functions */

void def_sprite_show(struct sprite *spr) {
  int src_x = 0, src_y = 0;
  struct divtree *dt;

  //  DBG("spr = %p\n",spr);

  if (disable_output)
    return;
   
  if (spr->onscreen || !spr->visible || !vid->xres) return;
   
  if (iserror(rdhandle((void**)&dt, PG_TYPE_DIVTREE, -1, spr->dt)))
    return;

  /* Clip to a divnode */
  if (spr->clip_to) {
    if (spr->x < spr->clip_to->r.x) spr->x = spr->clip_to->r.x;
    if (spr->y < spr->clip_to->r.y) spr->y = spr->clip_to->r.y;
    if (spr->x+spr->w > spr->clip_to->r.x+spr->clip_to->r.w)
      spr->x = spr->clip_to->r.x + spr->clip_to->r.w - spr->w;
    if (spr->y+spr->h > spr->clip_to->r.y+spr->clip_to->r.h)
      spr->y = spr->clip_to->r.y + spr->clip_to->r.h - spr->h;

    spr->ow = spr->w; spr->oh = spr->h;
  }
  else {
    /* Clip to screen edge, cursor style. For correct mouse cursor
     * functionality and for sanity. 
     */
    if (spr->x<0) {
      src_x = -spr->x;
      spr->x = 0;
    }
    else if (spr->x>(vid->lxres-1))
      spr->x = vid->lxres-1;
    
    if (spr->y<0) {
      src_y = -spr->y;  
      spr->y = 0;
    }
    else if (spr->y>(vid->lyres-1))
       spr->y = vid->lyres-1;
    
    spr->ow = vid->lxres - spr->x;
    if (spr->ow > spr->w) spr->ow = spr->w;
    spr->oh = vid->lyres - spr->y;
    if (spr->oh > spr->h) spr->oh = spr->h;     
    spr->ow -= src_x;
    spr->oh -= src_y;

    /* Necessary if the sprite is larger than the screen.. (hey, it could happen) */
    if (spr->ow < 0)
      spr->ow = 0;
    if (spr->oh < 0)
      spr->oh = 0;
  }

  /* Update coordinates */
  spr->ox = spr->x; spr->oy = spr->y;

  /* Hide sprites above ours */
  def_sprite_hide_above(spr);
  
  /* Grab a new backbuffer */
  VID(blit) (spr->backbuffer,0,0,spr->ow,spr->oh,
	     dt->display,spr->x,spr->y,PG_LGOP_NONE);

  /* Display the sprite */
  if (spr->mask && *spr->mask) {
     VID(blit) (dt->display,spr->x,spr->y,spr->ow,spr->oh,
		*spr->mask,src_x,src_y,PG_LGOP_AND);
     VID(blit) (dt->display,spr->x,spr->y,spr->ow,spr->oh,
		*spr->bitmap,src_x,src_y,PG_LGOP_OR);
  }
  else {
    VID(blit) (dt->display,spr->x,spr->y,spr->ow,spr->oh,
	       *spr->bitmap,src_x,src_y,spr->lgop);
  }
   
  add_updarea(dt,spr->x,spr->y,spr->ow,spr->oh);

  spr->onscreen = 1;
}

void def_sprite_hide(struct sprite *spr) {
  static struct pgquad cr;
  struct divtree *dt;

  if (disable_output)
    return;

  if (iserror(rdhandle((void**)&dt, PG_TYPE_DIVTREE, -1, spr->dt)))
    return;
   
  if ( (!spr->onscreen) ||
       (spr->ox == -1) )
     return;

  cr.x1 = spr->x;
  cr.y1 = spr->y;
  cr.x2 = spr->x+spr->w-1;
  cr.y2 = spr->y+spr->h-1;

  /* Remove sprites above this one too */
  def_sprite_hide_above(spr);
   
  /* Put back the old image */
  VID(blit) (dt->display,spr->ox,spr->oy,spr->ow,spr->oh,
	     spr->backbuffer,0,0,PG_LGOP_NONE);
  add_updarea(dt,spr->ox,spr->oy,spr->ow,spr->oh);

  spr->onscreen = 0;
}

void def_sprite_update(struct sprite *spr) {
  struct divtree *dt;
  if (iserror(rdhandle((void**)&dt, PG_TYPE_DIVTREE, -1, spr->dt)))
    return;

  (*vid->sprite_hide) (spr);
  def_sprite_showall();       /* Also re-show the sprites we hid with protectarea */

  /* Redraw */
  realize_updareas(dt);
}

/* Traverse back -> front, showing sprites */
void def_sprite_showall(void) {
  struct sprite *p = spritelist;

  DBG("\n");

  while (p) {
    (*vid->sprite_show) (p);
    p = p->next;
  }
}

/* Traverse front -> back, hiding sprites */
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
void def_sprite_protectarea(struct pgquad *in,struct sprite *from) {
   /* Base case: from is null */
   if (!from) return;

   /* Load this all on the stack so we go backwards (front to back) */
   def_sprite_protectarea(in,from->next);
   
   /* Hide this sprite if necessary */
   if (def_sprite_test_overlap(from,in))
      (*vid->sprite_hide) (from);
}

/******************************************** Internal utilities */

/* Hide the first sprite above the specified one that overlaps it
 */
void def_sprite_hide_above(struct sprite *spr) {
  struct sprite *s;
  struct pgquad *q = def_sprite_quad(spr);

  for (s=spr->next;s;s=s->next)
    /* FIXME: IT shouldn't be necessary to hide the sprites that don't overlap,
     *        but for some reason the following commened out line doesn't work.
     */
    // if (def_sprite_test_overlap(s,q)) {
    {
      def_sprite_hide(spr->next);
      return;
    }
}

/* Test whether a sprite overlaps the given rectangle
 */
int def_sprite_test_overlap(struct sprite *spr,  struct pgquad *r) {
  return ((spr->x+spr->w) >= r->x1) &&
    ((spr->y+spr->h) >= r->y1) &&
    (spr->x <= r->x2) &&
    (spr->y <= r->y2);
}

/* Convert a sprite's position to a struct pgquad, for temporary use
 */
struct pgquad *def_sprite_quad(struct sprite *spr) {
  static struct pgquad cr;
  cr.x1 = spr->x;
  cr.y1 = spr->y;
  cr.x2 = spr->x+spr->w-1;
  cr.y2 = spr->y+spr->h-1;
  return &cr;
}

/* The End */
