/* $Id$
 *
 * flatbutton.c - another custom border, simply using an alternate theme object
 *                without the usual border
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

g_error flatbutton_install(struct widget *self) {
  g_error e;
  e = button_install(self);
  errorcheck;

  /* Customize */
  widget_set(self, PG_WP_THOBJ_BUTTON,              PGTH_O_FLATBUTTON);
  widget_set(self, PG_WP_THOBJ_BUTTON_ON,           PGTH_O_FLATBUTTON_ON);
  widget_set(self, PG_WP_THOBJ_BUTTON_HILIGHT,      PGTH_O_FLATBUTTON_HILIGHT);
  widget_set(self, PG_WP_THOBJ_BUTTON_ON_NOHILIGHT, PGTH_O_FLATBUTTON_ON);

  return success;
}

/* The End */



