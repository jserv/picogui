/* $Id$
 *
 * scrollbox.c - A box widget that includes scrollbars. It also
 *               conglomerates properties and events as necessary
 *               to support multiple scrollbars, and scroll wheels
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

struct scrollboxdata {
  struct widget *scrollh, *scrollv, *box;
  handle hscrollh, hscrollv, hbox;
};
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(scrollboxdata)

g_error scrollbox_install(struct widget *self) {
  g_error e;
  struct divnode *interior;
  WIDGET_ALLOC_DATA(scrollboxdata);

  /* Main divnode */
  e = newdiv(&self->in, self);
  errorcheck;
  self->in->flags |= PG_S_ALL;

  /* Horizontal scrollbar */
  e = widget_create(&DATA->scrollh, &DATA->hscrollh, PG_WIDGET_SCROLL,
		    self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->scrollh, self->dt, &self->in->div, self->h);
  errorcheck;
  e = widget_set(DATA->scrollh, PG_WP_SIDE, PG_S_BOTTOM);
  errorcheck;

  /* Vertical scrollbar */
  e = widget_create(&DATA->scrollv, &DATA->hscrollv, PG_WIDGET_SCROLL, 
		    self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->scrollv, self->dt, DATA->scrollh->out, self->h);
  errorcheck;

  /* Box widget */
  e = widget_create(&DATA->box, &DATA->hbox, PG_WIDGET_BOX, 
		    self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->box, self->dt, DATA->scrollv->out, self->h);
  errorcheck;
  e = widget_set(DATA->box, PG_WP_SIDE, PG_S_ALL);
  errorcheck;

  /* Bind scrollbars to box */
  e = widget_set(DATA->scrollv, PG_WP_BIND, DATA->hbox);
  errorcheck;
  e = widget_set(DATA->scrollh, PG_WP_BIND, DATA->hbox);
  errorcheck;

  /* We proxy the widget properties out to the two scrollbars,
   * and we're the parent of both (so recalcs propagate to them)
   * so set our box's scrollbind to point back to us.
   */
  e = widget_set(DATA->box, PG_WP_BIND, self->h);
  errorcheck;

  /* Make a divnode after the title to act as an insertion point for child widgets */
  e = newdiv(DATA->box->sub, self);
  errorcheck;
  interior = *DATA->box->sub;
  interior->flags |= PG_S_ALL;
  self->sub = &interior->div;
  self->out = &self->in->next;

  self->trigger_mask = PG_TRIGGER_SCROLLWHEEL;

  return success;
}

void scrollbox_remove(struct widget *self) {
  handle_free(self->owner, DATA->hscrollv);
  handle_free(self->owner, DATA->hscrollh);
  handle_free(self->owner, DATA->hbox);
  g_free(DATA);
  r_divnode_free(self->in);
}

void scrollbox_resize(struct widget *self) {
}

g_error scrollbox_set(struct widget *self,int property, glob data) {
  switch (property) {
    
    /* Scrollbar properties */
  case PG_WP_SCROLL_Y:
    return widget_set(DATA->scrollv, PG_WP_VALUE, data);
  case PG_WP_SCROLL_X:
    return widget_set(DATA->scrollh, PG_WP_VALUE, data);

    /* Properties we should handle for this widget */
  case PG_WP_SIDE:
  case PG_WP_SIZE:
  case PG_WP_SIZEMODE:
    return mkerror(ERRT_PASS,0);

    /* Let the box widget handle the rest */
  default:
    return widget_set(DATA->box, property, data);
  }
}

glob scrollbox_get(struct widget *self,int property) {
  switch (property) {
    
    /* Scrollbar properties */
  case PG_WP_SCROLL_Y:
    return widget_get(DATA->scrollv, PG_WP_VALUE);
  case PG_WP_SCROLL_X:
    return widget_get(DATA->scrollh, PG_WP_VALUE);

    /* Let the box widget handle the rest */
  default:
    return box_get(DATA->box, property);
  }
}

/* Pass scroll wheel events to the scrollbar */
void scrollbox_trigger(struct widget *self,s32 type,union trigparam *param) {
  send_trigger(DATA->scrollv, type, param);
}

/* The End */




