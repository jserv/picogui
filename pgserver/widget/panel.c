/*
 * panel.c - simple container widget
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <widget.h>
#include <divtree.h>
#include <g_malloc.h>

/* param.c - color */
void fill(struct divnode *d) {
  grop_rect(&d->grop,0,0,d->w,d->h,d->param.c);
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error panel_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_TOP;
  self->in->split = HWG_BUTTON+HWG_MARGIN*2;
  e = newdiv(&self->in->next,self);
  if (e.type != ERRT_NONE) return e;
  self->in->next->flags |= S_TOP;
  self->in->next->split = 1;
  self->out = &self->in->next->next;
  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &fill;
  self->in->div->param.c = panelmid;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->split = HWG_MARGIN;
  self->sub = &self->in->div->div;
  e = newdiv(&self->in->next->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->next->div->on_recalc = &fill;
  self->in->next->div->param.c = paneledge;

  return sucess;
}

void panel_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error panel_set(struct widget *self,int property, glob data) {
  switch (property) {
  case WP_SIZE:
    if (data<0) return mkerror(ERRT_BADPARAM,"WP_SIZE is negative (panel)");
    self->in->split = (int) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_SIDE:
    if ((data != S_LEFT) && (data != S_RIGHT) && (data != S_TOP) &&
	(data != S_BOTTOM)) return mkerror(ERRT_BADPARAM,
	"WP_SIDE param is not a valid side value (panel)");
    self->in->next->flags &= SIDEMASK;
    self->in->flags &= SIDEMASK;
    self->in->next->flags |= ((sidet)data);
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BGCOLOR:
    self->in->div->param.c = cnvcolor(data);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BORDERCOLOR:
    self->in->next->div->param.c = cnvcolor(data);
    self->in->next->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BORDERSIZE:
    if (data<0) return mkerror(ERRT_BADPARAM,
			       "WP_BORDERSIZE is negative (panel)");
    self->in->next->split = (int) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_SIZEMODE:
    if ((data != SZMODE_PIXEL) && (data != SZMODE_PERCENT)) return
       mkerror(ERRT_BADPARAM,"WP_SIZEMODE invalid sizemode (panel)");
    self->in->flags &= SZMODE_MASK;
    self->in->flags |= data | DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for panel");
  }
  return sucess;
}

glob panel_get(struct widget *self,int property) {
  switch (property) {
  case WP_SIZE:
    return self->in->split;

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_COLOR:
    return self->in->div->param.c;

  case WP_BORDERCOLOR:
    return self->in->next->div->param.c;

  case WP_BORDERSIZE:
    return self->in->next->split;

  case WP_SIZEMODE:
    return self->in->flags & (~SZMODE_MASK);
  }
  return 0;
}

/* The End */




