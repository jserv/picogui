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

/* This widget has extra data we can't store in the divnodes themselves */

struct btndata {
  int on,over;
  int text;     /* If this is nonzero, this is a bitmap button instead
		   of a text button and the 'bitmap' param structure
		   will be used. */
};
#define DATA ((struct btndata *)(self->data))

void bitbutton(struct divnode *d) {
  int x,y,w,h;
  struct bitmap *bit;
  struct widget *self = d->owner;
  x = y = 0;
  w = d->w;
  h = d->h;

  grop_frame(&d->grop,x,y,w,h,btn_b1); x++; y++; w-=2; h-=2;
  grop_frame(&d->grop,x,y,w,h,
	     DATA->over ? btn_over : btn_b2); x++; y++; w-=2; h-=2;
  grop_rect(&d->grop,x,y,w,h,
	    DATA->on ? btn_on : btn_off); x++; y++; w-=2; h-=2;

  /* We need at least the main bitmap for alignment.  The mask
     bitmap is optional, but without it what's the point... */
  if (d->param.bitmap.bitmap && (rdhandle((void **) &bit,TYPE_BITMAP,-1,
      d->param.bitmap.bitmap).type==ERRT_NONE) && bit) {
    w = bit->w;
    h = bit->h;
    align(d,d->param.bitmap.align,&w,&h,&x,&y);

    /* AND the mask, then OR the bitmap. Yay for transparency effects! */
    grop_bitmap(&d->grop,x,y,w,h,d->param.bitmap.mask,LGOP_AND);
    grop_bitmap(&d->grop,x,y,w,h,d->param.bitmap.bitmap,LGOP_OR);
  }
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

  /* This code for updating the button's appearance modifies
     the grops directly because it does not need a recalc, only
     a single-node redraw. Recalcs propagate like a virus, and
     require recreating grop-lists.
     Redraws don't spread to other nodes, and they are very fast.
  */
  if (DATA->on)
    self->in->div->grop->next->next->param.c = btn_on;
  else
    self->in->div->grop->next->next->param.c = btn_off;
  if (DATA->over)
    self->in->div->grop->next->param.c = btn_over;
  else
    self->in->div->grop->next->param.c = btn_b2;
  self->in->div->flags |= DIVNODE_NEED_REDRAW;
  self->dt->flags |= DIVTREE_NEED_REDRAW;   
}

/* The End */


