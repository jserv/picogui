/* $Id: toolbar.c,v 1.6 2000/09/03 19:27:59 micahjd Exp $
 *
 * toolbar.c - container widget for buttons
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

void toolbar(struct divnode *d) {
  int x,y,w,h;
  x=y=0; w=d->w; h=d->h;

  addelement(d,&current_theme[E_TOOLBAR_BORDER],&x,&y,&w,&h);
  addelement(d,&current_theme[E_TOOLBAR_FILL],&x,&y,&w,&h);
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error toolbar_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= S_TOP;
  self->in->split = HWG_BUTTON+HWG_MARGIN*2;
  self->out = &self->in->next;

  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->on_recalc = &toolbar;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->split = HWG_MARGIN;
  self->sub = &self->in->div->div;

  return sucess;
}

void toolbar_remove(struct widget *self) {
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error toolbar_set(struct widget *self,int property, glob data) {
  switch (property) {

  case WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(ERRT_BADPARAM,15);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  default:
    return mkerror(ERRT_BADPARAM,16);
  }
  return sucess;
}

glob toolbar_get(struct widget *self,int property) {
  switch (property) {

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  }
  return 0;
}

/* The End */




