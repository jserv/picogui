/* $Id: indicator.c,v 1.17 2001/02/17 05:18:41 micahjd Exp $
 *
 * indicator.c - progress meter, battery bar, etc.
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

#define VALUE ((int)self->data)

void build_indicator(struct gropctxt *c,unsigned short state,struct widget *self) {
  exec_fillstyle(c,state,PGTH_P_BGFILL);

  /* Within the remaining space, figure out where the indicator is
     hilighted. */
  if (c->w > c->h)
    c->w = c->w*VALUE/100;
  else {
    int t;
    t = c->h*(100-VALUE)/100;
    c->y += t;
    c->h -= t;
  }

  exec_fillstyle(c,state,PGTH_P_OVERLAY);
}

void resize_indicator(struct widget *self) {
  self->in->split = theme_lookup(self->in->div->state,PGTH_P_WIDTH);
  self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
  self->dt->flags |= DIVTREE_NEED_RECALC;
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error indicator_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  self->in->split = 10;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_indicator;
  self->in->div->state = PGTH_O_INDICATOR;
  self->resize = &resize_indicator;

  return sucess;
}

void indicator_remove(struct widget *self) {
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error indicator_set(struct widget *self,int property, glob data) {
  switch (property) {

  case PG_WP_VALUE:
    if (data > 100) data = 100;
    if (data < 0) data = 0;
    VALUE = (int) data;
    div_setstate(self->in->div,PGTH_O_INDICATOR);
    break;

  case PG_WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,8);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;
    
  default:
    return mkerror(PG_ERRT_BADPARAM,9);
  }
  return sucess;
}

glob indicator_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_VALUE:
    return VALUE;

  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  }
  return 0;
}

/* The End */









