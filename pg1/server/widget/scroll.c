/* $Id$
 *
 * scroll.c - standard scroll indicator
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <string.h>
#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/input.h>
#include <pgserver/timer.h>

#ifdef DEBUG_WIDGET
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

/* FIXME: These timing values shouldn't be hardcoded, this should
 *        adapt to whatever frame rate it can get and try to
 *        achieve a constant speed
 */

/* Minimum # of milliseconds between scrolls. This is used to limit the
   scroll bar's frame rate so it doesn't 'lag' behind the mouse */
#define SCROLL_DELAY 15

/* # of milliseconds between scrolls when holding down the mouse */
#define SCROLLSPEED  15

/* Amount of pixels per redraw while holding down the mouse */
#define SCROLLAMOUNT 5

/* A power of two to divide the scroll bar's height by to get the
   indicator's height */
#define HEIGHT_DIV 2

/* Minimum thumb size */
#define MIN_THUMB 10

/* This widget has extra data we can't store in the divnodes themselves */
struct scrolldata {
  int horizontal;
  int on,over;
  int res;        /* Scroll bar's resolution - maximum value */
  int grab_offset;  /* The difference from the top of the indicator to
		       the point that was clicked */
  int release_delta;
  int value,old_value;
  u32 wait_tick;
  int thumbscale;
  int thumbsize;
};
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(scrolldata)

#ifndef MAX
#define MAX(a, b)  (a > b ? a : b)
#endif /* max */

void scrollevent(struct widget *self);

/* When value changes, update the grop coordinates */
void scrollupdate(struct widget *self) {

  if(DATA->horizontal){
    if (DATA->res <= 0)
      self->in->div->translation.x = 0;
    else
      self->in->div->translation.x = DATA->value * DATA->thumbscale / DATA->res;
  }
  else {
    if (DATA->res <= 0)
      self->in->div->translation.y = 0;
    else
      self->in->div->translation.y = DATA->value * DATA->thumbscale / DATA->res;
  }

  self->in->div->flags |= DIVNODE_NEED_REDRAW;
  self->dt->flags |= DIVTREE_NEED_REDRAW;

}

void build_scroll(struct gropctxt *c,u16 state,struct widget *self) {
  struct widget *wgt;
  s16 oldres;
  /* Size ourselves to fit the widget we are bound to
   */

  /* The scrollbar thumb size */
  if(DATA->horizontal)
      DATA->thumbsize = c->r.w>>HEIGHT_DIV;
  else 
      DATA->thumbsize = c->r.h>>HEIGHT_DIV;
  
  oldres = DATA->res;
  if (!iserror(rdhandle((void **)&wgt,PG_TYPE_WIDGET,-1,
			self->scrollbind)) && wgt && wgt->in->div) {
    
      if (DATA->horizontal) {
          DATA->res = wgt->in->child.w - wgt->in->r.w;
          if (wgt->in->child.w)
              DATA->thumbsize = (c->r.w * c->r.w) / wgt->in->child.w;
      } else {
          DATA->res = wgt->in->child.h - wgt->in->r.h;
          if (wgt->in->child.h)
              DATA->thumbsize = (c->r.h * c->r.h) / wgt->in->child.h;
      }
    /* Bounds/sanity checking.. */
    if (DATA->res < 0)
      DATA->res = 0;
    if (DATA->value > DATA->res) {
      DATA->value = DATA->res;
      scrollevent(self);
    }

    DBG("Resizing to res %d, div->h = %d, calch = %d, oldres = %d, value = %d, t = (%d,%d)\n",
	DATA->res, wgt->in->div->r.h, wgt->in->div->calc.h,oldres, DATA->value,
	self->in->div->translation.x, self->in->div->translation.y);
  }

  if ( (oldres==0) != (DATA->res==0) ) {
    static int lock = 0;
    DBG("Scroll bar show/hide, residewidget()\n");

    /* Jumpstart layout engine here to account for the change 
     *
     * Yeah, this code does appear to be a little magical..
     * Here's a bit of a description- at this point in time, the layout
     * engine will have already finished processing self->in. Since we're
     * changing its split, we need to reset the calc flags and re-run
     * the layout engine (divnode_recalc). This doesn't actually
     * run the layout engine twice, since the flags will be reset correctly.
     * Then, we need to restart div_rebuild (this function!) with the new
     * coordinates.
     *
     * Note that it may be possible for the layout engine to oscillate
     * between scrollbar states, probably due to an inconsistency in some
     * widget's preferred size calculations. We use the 'lock'
     * variable here to prevent infinite recursion in that case. Note that
     * this probably shouldn't happen, so if we can, bug some developer about it.
     */
    if (lock) {
      //guru("Something's causing layout engine oscillation via the scroll widget!");
      return;
    }
    lock++;
    resizewidget(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    divnode_recalc(&self->in,NULL);
    div_rebuild(self->in->div);
    lock = 0;
    return;
  }

  /* Background for the bar */
  exec_fillstyle(c,state,PGTH_P_BGFILL);
  
  /* Finish scrollbar thumb */
  DATA->thumbsize = MAX(DATA->thumbsize, MIN_THUMB);
  if(DATA->horizontal) {
      DATA->thumbscale = c->r.w - DATA->thumbsize;
      c->r.w = DATA->thumbsize;
  } else {
      DATA->thumbscale = c->r.h - DATA->thumbsize;
      c->r.h = DATA->thumbsize;
  }
  c->defaultgropflags = PG_GROPF_TRANSLATE;
  exec_fillstyle(c,state,PGTH_P_OVERLAY);

  /* Update the scroll position */
  scrollupdate(self);
}

/* When the value changes, send an event */
void scrollevent(struct widget *self) {
  struct widget *w;

  DBG("value = %d\n",DATA->value);

  if (self->scrollbind) {
    /* Send to a widget */
    if (!iserror(rdhandle((void **)&w,PG_TYPE_WIDGET,
			  -1,self->scrollbind)) && w) 
      widget_set(w,DATA->horizontal ? PG_WP_SCROLL_X : PG_WP_SCROLL_Y,DATA->value);
  }
  else {
    /* Send to a client */
    post_event(PG_WE_ACTIVATE,self,DATA->value,0,NULL);
  }
}

void scroll_resize(struct widget *self) {
  if (DATA->horizontal) {
    self->in->div->preferred.w = 0;
    self->in->split = self->in->div->preferred.h = DATA->res ? 
      theme_lookup(self->in->div->state,PGTH_P_WIDTH) : 0;
  }
  else {
    self->in->div->preferred.h = 0;
    self->in->split = self->in->div->preferred.w = DATA->res ?
      theme_lookup(self->in->div->state,PGTH_P_WIDTH) : 0;
  }
}

/* Here, the divnodes are set up.
   Groplist is created in the on_recalc handler (above).
   Events cause grop modification, but not a recalc.
*/
g_error scroll_install(struct widget *self) {
  g_error e;
  
  WIDGET_ALLOC_DATA(scrolldata);
  DATA->res = 100;    /* By default, make it compatible with percent */

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_RIGHT;
  self->in->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_scroll;
  self->in->div->state = PGTH_O_SCROLL_V;
  self->out = &self->in->next;
  self->trigger_mask = PG_TRIGGER_DRAG | PG_TRIGGER_ENTER | PG_TRIGGER_LEAVE |
    PG_TRIGGER_UP | PG_TRIGGER_DOWN | PG_TRIGGER_RELEASE | PG_TRIGGER_TIMER |
    PG_TRIGGER_SCROLLWHEEL;

  return success;
}

void scroll_remove(struct widget *self) {
  g_free(DATA);
  r_divnode_free(self->in);
}

g_error scroll_set(struct widget *self,int property, glob data) {
  struct widget *w;

  switch (property) {

    /* Intercept PG_WP_SIDE so we can set our horizontality flag.
     * We can't determine this by our width and height because if we
     * are resized such that our height is smaller than width but this really
     * is a vertical scrollbar, it will switch to horizontal and not switch back!
     */
  case PG_WP_SIDE:
    DATA->horizontal = (data == PG_S_TOP) || (data == PG_S_BOTTOM);
    self->in->div->state = DATA->horizontal ? PGTH_O_SCROLL_H : PGTH_O_SCROLL_V;
    return mkerror(ERRT_PASS,0);

    /* VALUE is the native WP for this widget, but also accept the others
     * for compatibility with the object being scrolled, or the scrollbox
     */
  case PG_WP_VALUE:
  case PG_WP_SCROLL_X:
  case PG_WP_SCROLL_Y:
    if (data < 0)
      data = 0;
    if (data > DATA->res)
      data = DATA->res;
    DATA->value = data;
    scrollevent(self);
    scrollupdate(self);
    div_setstate(self->in->div,self->in->div->state,1);
    break;
    
  case PG_WP_SIZE:
    DATA->res = data;
    scrollupdate(self);
    break;
    
  case PG_WP_BIND:
    if (!data) {
      self->scrollbind = 0;
      break;
    }

    if (iserror(rdhandle((void **)&w,PG_TYPE_WIDGET,-1,data)) || !w) 
      return mkerror(PG_ERRT_HANDLE,17);

    self->scrollbind = (handle) data;
 
    /* If applicable, turn off transparency in the widget */
    widget_set(w,PG_WP_TRANSPARENT,0);

    /* Reset scrolling */
    widget_set(w,DATA->horizontal ? PG_WP_SCROLL_X : PG_WP_SCROLL_Y,0);

    /* Set the other widget's binding, as long as it's not a scrollbar!
     * (That wouldn't make much sense, plus it would cause an infinite loop)
     */
    if (w->type != PG_WIDGET_SCROLL)
      widget_set(w,PG_WP_BIND,hlookup(self,NULL));

    break;

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob scroll_get(struct widget *self,int property) {
  switch (property) {

    /* VALUE is the native WP for this widget, but also accept the others
     * for compatibility with the object being scrolled, or the scrollbox
     */
  case PG_WP_VALUE:
  case PG_WP_SCROLL_X:
  case PG_WP_SCROLL_Y:
    return DATA->value;
   
  case PG_WP_SIZE:
    return DATA->res;

  }
  return widget_base_get(self,property);
}

void scroll_trigger(struct widget *self,s32 type,union trigparam *param) {
  u32 tick;
  bool force = 0;     /* Force div_setstate to redraw? */

  switch (type) {

  case PG_TRIGGER_ENTER:
    DATA->over=1;
    break;
    
  case PG_TRIGGER_LEAVE:
    DATA->over=0;
    break;
    
  case PG_TRIGGER_SCROLLWHEEL:
    widget_set(self, PG_WP_VALUE, DATA->value + param->mouse.y);
    break;

  case PG_TRIGGER_DOWN:
    if (param->mouse.chbtn==1) {

      DATA->value++;

      if (DATA->horizontal) {
	DATA->grab_offset = param->mouse.x - self->in->div->r.x -
	  self->in->div->translation.x;
      }
      else {
	DATA->grab_offset = param->mouse.y - self->in->div->r.y -
	  self->in->div->translation.y;
      }
      
      if (DATA->grab_offset < 0)  
	DATA->release_delta = -SCROLLAMOUNT;
      else if (DATA->grab_offset > DATA->thumbsize)
	DATA->release_delta = SCROLLAMOUNT;
      else {
	DATA->on=1;
	DATA->release_delta = 0;
      }
      
      /* Set up a timer for repeating the scroll */
      if (DATA->release_delta != 0)
	install_timer(self,SCROLLSPEED);
    }
    else
      return;
    break;

    /* Well, it gets the job done: */
    
  case PG_TRIGGER_TIMER:
    if (DATA->release_delta == 0) return;
  case PG_TRIGGER_UP:
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
      force = 1;
    }
  case PG_TRIGGER_RELEASE:
    if (type==PG_TRIGGER_TIMER) {
      install_timer(self,SCROLLSPEED);
      break;
    }
    DATA->on=0;
    DATA->release_delta = 0;
    break;

  case PG_TRIGGER_DRAG:
    if (!DATA->on) return;

    /* Button 1 is being dragged through our widget. */
    if (DATA->horizontal) {
      DATA->value = (param->mouse.x - self->in->div->r.x -
		     DATA->grab_offset) * DATA->res /
	(self->in->div->r.w - DATA->thumbsize);
    }
    else {
      DATA->value = (param->mouse.y - self->in->div->r.y -
		     DATA->grab_offset) * DATA->res /
          (self->in->div->r.h - DATA->thumbsize);
    }

    if (DATA->value > DATA->res) DATA->value = DATA->res;
    if (DATA->value < 0) DATA->value =   0;

    /* Same old value... */
    if (DATA->old_value==DATA->value) return;

     /* If possible, use the input driver to see when we're behind
      * and skip a frame. Otherwise, just use a timer as a throttle */
     if (events_pending())
       return;
     tick = os_getticks();
     if (tick < DATA->wait_tick) return;
     DATA->wait_tick = tick + SCROLL_DELAY;
     
     /* Only store the old value if we're actually redrawing this
       time. Otherwise, scrolling to the top or bottom very fast
       will leave the last update somewhere in between */
    DATA->old_value = DATA->value;

    scrollevent(self);
    force = 1;
    break;
  }

  scrollupdate(self);

  /* Change State */
  if (DATA->on)
    div_setstate(self->in->div,DATA->horizontal ? PGTH_O_SCROLL_H_ON : PGTH_O_SCROLL_V_ON,force);
  else if (DATA->over)
    div_setstate(self->in->div,DATA->horizontal ? PGTH_O_SCROLL_H_HILIGHT : PGTH_O_SCROLL_V_HILIGHT,force);
  else
    div_setstate(self->in->div,DATA->horizontal ? PGTH_O_SCROLL_H : PGTH_O_SCROLL_V,force);
}

/* The End */


