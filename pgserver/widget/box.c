/* $Id: box.c,v 1.1 2000/06/10 14:15:56 micahjd Exp $
 *
 * box.c - Generic container for laying out widgets
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

#define BOXMARGIN (HWG_MARGIN<<1)

/* The only data to store is a color, so just stick it in the data
   pointer... 
*/
#define BOXCOLOR ((devcolort)self->data)

void box(struct divnode *d) {
  struct widget *self = d->owner;

  grop_frame(&d->grop,BOXMARGIN-1,BOXMARGIN-1,
	     d->w-BOXMARGIN-2,d->h-BOXMARGIN-2,BOXCOLOR);
}

g_error box_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_TOP;
  self->in->split = HWG_BUTTON+(BOXMARGIN<<1);
  self->out = &self->in->next;

  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->split = BOXMARGIN;
  self->sub = &self->in->div->div;

  return sucess;
}

void box_remove(struct widget *self) {
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error box_set(struct widget *self,int property, glob data) {
  switch (property) {

  case WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(ERRT_BADPARAM,
	"WP_SIDE param is not a valid side value (toolbar)");
    redraw_bg(self);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_SIZE:
    if (data<0) data = 0;
    self->in->split = data;
    redraw_bg(self);
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BORDERCOLOR:
    BOXCOLOR = data;
    self->in->div->on_recalc = &box;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for boxx");

  }
  return sucess;
}

glob box_get(struct widget *self,int property) {
  switch (property) {

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_SIZE:
    return self->in->split;

  }
  return 0;
}

/* The End */




