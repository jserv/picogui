/* $Id: submenuitem.c,v 1.1 2002/10/23 06:17:26 micahjd Exp $
 *
 * submenuitem.c - a customized button, for menuitems opening submenus
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

g_error submenuitem_install(struct widget *self) {
  g_error e;

  /* Start with a button */
  e = button_install(self);
  errorcheck;

  /* Customize */
  widget_set(self, PG_WP_THOBJ_BUTTON,              PGTH_O_SUBMENUITEM);
  widget_set(self, PG_WP_THOBJ_BUTTON_ON,           PGTH_O_SUBMENUITEM_HILIGHT);
  widget_set(self, PG_WP_THOBJ_BUTTON_HILIGHT,      PGTH_O_SUBMENUITEM_HILIGHT);
  widget_set(self, PG_WP_THOBJ_BUTTON_ON_NOHILIGHT, PGTH_O_SUBMENUITEM_HILIGHT);

  /* We need extra events */
  widget_set(self,PG_WP_EXTDEVENTS,PG_EXEV_PNTR_UP | PG_EXEV_NOCLICK);

  /* Stack vertically */
  widget_set(self,PG_WP_SIDE,PG_S_TOP);

  return success;
}

/* The End */



