/* $Id: simplemenu.c,v 1.2 2002/09/25 15:26:08 micahjd Exp $
 *
 * simplemenu.c - This is a type of popup widget that builds a simple popup menu from
 *                either a single string of pipe-separated items or an array
 *                of string handles set with the PG_WP_TEXT property.
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

void simplemenu_resize(struct widget *self) {
}

g_error simplemenu_install(struct widget *self) {
  self->out = &self->in->next;
  self->sub = &self->in->div->div;
  return success;
}

void simplemenu_remove(struct widget *self) {
}

g_error simplemenu_set(struct widget *self,int property, glob data) {
  return success;
}

glob simplemenu_get(struct widget *self,int property) {
  return 0;
}

void simplemenu_trigger(struct widget *self,s32 type,union trigparam *param) {
  popup_trigger(self,type,param);
}

/* The End */




