/* $Id: panel.c,v 1.14 2000/06/08 00:15:57 micahjd Exp $
 *
 * panel.c - Holder for applications
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

#include <widget.h>
#include <divtree.h>
#include <g_malloc.h>
#include <theme.h>

#define PANELBAR_SIZE 15

void panelbar(struct divnode *d) {
  int x,y,w,h;
  x=y=0; w=d->w; h=d->h;

  addelement(d,&current_theme[E_PANELBAR_BORDER],&x,&y,&w,&h);
  addelement(d,&current_theme[E_PANELBAR_FILL],&x,&y,&w,&h);

}

void panel(struct divnode *d) {
  int x,y,w,h;
  x=y=0; w=d->w; h=d->h;

  addelement(d,&current_theme[E_PANEL_BORDER],&x,&y,&w,&h);
  addelement(d,&current_theme[E_PANEL_FILL],&x,&y,&w,&h);
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error panel_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_TOP;
  self->in->split = PANELBAR_SIZE;

  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &panelbar;

  e = newdiv(&self->in->next,self);
  if (e.type != ERRT_NONE) return e;
  self->in->next->on_recalc = &panel;
  self->in->next->flags |= S_TOP;

  self->sub = &self->in->next->div;
  self->out = &self->in->next->next;

  return sucess;
}

void panel_remove(struct widget *self) {
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error panel_set(struct widget *self,int property, glob data) {
  switch (property) {

  case WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(ERRT_BADPARAM,
	"WP_SIDE param is not a valid side value (toolbar)");
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;
  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for toolbar");

  }
  return sucess;
}

glob panel_get(struct widget *self,int property) {
  switch (property) {
    /*
  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);
    */
  }
  return 0;
}

void panel_trigger(struct widget *self,long type,union trigparam *param) {
}

/* The End */




