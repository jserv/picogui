/* $Id: button.c,v 1.12 2000/04/29 21:57:45 micahjd Exp $
 *
 * button.c - generic button, with a string or a bitmap
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <widget.h>
#include <divtree.h>
#include <font.h>
#include <g_malloc.h>
#include <appmgr.h>
#include <theme.h>

/* Button content offsetted in X and Y coords by this much when on */
#define ON_OFFSET 1

/* This widget has extra data we can't store in the divnodes themselves */
struct btndata {
  int on,over;
  int x,y;      /* Coordinates of the button's content (bitmap or text) */
  int text;     /* If this is nonzero, this is a bitmap button instead
		   of a text button and the 'bitmap' param structure
		   will be used. */
};
#define DATA ((struct btndata *)(self->data))

/* Grop structure:
   Node#  Type  Purpose
   --------------------
   0    element Fill     (fill's x,y,w,h have no effect)
   1    element Border
   2    bit/txt Mask
   3    bit/txt Item
   4    element Overlay
*/
void bitbutton(struct divnode *d) {
  int ex,ey,ew,eh,x,y,w,h;
  struct bitmap *bit;
  struct widget *self = d->owner;

  ex=ey=0; ew=d->w; eh=d->h;

  addelement(&d->grop,&current_theme[E_BUTTON_BORDER],&ex,&ey,&ew,&eh,1);
  addelement(&d->grop,&current_theme[E_BUTTON_FILL],&ex,&ey,&ew,&eh,1);

  /* We need at least the main bitmap for alignment.  The mask
     bitmap is optional, but without it what's the point... */
  if (d->param.bitmap.bitmap && (rdhandle((void **) &bit,TYPE_BITMAP,-1,
      d->param.bitmap.bitmap).type==ERRT_NONE) && bit) {
    w = bit->w;
    h = bit->h;
    align(d,d->param.bitmap.align,&w,&h,&x,&y);

    /* AND the mask, then OR the bitmap. Yay for transparency effects! */
    DATA->x = x;
    DATA->y = y;
    if (DATA->on) {
      x+=ON_OFFSET;
      y+=ON_OFFSET;
    }
    grop_bitmap(&d->grop,x,y,w,h,d->param.bitmap.mask,LGOP_AND);
    grop_bitmap(&d->grop,x,y,w,h,d->param.bitmap.bitmap,LGOP_OR);
  }
  else {
    grop_null(&d->grop);
    grop_null(&d->grop);
  }

  addelement(&d->grop,&current_theme[E_BUTTON_OVERLAY],&ex,&ey,&ew,&eh,0);
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error button_install(struct widget *self) {
  g_error e;

  e = g_malloc(&self->data,sizeof(struct btndata));
  if (e.type != ERRT_NONE) return e;
  memset(self->data,0,sizeof(struct btndata));

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_LEFT;
  self->in->split = HWG_BUTTON;

  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &bitbutton;

  e = newdiv(&self->in->next,self);
  if (e.type != ERRT_NONE) return e;
  self->in->next->flags |= S_LEFT;
  self->in->next->split = HWG_MARGIN;
  self->out = &self->in->next->next;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | 
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_DIRECT;

  return sucess;
}

void button_remove(struct widget *self) {
  g_free(self->data);

  r_divnode_free(self->in);
}

g_error button_set(struct widget *self,int property, glob data) {
  g_error e;
  struct bitmap *bit;

  switch (property) {

  case WP_SIDE:
    if ((data != S_LEFT) && (data != S_RIGHT) && (data != S_TOP) &&
	(data != S_BOTTOM)) return mkerror(ERRT_BADPARAM,
	"WP_SIDE param is not a valid side value (button)");
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    self->in->next->flags &= SIDEMASK;
    self->in->next->flags |= ((sidet)data);
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_ALIGN:
    if (data > AMAX) return mkerror(ERRT_BADPARAM,
		     "WP_ALIGN param is not a valid align value (button)");
    self->in->div->param.bitmap.align = (alignt) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BITMAP:
    if (rdhandle((void **)&bit,TYPE_BITMAP,-1,data).type==ERRT_NONE && bit) {
      self->in->div->param.bitmap.bitmap = (handle) data;
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(ERRT_HANDLE,"WP_BITMAP invalid bitmap handle");
    break;

  case WP_BITMASK:
    if (rdhandle((void **)&bit,TYPE_BITMAP,-1,data).type==ERRT_NONE && bit) {
      self->in->div->param.bitmap.mask = (handle) data;
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(ERRT_HANDLE,"WP_BITMAP invalid bitmap mask handle");
    break;

  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for button");
  }
  return sucess;
}

glob button_get(struct widget *self,int property) {
  g_error e;
  handle h;

  switch (property) {

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_ALIGN:
    return self->in->div->param.bitmap.align;

  default:
    return 0;
  }
}

void button_trigger(struct widget *self,long type,union trigparam *param) {

  /* Figure out the button's new state */
  switch (type) {

  case TRIGGER_ENTER:
    DATA->over=1;
    break;
    
  case TRIGGER_LEAVE:
    DATA->over=0;
    break;
    
  case TRIGGER_DOWN:
    if (param->mouse.chbtn==1)
      DATA->on=1;
    break;

  case TRIGGER_UP:
    if (DATA->on && param->mouse.chbtn==1) {
      post_event(WE_ACTIVATE,self,0);
      DATA->on=0;
    }
    break;

  case TRIGGER_RELEASE:
    if (param->mouse.chbtn==1)
      DATA->on=0;
    break;

  case TRIGGER_DIRECT:
    post_event(WE_ACTIVATE,self,1);
    break;
    
  }

  /* If we're busy rebuilding the grop list, don't bother poking
     at the individual nodes */
  if (self->in->div->grop_lock)
    return;

  /* This code for updating the button's appearance modifies
     the grops directly because it does not need a recalc, only
     a single-node redraw. Recalcs propagate like a virus, and
     require recreating grop-lists.
     Redraws don't spread to other nodes, and they are very fast.
  */
  if (DATA->on && DATA->over) {
    applystate(self->in->div->grop,
	       &current_theme[E_BUTTON_BORDER],
	       STATE_ACTIVATE);
    applystate(self->in->div->grop->next,
	       &current_theme[E_BUTTON_FILL],
	       STATE_ACTIVATE);

    self->in->div->grop->next->next->x = DATA->x + ON_OFFSET;
    self->in->div->grop->next->next->next->x = DATA->x + ON_OFFSET;
    self->in->div->grop->next->next->y = DATA->y + ON_OFFSET;
    self->in->div->grop->next->next->next->y = DATA->y + ON_OFFSET;

    applystate(self->in->div->grop->next->next->next->next,
	       &current_theme[E_BUTTON_OVERLAY],
	       STATE_ACTIVATE);
  }
  else if (DATA->over) {
    applystate(self->in->div->grop,
	       &current_theme[E_BUTTON_BORDER],
	       STATE_HILIGHT);
    applystate(self->in->div->grop->next,
	       &current_theme[E_BUTTON_FILL],
	       STATE_HILIGHT);

    self->in->div->grop->next->next->x = DATA->x;
    self->in->div->grop->next->next->next->x = DATA->x;
    self->in->div->grop->next->next->y = DATA->y;
    self->in->div->grop->next->next->next->y = DATA->y;

    applystate(self->in->div->grop->next->next->next->next,
	       &current_theme[E_BUTTON_OVERLAY],
	       STATE_HILIGHT);
  }
  else {
    applystate(self->in->div->grop,
	       &current_theme[E_BUTTON_BORDER],
	       STATE_NORMAL);
    applystate(self->in->div->grop->next,
	       &current_theme[E_BUTTON_FILL],
	       STATE_NORMAL);

    self->in->div->grop->next->next->x = DATA->x;
    self->in->div->grop->next->next->next->x = DATA->x;
    self->in->div->grop->next->next->y = DATA->y;
    self->in->div->grop->next->next->next->y = DATA->y;

    applystate(self->in->div->grop->next->next->next->next,
	       &current_theme[E_BUTTON_OVERLAY],
	       STATE_NORMAL);
  }

  self->in->div->flags |= DIVNODE_NEED_REDRAW;
  self->dt->flags |= DIVTREE_NEED_REDRAW;   
}

/* The End */



