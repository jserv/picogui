/* $Id: scroll.c,v 1.7 2000/05/06 06:42:21 micahjd Exp $
 *
 * scroll.c - standard scroll indicator
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
#include <theme.h>
#include <g_error.h>

/* A power of two to divide the scroll bar's height by to get the
   indicator's height */
#define HEIGHT_DIV 2

/* This widget has extra data we can't store in the divnodes themselves */
struct scrolldata {
  int on,over;
  int res;        /* Scroll bar's resolution - maximum value */
};
#define DATA ((struct scrolldata *)(self->data))

void scrollbar(struct divnode *d) {
  int x,y,w,h;
  struct widget *self = d->owner;

  /* Background for the whole bar */
  x=y=0; w=d->w; h=d->h;
  addelement(&d->grop,&current_theme[E_SCROLLBAR_BORDER],&x,&y,&w,&h,1);
  addelement(&d->grop,&current_theme[E_SCROLLBAR_FILL],&x,&y,&w,&h,1);

  /* Within the remaining space, figure out where the indicator goes */
  y += d->param.i * (h-(h>>HEIGHT_DIV)) / DATA->res;
  h = h>>HEIGHT_DIV;

  /* Add the indicator elements */
  addelement(&d->grop,&current_theme[E_SCROLLIND_BORDER],&x,&y,&w,&h,1);
  addelement(&d->grop,&current_theme[E_SCROLLIND_FILL],&x,&y,&w,&h,1);
  addelement(&d->grop,&current_theme[E_SCROLLIND_OVERLAY],&x,&y,&w,&h,0);
}

/* When param.i changes, update the grop coordinates */
void scrollupdate(struct widget *self) {

  /* If we're busy rebuilding the grop list, don't bother poking
     at the individual nodes */
  if (self->in->div->grop_lock || !self->in->div->grop)
    return;
  
  /* Apply the current state to the elements */
  if (DATA->on && DATA->over) {
    applystate(self->in->div->grop,
	       &current_theme[E_SCROLLBAR_BORDER],
	       STATE_ACTIVATE);
    applystate(self->in->div->grop->next,
	       &current_theme[E_SCROLLBAR_FILL],
	       STATE_ACTIVATE);
    applystate(self->in->div->grop->next->next,
	       &current_theme[E_SCROLLIND_BORDER],
	       STATE_ACTIVATE);
    applystate(self->in->div->grop->next->next->next,
	       &current_theme[E_SCROLLIND_FILL],
	       STATE_ACTIVATE);
    applystate(self->in->div->grop->next->next->next->next,
	       &current_theme[E_SCROLLIND_OVERLAY],
	       STATE_ACTIVATE);
  }
  else if (DATA->over) {
    applystate(self->in->div->grop,
	       &current_theme[E_SCROLLBAR_BORDER],
	       STATE_HILIGHT);
    applystate(self->in->div->grop->next,
	       &current_theme[E_SCROLLBAR_FILL],
	       STATE_HILIGHT);
    applystate(self->in->div->grop->next->next,
	       &current_theme[E_SCROLLIND_BORDER],
	       STATE_HILIGHT);
    applystate(self->in->div->grop->next->next->next,
	       &current_theme[E_SCROLLIND_FILL],
	       STATE_HILIGHT);
    applystate(self->in->div->grop->next->next->next->next,
	       &current_theme[E_SCROLLIND_OVERLAY],
	       STATE_HILIGHT);
  }
  else {
    applystate(self->in->div->grop,
	       &current_theme[E_SCROLLBAR_BORDER],
	       STATE_NORMAL);
    applystate(self->in->div->grop->next,
	       &current_theme[E_SCROLLBAR_FILL],
	       STATE_NORMAL);
    applystate(self->in->div->grop->next->next,
	       &current_theme[E_SCROLLIND_BORDER],
	       STATE_NORMAL);
    applystate(self->in->div->grop->next->next->next,
	       &current_theme[E_SCROLLIND_FILL],
	       STATE_NORMAL);
    applystate(self->in->div->grop->next->next->next->next,
	       &current_theme[E_SCROLLIND_OVERLAY],
	       STATE_NORMAL);
  }

  self->in->div->grop->next->next->next->next->y = 
    (self->in->div->grop->next->next->next->y = 
     (self->in->div->grop->next->next->y = 
      (self->in->div->param.i * (self->in->div->h-
				 (self->in->div->h>>HEIGHT_DIV)) / DATA->res))
      + current_theme[E_SCROLLIND_BORDER].y) 
     + current_theme[E_SCROLLIND_FILL].y;

  self->in->div->flags |= DIVNODE_NEED_REDRAW;
  self->dt->flags |= DIVTREE_NEED_REDRAW;   
}


/* Here, the divnodes are set up.
   Groplist is created in the on_recalc handler (above).
   Events cause grop modification, but not a recalc.
*/
g_error scroll_install(struct widget *self) {
  g_error e;
  
  e = g_malloc(&self->data,sizeof(struct scrolldata));
  if (e.type != ERRT_NONE) return e;
  memset(self->data,0,sizeof(struct scrolldata));
  DATA->res = 100;    /* By default, make it compatible with percent */

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_RIGHT;
  self->in->split = HWG_SCROLL;
  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &scrollbar;
  self->out = &self->in->next;

  self->trigger_mask = TRIGGER_MOVE | TRIGGER_ENTER | TRIGGER_LEAVE |
    TRIGGER_UP | TRIGGER_DOWN;

  return sucess;
}

void scroll_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error scroll_set(struct widget *self,int property, glob data) {
  switch (property) {

  case WP_VALUE:
    self->in->div->param.i = data;
    scrollupdate(self);
    break;

  case WP_SIZE:
    DATA->res = data;
    scrollupdate(self);
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
   
  case WP_SIZE:
    return DATA->res;

  }
  return 0;
}

void scroll_trigger(struct widget *self,long type,union trigparam *param) {

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
      

      DATA->on=0;
    }
    break;

  case TRIGGER_RELEASE:
    if (param->mouse.chbtn==1)
      DATA->on=0;
    break;

  case TRIGGER_MOVE:
    if (param->mouse.btn!=1) break;
    /* Button 1 is being dragged through our widget. */
    self->in->div->param.i = (param->mouse.y - self->in->div->y) * 100 /
      (self->in->div->h - (self->in->div->h>>HEIGHT_DIV));

    if (self->in->div->param.i > DATA->res) self->in->div->param.i = DATA->res;
    if (self->in->div->param.i < 0) self->in->div->param.i =   0;

    post_event(WE_ACTIVATE,self,self->in->div->param.i);
    break;

  }

  scrollupdate(self);
}

/* The End */


