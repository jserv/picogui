/* $Id: indicator.c,v 1.31 2002/03/26 17:08:00 instinc Exp $
 *
 * indicator.c - progress meter, battery bar, etc.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#define VALUE ((long)self->data)

void build_indicator(struct gropctxt *c,u16 state,struct widget *self) {
  /* Set orientation */
  if (c->w > c->h)
    self->in->div->state = PGTH_O_INDICATOR_H;
  else
    self->in->div->state = PGTH_O_INDICATOR_V;

  exec_fillstyle(c,state,PGTH_P_BGFILL);

  /* Within the remaining space, figure out where the indicator is
     hilighted. */
  if (c->w > c->h)
    c->w = c->w*VALUE/100;
  else {
    int t;
    t = c->h*(100-VALUE)/100;
    c->y += t;
    c->h -= t;
  }

  exec_fillstyle(c,state,PGTH_P_OVERLAY);
}

void indicator_resize(struct widget *self) {
   if ((self->in->flags & PG_S_TOP) ||
       (self->in->flags & PG_S_BOTTOM)) {
 
     self->in->div->pw = 0;
     self->in->div->ph = theme_lookup(self->in->div->state,PGTH_P_WIDTH);
   }
   else if ((self->in->flags & PG_S_LEFT) ||
	    (self->in->flags & PG_S_RIGHT)) {
      
     self->in->div->ph = 0;
     self->in->div->pw = theme_lookup(self->in->div->state,PGTH_P_WIDTH);
   }
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error indicator_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_indicator;
  self->in->div->state = PGTH_O_INDICATOR;

  return success;
}

void indicator_remove(struct widget *self) {
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error indicator_set(struct widget *self,int property, glob data) {
  switch (property) {

  case PG_WP_VALUE:
    if (data > 100) data = 100;
    if (data < 0) data = 0;
    self->data = (void*) data;
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
    return VALUE;

  }
  return 0;
}

/* The End */









