/* $Id$
 *
 * indicator.c - progress meter, battery bar, etc.
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

#include <pgserver/common.h>
#include <pgserver/widget.h>

struct indicatordata {
  long value;
  pgcolor color;
};
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(indicatordata)

void build_indicator(struct gropctxt *c,u16 state,struct widget *self) {
  /* Set orientation */
  if (c->r.w > c->r.h)
    state = c->owner->state = PGTH_O_INDICATOR_H;
  else
    state = c->owner->state = PGTH_O_INDICATOR_V;

  exec_fillstyle(c,state,PGTH_P_BGFILL);

  /* Within the remaining space, figure out where the indicator is
     hilighted. */
  if (c->r.w > c->r.h)
    c->r.w = c->r.w * DATA->value/100;
  else {
    int t;
    t = c->r.h * (100-DATA->value)/100;
    c->r.y += t;
    c->r.h -= t;
  }

  exec_fillstyle(c,state,PGTH_P_OVERLAY);
}

void indicator_resize(struct widget *self) {
   if ((self->in->flags & PG_S_TOP) ||
       (self->in->flags & PG_S_BOTTOM)) {
 
     self->in->div->preferred.w = 0;
     self->in->div->preferred.h = theme_lookup(self->in->div->state,PGTH_P_WIDTH);
   }
   else if ((self->in->flags & PG_S_LEFT) ||
	    (self->in->flags & PG_S_RIGHT)) {
      
     self->in->div->preferred.h = 0;
     self->in->div->preferred.w = theme_lookup(self->in->div->state,PGTH_P_WIDTH);
   }
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error indicator_install(struct widget *self) {
  g_error e;

  WIDGET_ALLOC_DATA(indicatordata);

  DATA->color = theme_lookup(PGTH_O_INDICATOR, PGTH_P_FGCOLOR);

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_indicator;
  self->in->div->state = PGTH_O_INDICATOR;

  self->sub = &self->in->div->div;

  return success;
}

void indicator_remove(struct widget *self) {
  r_divnode_free(self->in);
  g_free(DATA);
}

g_error indicator_set(struct widget *self,int property, glob data) {
  switch (property) {

  case PG_WP_VALUE:
    if (data > 100) data = 100;
    if (data < 0) data = 0;
    DATA->value = data;
    set_widget_rebuild(self);
    break;

   case PG_WP_COLOR:
    DATA->color = (pgcolor) data;
    set_widget_rebuild(self);
    break;

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob indicator_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_VALUE:
    return DATA->value;

  case PG_WP_COLOR:
    return (glob) DATA->color;

  }
  return widget_base_get(self,property);
}

/* The End */









