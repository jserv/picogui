/* $Id: scroll.c,v 1.26 2000/10/10 00:33:37 micahjd Exp $
 *
 * scroll.c - standard scroll indicator
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/widget.h>

/* Minimum # of milliseconds between scrolls. This is used to limit the
   scroll bar's frame rate so it doesn't 'lag' behind the mouse */
#define SCROLL_DELAY 50

/* # of milliseconds between scrolls when holding down the mouse */
#define SCROLLSPEED  50

/* A power of two to divide the scroll bar's height by to get the
   indicator's height */
#define HEIGHT_DIV 2

/* This widget has extra data we can't store in the divnodes themselves */
struct scrolldata {
  int on,over;
  int res;        /* Scroll bar's resolution - maximum value */
  int grab_offset;  /* The difference from the top of the indicator to
		       the point that was clicked */
  int release_delta;
  int value,old_value;
  handle binding;  /* If nonzero, this widget's PG_WP_SCROLL property will
		      be set in response to scrollbar movement instead of
		      an event being sent back to the client */

  unsigned long wait_tick;
};
#define DATA ((struct scrolldata *)(self->data))

void scrollbar(struct divnode *d) {
  int x,y,w,h;
  struct widget *self = d->owner;
  struct widget *wgt;
  
  /* Size ourselves to fit the widget we are bound to */
  if (!iserror(rdhandle((void **)&wgt,PG_TYPE_WIDGET,-1,
			DATA->binding)) && wgt) 
    DATA->res = widget_get(wgt,PG_WP_VIRTUALH) - wgt->in->h;

  /* Background for the whole bar */
  x=y=0; w=d->w; h=d->h;
  addelement(d,&current_theme[PG_E_SCROLLBAR_BORDER],&x,&y,&w,&h);
  addelement(d,&current_theme[PG_E_SCROLLBAR_FILL],&x,&y,&w,&h);

  /* Within the remaining space, figure out where the indicator goes */
  y += DATA->value * (h-(h>>HEIGHT_DIV)) / DATA->res;
  h = h>>HEIGHT_DIV;

  /* Add the indicator elements */
  addelement(d,&current_theme[PG_E_SCROLLIND_BORDER],&x,&y,&w,&h);
  addelement(d,&current_theme[PG_E_SCROLLIND_FILL],&x,&y,&w,&h);
  addelement(d,&current_theme[PG_E_SCROLLIND_OVERLAY],&x,&y,&w,&h);
}

/* When the value changes, send an event */
void scrollevent(struct widget *self) {
  struct widget *w;

  if (DATA->binding) {
    /* Send to a widget */
    if (!iserror(rdhandle((void **)&w,PG_TYPE_WIDGET,
			  -1,DATA->binding)) && w) 
      widget_set(w,PG_WP_SCROLL,DATA->value);
  }
  else {
    /* Send to a client */
    post_event(PG_WE_ACTIVATE,self,DATA->value,0);
  }
}

/* When value changes, update the grop coordinates */
void scrollupdate(struct widget *self) {
  int state;

  /* If we're busy rebuilding the grop list, don't bother poking
     at the individual nodes */
  if (self->in->div->grop_lock || !self->in->div->grop)
    return;
  
  /* Apply the current state to the elements */
  if (DATA->on)
    state = PG_STATE_ACTIVATE;
  else if (DATA->over)
    state = PG_STATE_HILIGHT;
  else
    state = PG_STATE_NORMAL;
  applystate(self->in->div->grop,
	     &current_theme[PG_E_SCROLLBAR_BORDER],state);
  applystate(self->in->div->grop->next,
	     &current_theme[PG_E_SCROLLBAR_FILL],state);
  applystate(self->in->div->grop->next->next,
	     &current_theme[PG_E_SCROLLIND_BORDER],state);
  applystate(self->in->div->grop->next->next->next,
	     &current_theme[PG_E_SCROLLIND_FILL],state);
  applystate(self->in->div->grop->next->next->next->next,
	     &current_theme[PG_E_SCROLLIND_OVERLAY],state);

  /* Border */
  self->in->div->grop->next->next->y = DATA->value * 
    (self->in->div->h-(self->in->div->h>>HEIGHT_DIV)) / DATA->res;
  /* Fill */
  self->in->div->grop->next->next->next->y = 
    self->in->div->grop->next->next->y + (
    (current_theme[PG_E_SCROLLIND_BORDER].width >= 0) ?
    current_theme[PG_E_SCROLLIND_BORDER].width : 0);
  /* Overlay */
  self->in->div->grop->next->next->next->next->y = 
    self->in->div->grop->next->next->next->y + (
    (current_theme[PG_E_SCROLLIND_FILL].width >= 0) ?
    current_theme[PG_E_SCROLLIND_FILL].width : 0);

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
  errorcheck;
  memset(self->data,0,sizeof(struct scrolldata));
  DATA->res = 100;    /* By default, make it compatible with percent */

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_RIGHT;
  self->in->split = HWG_SCROLL;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->on_recalc = &scrollbar;
  self->out = &self->in->next;

  self->trigger_mask = TRIGGER_DRAG | TRIGGER_ENTER | TRIGGER_LEAVE |
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_TIMER;

  return sucess;
}

void scroll_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error scroll_set(struct widget *self,int property, glob data) {
  struct widget *w;

  switch (property) {

  case PG_WP_VALUE:
    DATA->value = data;
    scrollupdate(self);
    break;

  case PG_WP_SIZE:
    DATA->res = data;
    scrollupdate(self);
    break;

  case PG_WP_BIND:
    if (!data) {
      DATA->binding = 0;
      break;
    }

    if (iserror(rdhandle((void **)&w,PG_TYPE_WIDGET,-1,data)) || !w) 
      return mkerror(PG_ERRT_HANDLE,17);

    /* Do a test run to see if the widget supports PG_WP_SCROLL */
    if (iserror(widget_set(w,PG_WP_SCROLL,DATA->value)))
      return mkerror(PG_ERRT_BADPARAM,18);
    DATA->binding = (handle) data;

    /* If applicable, turn off transparency in the widget */
    widget_set(w,PG_WP_TRANSPARENT,0);

    break;

  default:
    return mkerror(PG_ERRT_BADPARAM,19);
  }
  return sucess;
}

glob scroll_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_VALUE:
    return DATA->value;
   
  case PG_WP_SIZE:
    return DATA->res;

  }
  return 0;
}

void scroll_trigger(struct widget *self,long type,union trigparam *param) {
  unsigned long tick;

  switch (type) {

  case TRIGGER_ENTER:
    DATA->over=1;
    break;
    
  case TRIGGER_LEAVE:
    DATA->over=0;
    break;
    
  case TRIGGER_DOWN:
    if (param->mouse.chbtn==1) {
      DATA->grab_offset = param->mouse.y - self->in->div->y - 
	self->in->div->grop->next->next->y;

      if (DATA->grab_offset < 0)  
	DATA->release_delta = -10;
      else if (DATA->grab_offset > (self->in->div->h>>HEIGHT_DIV))
	DATA->release_delta = 10;
      else {
	DATA->on=1;
	DATA->release_delta = 0;
      }

      /* Set up a timer for repeating the scroll */
      if (DATA->release_delta != 0)
	install_timer(self,SCROLLSPEED);
    }
    break;

    /* Well, it gets the job done: */
    
  case TRIGGER_TIMER:
    if (DATA->release_delta == 0) return;
  case TRIGGER_UP:
    if (DATA->release_delta) {
      DATA->value += DATA->release_delta;

      if (DATA->value > DATA->res) {
	DATA->value = DATA->res;
	DATA->release_delta = 0;
      }
      if (DATA->value < 0) {
	DATA->value =   0;
	DATA->release_delta = 0;
      }
      
      scrollevent(self);
    }
  case TRIGGER_RELEASE:
    if (type==TRIGGER_TIMER) {
      install_timer(self,SCROLLSPEED);
      break;
    }
    DATA->on=0;
    DATA->release_delta = 0;
    break;

  case TRIGGER_DRAG:
    if (!DATA->on) return;

    /* Button 1 is being dragged through our widget. */
    DATA->value = (param->mouse.y - self->in->div->y - 
		   DATA->grab_offset) * DATA->res /
      (self->in->div->h - (self->in->div->h>>HEIGHT_DIV));

    if (DATA->value > DATA->res) DATA->value = DATA->res;
    if (DATA->value < 0) DATA->value =   0;

    /* Same old value... */
    if (DATA->old_value==DATA->value) return;

    /* If we haven't waited long enough since the last update,
       go away */
    tick = getticks();
    if (tick < DATA->wait_tick) break;
    DATA->wait_tick = tick + SCROLL_DELAY;

    /* Only store the old value if we're actually redrawing this
       time. Otherwise, scrolling to the top or bottom very fast
       will leave the last update somewhere in between */
    DATA->old_value = DATA->value;

    scrollevent(self);
    break;

  }

  scrollupdate(self);

  /* Use a precious update to animate the scrollbar */
  if (self->dt==dts->top) update();
}

/* The End */


