/* $Id: flatbutton.c,v 1.4 2002/01/06 09:23:00 micahjd Exp $
 *
 * flatbutton.c - another custom border, simply using an alternate theme object
 *                without the usual border
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

g_error flatbutton_install(struct widget *self) {
  g_error e;
  e = button_install(self);
  errorcheck;

  /* Customize */
  customize_button(self,PGTH_O_FLATBUTTON,PGTH_O_FLATBUTTON_ON,
		   PGTH_O_FLATBUTTON_HILIGHT,PGTH_O_FLATBUTTON_ON,NULL,NULL);
  return success;
}

/* The End */



