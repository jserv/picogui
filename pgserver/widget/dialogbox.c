/* $Id: dialogbox.c,v 1.6 2002/11/06 08:03:51 micahjd Exp $
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
  struct divnode **title_location;   /* Insertion point for adding the title */
  struct widget *title;
  handle htitle;
};
#define DATA WIDGET_DATA(1,dialogboxdata)

g_error dialogbox_install(struct widget *self) {
  g_error e;
  struct divnode *interior;

  popup_install(self);
  WIDGET_ALLOC_DATA(1,dialogboxdata)

  DATA->title_location = self->sub;

  /* Make a divnode after the title to act as an insertion point for child widgets */
  e = newdiv(self->sub, self);
  errorcheck;
  interior = *self->sub;
  interior->flags |= PG_S_ALL;
  self->sub = &interior->div;

  return success;
}

void dialogbox_remove(struct widget *self) {
  if (DATA->title)
    handle_free(self->owner, DATA->htitle);
  popup_remove(self);
  g_free(DATA);
}

void dialogbox_resize(struct widget *self) {
}

g_error dialogbox_set(struct widget *self,int property, glob data) {
  g_error e;

  switch (property) {
  
  case PG_WP_TEXT:
    /* If we're rootless, just set the window title */
    if (VID(is_rootless)()) {
      struct pgstring *str;
      e = rdhandle((void**)&str, PG_TYPE_PGSTRING, self->owner, data);
      errorcheck;
      VID(window_set_title)(self->dt->display, str);
    }

    /* Otherwise, we'll use our own title widget */
    else {
      /* Need to create the title? */
      if (!DATA->title) {
	e = widget_create(&DATA->title, &DATA->htitle, PG_WIDGET_LABEL,
			  self->dt, self->container, self->owner);
	errorcheck;
	e = widget_attach(DATA->title, self->dt, DATA->title_location, self->h);
	errorcheck;
	e = widget_set(DATA->title, PG_WP_SIDE, PG_S_TOP);
	errorcheck;
	e = widget_set(DATA->title, PG_WP_TRANSPARENT, 0);
	errorcheck;
	e = widget_set(DATA->title, PG_WP_THOBJ, PGTH_O_LABEL_DLGTITLE);
	errorcheck;
      }			
      
      /* Set the title */
      e = widget_set(DATA->title, PG_WP_TEXT, data);
    }
    break;

  default:
    return popup_set(self,property,data);
  }
  return success;
}

glob dialogbox_get(struct widget *self,int property) {
  return popup_get(self,property);
}

void dialogbox_trigger(struct widget *self,s32 type,union trigparam *param) {
  popup_trigger(self,type,param);
}

/* The End */




