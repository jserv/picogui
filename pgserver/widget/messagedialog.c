/* $Id: messagedialog.c,v 1.1 2002/09/25 15:26:08 micahjd Exp $
 *
 * messagedialog.c - A type of dialog box that can display a message and get
 *                   a user's response to it.
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

struct messagedialogdata {
  struct widget *message, *toolbar;
  handle hmessage, htoolbar;
};
#define DATA WIDGET_DATA(2,messagedialogdata)

g_error messagedialog_install(struct widget *self) {
  g_error e;
  dialogbox_install(self);
  WIDGET_ALLOC_DATA(2,messagedialogdata)

  /* Create a toolbar widget attached to the bottom side */
  e = widget_create(&DATA->toolbar, PG_WIDGET_TOOLBAR, self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->toolbar, self->dt, self->sub, 0, self->owner);
  errorcheck;
  e = mkhandle(&DATA->htoolbar,PG_TYPE_WIDGET,self->owner,DATA->toolbar);
  errorcheck;
  e = widget_set(DATA->toolbar, PG_WP_SIDE, PG_S_BOTTOM);
  errorcheck;

  

  /* This is not a container widget any more */
  self->sub = NULL;

  return success;
}

void messagedialog_remove(struct widget *self) {
  handle_free(self->owner, DATA->htoolbar);
  dialogbox_remove(self);
  g_free(DATA);
}

void messagedialog_resize(struct widget *self) {
}

g_error messagedialog_set(struct widget *self,int property, glob data) {
  return success;
}

glob messagedialog_get(struct widget *self,int property) {
  return 0;
}

/* The End */
