/* $Id$
 *
 * label.c - A customized button for displaying static text and/or bitmaps
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

g_error label_install(struct widget *self) {
  g_error e;

  /* Start with a button */
  e = button_install(self);
  errorcheck;

  /* Customize */
  widget_set(self, PG_WP_THOBJ_BUTTON, PGTH_O_LABEL);

  /* Make it inert */
  widget_set(self,PG_WP_EXTDEVENTS,PG_EXEV_NO_HOTSPOT);
  widget_set(self,PG_WP_TRIGGERMASK,0);

  /* Stack vertically */
  widget_set(self,PG_WP_SIDE,PG_S_TOP);

  /* Transparent by default */
  widget_set(self,PG_WP_TRANSPARENT,1);

  return success;
}

/* The End */



