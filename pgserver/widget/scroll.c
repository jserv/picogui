/*
 * scroll.c - standard scroll indicator
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <widget.h>

void scrollbar(struct divnode *d) {
  int ind_h = d->param.i * (d->h-5) / 100;

  grop_rect(&d->grop,1,0,d->w-1,d->h,panelmid); 
  grop_bar(&d->grop,0,0,d->h,paneledge);
  grop_slab(&d->grop,1,ind_h++,d->w-1,black);
  grop_slab(&d->grop,1,ind_h++,d->w-1,gray);
  grop_slab(&d->grop,1,ind_h++,d->w-1,ltgray);
  grop_slab(&d->grop,1,ind_h++,d->w-1,gray);
  grop_slab(&d->grop,1,ind_h,d->w-1,black);
}

/* Nice and simple, the oncalc does most of the work */
g_error scroll_install(struct widget *self) {
  g_error e;
  
  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_RIGHT;
  self->in->split = HWG_SCROLL;
  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &scrollbar;
  self->out = &self->in->next;

  return sucess;
}

void scroll_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error scroll_set(struct widget *self,int property, glob data) {
  switch (property) {
  case WP_VALUE:
    if (data > 100) data = 100;
    if (data < 0) data = 0;
    self->in->div->param.i = data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for scroll");
  }
  return sucess;
}

glob scroll_get(struct widget *self,int property) {
  switch (property) {
  case WP_VALUE:
    return self->in->div->param.i;
  }
  return 0;
}

/* The End */
