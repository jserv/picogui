/* $Id: field.c,v 1.60 2002/09/27 02:59:27 micahjd Exp $
 *
 * field.c - A single line text editor, based (hybrid, not subclassed) on textbox
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

g_error field_install(struct widget *self) {
  g_error e;

  e = textbox_install(self);
  errorcheck;

  widget_set(self, PG_WP_MULTILINE, 0);
  widget_set(self, PG_WP_THOBJ,     PGTH_O_FIELD);

  return success;
}

/* The End */



