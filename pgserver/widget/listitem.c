/* $Id: listitem.c,v 1.1 2001/08/03 14:56:11 micahjd Exp $
 *
 * listitem.c - a customized button, used for items in a listbox
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

g_error listitem_install(struct widget *self) {
  g_error e;

  /* Start with a button */
  e = button_install(self);
  errorcheck;

  /* Customize */
  customize_button(self,PGTH_O_LISTITEM,PGTH_O_LISTITEM_ON,
		   PGTH_O_LISTITEM_HILIGHT,NULL,NULL);

  /* We need extra events */
  widget_set(self,PG_WP_EXTDEVENTS,PG_EXEV_TOGGLE | PG_EXEV_EXCLUSIVE);

  /* Stack vertically */
  widget_set(self,PG_WP_SIDE,PG_S_TOP);

  return sucess;
}

/* The End */



