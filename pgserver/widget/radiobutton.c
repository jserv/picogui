/* $Id: radiobutton.c,v 1.3 2002/01/06 09:23:00 micahjd Exp $
 *
 * radiobutton.c - a customized button, used for "radio" buttons
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

g_error radiobutton_install(struct widget *self) {
  g_error e;

  /* Start with a button */
  e = button_install(self);
  errorcheck;

  /* Customize */
  customize_button(self,PGTH_O_RADIOBUTTON,PGTH_O_RADIOBUTTON_ON,
		   PGTH_O_RADIOBUTTON_HILIGHT,PGTH_O_RADIOBUTTON_ON_NOHILIGHT,
		   NULL,NULL);

  /* Use alternate click event handling to toggle */
  widget_set(self,PG_WP_EXTDEVENTS,PG_EXEV_TOGGLE | PG_EXEV_EXCLUSIVE);

  /* Stack vertically */
  widget_set(self,PG_WP_SIDE,PG_S_TOP);

  return success;
}

/* The End */



