/* $Id: dialogbox.c,v 1.4 2002/10/07 07:08:09 micahjd Exp $
 *
 * dialogbox.c - The dialogbox is a type of popup widget that is always
 *               automatically sized, and has a title
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

struct dialogboxdata {
  struct widget *title;
  handle htitle;
};
#define DATA WIDGET_DATA(1,dialogboxdata)

g_error dialogbox_install(struct widget *self) {
  g_error e;
  struct divnode *interior;

  popup_install(self);
  WIDGET_ALLOC_DATA(1,dialogboxdata)

  /* Create a label widget for our DATA->title, attached to our former child attachment point */
  e = widget_create(&DATA->title, &DATA->htitle, PG_WIDGET_LABEL,
		    self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->title, self->dt, self->sub, self->h);
  errorcheck;
  e = widget_set(DATA->title, PG_WP_SIDE, PG_S_TOP);
  errorcheck;
  e = widget_set(DATA->title, PG_WP_TRANSPARENT, 0);
  errorcheck;
  e = widget_set(DATA->title, PG_WP_THOBJ, PGTH_O_LABEL_DLGTITLE);
  errorcheck;

  /* Make a divnode after the title to act as an insertion point for child widgets */
  e = newdiv(DATA->title->out, self);
  errorcheck;
  interior = *DATA->title->out;
  interior->flags |= PG_S_ALL;
  self->sub = &interior->div;

  return success;
}

void dialogbox_remove(struct widget *self) {
  handle_free(self->owner, DATA->htitle);
  popup_remove(self);
  g_free(DATA);
}

void dialogbox_resize(struct widget *self) {
}

g_error dialogbox_set(struct widget *self,int property, glob data) {
  switch (property) {
  
  case PG_WP_TEXT:
    return widget_set(DATA->title, PG_WP_TEXT, data);

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob dialogbox_get(struct widget *self,int property) {
  return widget_base_get(self,property);
}

void dialogbox_trigger(struct widget *self,s32 type,union trigparam *param) {
  popup_trigger(self,type,param);
}

/* The End */




