/*
 * button.c - generic button
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <widget.h>
#include <divtree.h>
#include <font.h>
#include <g_malloc.h>
#include <appmgr.h>

void bitbutton(struct divnode *d) {
  int x,y,w,h,p;
  x = y = 0;
  w = d->w;
  h = d->h;

  grop_frame(&d->grop,x,y,w,h,btn_b1); x++; y++; w-=2; h-=2;
  grop_frame(&d->grop,x,y,w,h,btn_b2); x++; y++; w-=2; h-=2;
  grop_rect(&d->grop,x,y,w,h,btn_off); x++; y++; w-=2; h-=2;
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error button_install(struct widget *self) {
  g_error e;

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

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE;

  return sucess;
}

void button_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error button_set(struct widget *self,int property, glob data) {
  g_error e;

  switch (property) {

  case WP_SIDE:
    if ((data != S_LEFT) && (data != S_RIGHT) && (data != S_TOP) &&
	(data != S_BOTTOM)) return mkerror(ERRT_BADPARAM,
	"WP_SIDE param is not a valid side value (button)");
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC;
    self->in->next->flags &= SIDEMASK;
    self->in->next->flags |= ((sidet)data) | DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_ALIGN:
    if (data > AMAX) return mkerror(ERRT_BADPARAM,
		     "WP_ALIGN param is not a valid align value (button)");
    self->in->div->param.bitmap.align = (alignt) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
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
  /* This code for updating the button's appearance modifies
     the grops directly because it does not need a recalc, only
     a single-node redraw. Recalcs propagate like a virus, and
     require recreating grop-lists.
     Redraws don't spread to other nodes, and they are very fast.
  */

  switch (type) {

  case TRIGGER_ENTER:
    self->in->div->grop->next->next->param.c = btn_over;
    self->in->flags |= DIVNODE_NEED_REDRAW;
    self->dt->flags |= DIVTREE_NEED_REDRAW;   
    break;

  case TRIGGER_LEAVE:
    self->in->div->grop->next->next->param.c = btn_off;
    self->in->flags |= DIVNODE_NEED_REDRAW;
    self->dt->flags |= DIVTREE_NEED_REDRAW;    
    break;

  }
}

/* The End */
