/*
 * bitmap.c - just displays a bitmap, similar resizing and alignment to labels
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <widget.h>

#define BITMAP_MARGIN 0

/* param.bitmap - display a bitmap, with alignment */
void bitmap(struct divnode *d) {
  int x,y,w,h;
  struct bitmap *bit;

  if (!d->param.bitmap.transparent)
    grop_rect(&d->grop,0,0,d->w,d->h,d->param.bitmap.fill);

  /* Here if the bitmap is null we don't want to be blitting from the
     screen... */
  if (d->param.bitmap.bitmap && (rdhandle((void **) &bit,TYPE_BITMAP,-1,
      d->param.bitmap.bitmap).type==ERRT_NONE) && bit) {
    w = bit->w;
    h = bit->h;
    align(d,d->param.bitmap.align,&w,
	  &h,&x,&y);
    grop_bitmap(&d->grop,x,y,w,h,d->param.bitmap.bitmap,d->param.bitmap.lgop);
  }
}

void resizebitmap(struct widget *self);

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error bitmap_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_TOP;
  self->in->split = 0;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &bitmap;
  self->in->div->param.bitmap.fill = white;
  self->in->div->param.bitmap.transparent = 0; 
  self->in->div->param.bitmap.align = A_CENTER;
  self->in->div->param.bitmap.lgop = LGOP_NONE;

  return sucess;
}

void bitmap_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error bitmap_set(struct widget *self,int property, glob data) {
  struct bitmap *bit;

  switch (property) {

  case WP_BITMAP_SIDE:
    if ((data != S_LEFT) && (data != S_RIGHT) && (data != S_TOP) &&
	(data != S_BOTTOM)) return mkerror(ERRT_BADPARAM,
	"WP_BITMAP_SIDE param is not a valid side value");
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC;
    resizebitmap(self);
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BITMAP_COLOR:
    self->in->div->param.bitmap.fill = cnvcolor(data);
    self->in->div->param.bitmap.transparent = 0;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BITMAP_TRANSPARENT:
    self->in->div->param.bitmap.transparent = (data != 0);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BITMAP_ALIGN:
    if (data > AMAX) return mkerror(ERRT_BADPARAM,
		     "WP_BITMAP_ALIGN param is not a valid align value");
    if (data==A_ALL || self->in->div->param.bitmap.align==A_ALL) {
      self->in->div->param.bitmap.align = (alignt) data;
      resizebitmap(self);
    }
    else { 
      self->in->div->param.bitmap.align = (alignt) data;
    }
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BITMAP_LGOP:
    if (data > LGOPMAX) return mkerror(ERRT_BADPARAM,
		     "WP_BITMAP_LGOP param is not a valid lgop value");
    self->in->div->param.bitmap.lgop = data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BITMAP_BITMAP:
    if (!data) {
      self->in->flags |= DIVNODE_NEED_REDRAW;
      self->dt->flags |= DIVTREE_NEED_REDRAW;
    }
    else if (rdhandle((void **)&bit,TYPE_BITMAP,-1,data).type==ERRT_NONE && bit) {
      self->in->div->param.bitmap.bitmap = (handle) data;
      resizebitmap(self);
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(ERRT_HANDLE,"WP_BITMAP_BITMAP invalid bitmap handle");
    break;

  }

  return sucess;
}

glob bitmap_get(struct widget *self,int property) {
  switch (property) {

  case WP_BITMAP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_BITMAP_COLOR:
    return self->in->div->param.bitmap.fill;
    
  case WP_BITMAP_TRANSPARENT:
    return self->in->div->param.bitmap.transparent;

  case WP_BITMAP_ALIGN:
    return self->in->div->param.bitmap.align;

  case WP_BITMAP_LGOP:
    return self->in->div->param.bitmap.lgop;

  case WP_BITMAP_BITMAP:
    return (glob) self->in->div->param.bitmap.bitmap;

  }
}
 
void resizebitmap(struct widget *self) {
  struct bitmap *bit;
 
  if (rdhandle((void **) &bit,TYPE_BITMAP,-1,
	       self->in->div->param.bitmap.bitmap).type!=ERRT_NONE)
    return;
  if (!bit) return;

  if (self->in->div->param.bitmap.align == A_ALL)
    /* Expand to all free space (div will clip this) */
    self->in->split = (HWR_WIDTH>HWR_HEIGHT) ? HWR_WIDTH : HWR_HEIGHT;
  else if ((self->in->flags & DIVNODE_SPLIT_TOP) ||
      (self->in->flags & DIVNODE_SPLIT_BOTTOM))
    self->in->split = bit->h + BITMAP_MARGIN;
  else if ((self->in->flags & DIVNODE_SPLIT_LEFT) ||
	   (self->in->flags & DIVNODE_SPLIT_RIGHT))
    self->in->split = bit->w + BITMAP_MARGIN;
}

/* The End */











