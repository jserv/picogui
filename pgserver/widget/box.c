/* $Id: box.c,v 1.15 2001/04/12 02:37:56 micahjd Exp $
 *
 * box.c - Generic container for holding a group of widgets. It's sizing and
 *         appearance are defined by the theme.
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

void resize_box(struct widget *self) {
   int m;
   
   /* Transparent boxen have the same margin as a button
    * (necessary for grids to look good) */
   if (self->in->div->build)
     m = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
   else
     m = theme_lookup(PGTH_O_BUTTON,PGTH_P_SPACING) >> 1;
   
   if (!self->sizelock) {
      if (self->in->flags & (PG_S_TOP | PG_S_BOTTOM))
	self->in->split = theme_lookup(PGTH_O_BUTTON,PGTH_P_HEIGHT)+(m<<1);
      else
	self->in->split = theme_lookup(PGTH_O_BUTTON,PGTH_P_WIDTH)+(m<<1);
   }
   
   self->in->div->split = m;
}

g_error box_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_BOX;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;
  self->resize = &resize_box;

  return sucess;
}

void box_remove(struct widget *self) {
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error box_set(struct widget *self,int property, glob data) {
   switch (property) {
      
    case PG_WP_TRANSPARENT:
      self->in->div->build = data ? NULL : (&build_bgfill_only);
      break;	

    case PG_WP_STATE:
      self->in->div->state = data;
      break;
      
    default:
      return mkerror(ERRT_PASS,0);
   }
   return sucess;
}

glob box_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case PG_WP_SIZE:
    resize_box(self);
    return self->in->split;

  }
  return 0;
}

/* The End */




