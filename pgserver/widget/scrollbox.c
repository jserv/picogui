/* $Id: scrollbox.c,v 1.1 2002/09/28 04:06:56 micahjd Exp $
 *
 * scrollbox.c - A box widget that includes scrollbars
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

struct scrollboxdata {
  struct widget *scrollh, *scrollv, *box;
  handle hscrollh, hscrollv, hbox;
};
#define DATA WIDGET_DATA(0,scrollboxdata)

g_error scrollbox_install(struct widget *self) {
  g_error e;
  WIDGET_ALLOC_DATA(0,scrollboxdata)

  /* Main divnode */
  e = newdiv(&self->in, self);
  errorcheck;
  self->in->flags |= PG_S_ALL;

  /* Horizontal scrollbar */
  e = widget_create(&DATA->scrollh, PG_WIDGET_SCROLL, self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->scrollh, self->dt, &self->in->div, 0, self->owner);
  errorcheck;
  e = mkhandle(&DATA->hscrollh,PG_TYPE_WIDGET,self->owner,DATA->scrollh);
  errorcheck;
  e = widget_set(DATA->scrollh, PG_WP_SIDE, PG_S_BOTTOM);
  errorcheck;

  /* Vertical scrollbar */
  e = widget_create(&DATA->scrollv, PG_WIDGET_SCROLL, self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->scrollv, self->dt, DATA->scrollh->out, 0, self->owner);
  errorcheck;
  e = mkhandle(&DATA->hscrollv,PG_TYPE_WIDGET,self->owner,DATA->scrollv);
  errorcheck;

  /* Box widget */
  e = widget_create(&DATA->box, PG_WIDGET_BOX, self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->box, self->dt, DATA->scrollv->out, 0, self->owner);
  errorcheck;
  e = mkhandle(&DATA->hbox,PG_TYPE_WIDGET,self->owner,DATA->box);
  errorcheck;
  e = widget_set(DATA->box, PG_WP_SIDE, PG_S_ALL);
  errorcheck;

  /* Bind scrollbars to box */
  e = widget_set(DATA->scrollv, PG_WP_BIND, DATA->hbox);
  errorcheck;
  e = widget_set(DATA->scrollh, PG_WP_BIND, DATA->hbox);
  errorcheck;

  /* Insertion points */
  self->out = &self->in->next;
  self->sub = DATA->box->sub;

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
    return widget_get(DATA->box, property);
  }
}

/* The End */




