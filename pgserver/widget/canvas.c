/* $Id: canvas.c,v 1.1 2001/01/19 06:27:54 micahjd Exp $
 *
 * canvas.c - canvas widget, allowing clients to manipulate the groplist
 * and recieve events directly, implementing graphical output or custom widgets
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/widget.h>
#include <picogui/canvas.h>

/* Set up divnodes */
g_error canvas_install(struct widget *self) {
  g_error e;

  /* Main split */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_ALL;

  /* Visible node */
  e = newdiv(&self->in->div,self);
  errorcheck;

  return sucess;
}

void canvas_remove(struct widget *self) {
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error canvas_set(struct widget *self,int property, glob data) {

  switch (property) {

  case PG_WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,31);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    redraw_bg(self);
    break;

   default:
    return mkerror(PG_ERRT_BADPARAM,37);
  }
  return sucess;
}

glob canvas_get(struct widget *self,int property) {
   return 0;
}

void canvas_trigger(struct widget *self,long type,union trigparam *param) {
}

/* The End */



