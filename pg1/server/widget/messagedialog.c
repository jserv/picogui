/* $Id$
 *
 * messagedialog.c - A type of dialog box that can display a message and get
 *                   a user's response to it.
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

struct messagedialogdata {
  struct widget *message, *toolbar;
  handle hmessage, htoolbar;
};
#define WIDGET_SUBCLASS 2
#define DATA WIDGET_DATA(messagedialogdata)

g_error messagedialog_install(struct widget *self) {
  g_error e;
  WIDGET_INSTALL_PARENT(PG_WIDGET_DIALOGBOX);
  WIDGET_ALLOC_DATA(messagedialogdata);

  /* Create a toolbar widget attached to the bottom side */
  e = widget_create(&DATA->toolbar, &DATA->htoolbar, PG_WIDGET_TOOLBAR, self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->toolbar, self->dt, self->sub, 0);
  errorcheck;
  e = widget_set(DATA->toolbar, PG_WP_SIDE, PG_S_BOTTOM);
  errorcheck;

  /* This is not a container widget any more */
  self->sub = NULL;

  return success;
}

void messagedialog_remove(struct widget *self) {
  handle_free(self->owner, DATA->htoolbar);
  g_free(DATA);
  WIDGET_REMOVE_PARENT;
}

void messagedialog_resize(struct widget *self) {
  WIDGET_PARENT->resize(self);
}

g_error messagedialog_set(struct widget *self,int property, glob data) {
  return success;
}

glob messagedialog_get(struct widget *self,int property) {
  return widget_base_get(self,property);
}

/* The End */
