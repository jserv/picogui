/*
 * indicator.c - progress meter, battery bar, etc.
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
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




