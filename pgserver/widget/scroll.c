/* $Id: scroll.c,v 1.5 2000/05/05 22:28:07 micahjd Exp $
 *
 * scroll.c - standard scroll indicator
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
#include <theme.h>
#include <g_error.h>

void scrollbar(struct divnode *d) {
  int x,y,w,h;

  /* Background for the whole bar */
  x=y=0; w=d->w; h=d->h;
  addelement(&d->grop,&current_theme[E_SCROLLBAR_BORDER],&x,&y,&w,&h,1);
  addelement(&d->grop,&current_theme[E_SCROLLBAR_FILL],&x,&y,&w,&h,1);

  /* Within the remaining space, figure out where the indicator goes */
  y += d->param.i * (h-HWG_BUTTON) / 100;
  h = HWG_BUTTON;

  /* Add the indicator elements */
  addelement(&d->grop,&current_theme[E_SCROLLIND_BORDER],&x,&y,&w,&h,1);
  addelement(&d->grop,&current_theme[E_SCROLLIND_FILL],&x,&y,&w,&h,1);
  addelement(&d->grop,&current_theme[E_SCROLLIND_OVERLAY],&x,&y,&w,&h,1);
}

/* Nice and simple, the oncalc does most of the work */
g_error scroll_install(struct widget *self) {
  g_error e;
  
  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_RIGHT;
  self->in->split = HWG_SCROLL;
  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &scrollbar;
  self->out = &self->in->next;

  return sucess;
}

void scroll_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error scroll_set(struct widget *self,int property, glob data) {
  switch (property) {
  case WP_VALUE:
    if (data > 100) data = 100;
    if (data < 0) data = 0;
    self->in->div->param.i = data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for scroll");
  }
  return sucess;
}

glob scroll_get(struct widget *self,int property) {
  switch (property) {
  case WP_VALUE:
    return self->in->div->param.i;
  }
  return 0;
}

/* The End */
