/* $Id: indicator.c,v 1.4 2000/04/24 02:38:36 micahjd Exp $
 *
 * indicator.c - progress meter, battery bar, etc.
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

void indicator(struct divnode *d) {
  int x,y,w,h,p;
  x = y = 0;
  w = d->w;
  h = d->h;

  grop_frame(&d->grop,x,y,w,h,ind_b1); x++; y++; w-=2; h-=2;
  grop_frame(&d->grop,x,y,w,h,ind_b2); x++; y++; w-=2; h-=2;
  grop_frame(&d->grop,x,y,w,h,ind_b3); x++; y++; w-=2; h-=2;

  if (w>h) {
    p = w*d->param.i/100;
    grop_rect(&d->grop,x,y,p,h,ind_midon);
    grop_rect(&d->grop,x+p,y,w-p,h,ind_midoff);
  }
  else {
    p = h*(100-d->param.i)/100;
    grop_rect(&d->grop,x,y,w,p,ind_midoff);
    grop_rect(&d->grop,x,y+p,w,h-p,ind_midon);
  }
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error indicator_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_TOP;
  self->in->split = 10;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &indicator;
  self->in->div->param.i = 0;

  return sucess;
}

void indicator_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error indicator_set(struct widget *self,int property, glob data) {
  switch (property) {

  case WP_VALUE:
    if (data > 100) data = 100;
    if (data < 0) data = 0;
    self->in->div->param.i = (int) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_SIZE:
    if (data < 0) return mkerror(ERRT_BADPARAM,
				 "WP_SIZE param is negative (indicator)");
    self->in->split = (int) data;
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_SIDE:
    if ((data != S_LEFT) && (data != S_RIGHT) && (data != S_TOP) &&
	(data != S_BOTTOM)) return mkerror(ERRT_BADPARAM,
	"WP_SIDE param is not a valid side value (indicator)");
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;

  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for indicator");
  }
  return sucess;
}

glob indicator_get(struct widget *self,int property) {
  switch (property) {

  case WP_VALUE:
    return self->in->div->param.i;

  case WP_SIZE:
    return self->in->split;

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  }
  return 0;
}

/* The End */




