/* $Id: scroll.c,v 1.47 2001/12/18 04:53:06 micahjd Exp $
 *
 * scroll.c - standard scroll indicator
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/common.h>
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
  unsigned long wait_tick;
  int thumbscale;
};
#define DATA ((struct scrolldata *)(self->data))

/* When value changes, update the grop coordinates */
void scrollupdate(struct widget *self) {
  /* Error checking for res <= 0.
   * FIXME: If res<=0 we should make the scroll bar disappear, because
   * there is nothing to scroll.
   */
  if (DATA->res < 0)
    DATA->res = 0;
  if (DATA->res == 0)
    return;

  self->in->div->ty = DATA->value * DATA->thumbscale / DATA->res;

  self->in->div->flags |= DIVNODE_NEED_REDRAW;
  self->dt->flags |= DIVTREE_NEED_REDRAW;
}

void build_scroll(struct gropctxt *c,unsigned short state,struct widget *self) {
  struct widget *wgt;
  s16 oldres;

  /* Size ourselves to fit the widget we are bound to */
  oldres = DATA->res;
  if (!iserror(rdhandle((void **)&wgt,PG_TYPE_WIDGET,-1,
			self->scrollbind)) && wgt && wgt->in->div)
    DATA->res = wgt->in->div->h - wgt->in->div->calch;
  if (DATA->res < 0)
    DATA->res = 0;
  if (DATA->value > DATA->res)
    scroll_set(self,PG_WP_VALUE,DATA->res);
  if ( (oldres && !DATA->res) ||
       (DATA->res && !oldres) )
    resizewidget(self);

  /* Background for the bar */
  exec_fillstyle(c,state,PGTH_P_BGFILL);

  /* The scrollbar thumb */
  DATA->thumbscale = (c->h-(c->h>>HEIGHT_DIV));
  c->h = c->h>>HEIGHT_DIV;
  c->defaultgropflags = PG_GROPF_TRANSLATE;
  exec_fillstyle(c,state,PGTH_P_OVERLAY);

  /* Update the scroll position */
  scrollupdate(self);
}

/* When the value changes, send an event */
void scrollevent(struct widget *self) {
  struct widget *w;

  if (self->scrollbind) {
    /* Send to a widget */
    if (!iserror(rdhandle((void **)&w,PG_TYPE_WIDGET,
			  -1,self->scrollbind)) && w) 
      widget_set(w,PG_WP_SCROLL,DATA->value);
  }
  else {
    /* Send to a client */
    post_event(PG_WE_ACTIVATE,self,DATA->value,0,NULL);
  }
}

void scroll_resize(struct widget *self) {
   if ((self->in->flags & PG_S_TOP) ||
       (self->in->flags & PG_S_BOTTOM)) {
      
      self->in->div->pw = 0;
      self->in->div->ph = DATA->res ? 
	theme_lookup(PGTH_O_SCROLL,PGTH_P_WIDTH) : 0;
   }
   else if ((self->in->flags & PG_S_LEFT) ||
	    (self->in->flags & PG_S_RIGHT)) {
      
      self->in->div->ph = 0;
      self->in->div->pw = DATA->res ?
	theme_lookup(PGTH_O_SCROLL,PGTH_P_WIDTH) : 0;
   }
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
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_scroll;
  self->in->div->state = PGTH_O_SCROLL;
  self->out = &self->in->next;
  self->trigger_mask = TRIGGER_DRAG | TRIGGER_ENTER | TRIGGER_LEAVE |
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_TIMER;

  return success;
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
   if (data < 0) {
      data = 0;
   }
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
    widget_set(w,PG_WP_SCROLL,0);

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

  case PG_WP_VALUE:
    return DATA->value;
   
  case PG_WP_SIZE:
    return DATA->res;

  }
  return 0;
}

void scroll_trigger(struct widget *self,long type,union trigparam *param) {
  unsigned long tick;
  bool force = 0;     /* Force div_setstate to redraw? */
   
  switch (type) {

  case TRIGGER_ENTER:
    DATA->over=1;
    break;
    
  case TRIGGER_LEAVE:
    DATA->over=0;
    break;
    
  case TRIGGER_DOWN:
    if (param->mouse.chbtn==1) {

      DATA->value++;

      DATA->grab_offset = param->mouse.y - self->in->div->y - 
	self->in->div->ty;
      
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
    else
      return;
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
      force = 1;
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

     /* If possible, use the input driver to see when we're behind
      * and skip a frame. Otherwise, just use a timer as a throttle */
     if (events_pending())
       return;
     tick = getticks();
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
    div_setstate(self->in->div,PGTH_O_SCROLL_ON,force);
  else if (DATA->over)
    div_setstate(self->in->div,PGTH_O_SCROLL_HILIGHT,force);
  else
    div_setstate(self->in->div,PGTH_O_SCROLL,force);
}

/* The End */


