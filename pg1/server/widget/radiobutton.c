/* $Id$
 *
 * radiobutton.c - a customized button, used for "radio" buttons
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

g_error radiobutton_install(struct widget *self) {
  g_error e;

  /* Start with a button */
  e = button_install(self);
  errorcheck;

  /* Customize themes */
  widget_set(self, PG_WP_THOBJ_BUTTON,              PGTH_O_RADIOBUTTON);
  widget_set(self, PG_WP_THOBJ_BUTTON_ON,           PGTH_O_RADIOBUTTON_ON);
  widget_set(self, PG_WP_THOBJ_BUTTON_HILIGHT,      PGTH_O_RADIOBUTTON_HILIGHT);
  widget_set(self, PG_WP_THOBJ_BUTTON_ON_NOHILIGHT, PGTH_O_RADIOBUTTON_ON_NOHILIGHT);

  /* Use alternate click event handling to toggle */
  widget_set(self,PG_WP_EXTDEVENTS,PG_EXEV_TOGGLE | PG_EXEV_EXCLUSIVE);

  /* Stack vertically */
  widget_set(self,PG_WP_SIDE,PG_S_TOP);

  return success;
}

/* The End */



