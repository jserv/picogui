/* $Id$
 *
 * toolbar.c - container widget for buttons
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/appmgr.h>

void build_nothing(struct gropctxt *c,unsigned short state, struct widget *self) {
}

void toolbar_resize(struct widget *self) {
  int m;

  if (self->in->div->build == &build_bgfill_only)
    m = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
  else
    m = 0;

  /* minimum size */
  self->in->div->preferred.h = theme_lookup(self->in->div->state,PGTH_P_HEIGHT);
  self->in->div->preferred.w = theme_lookup(self->in->div->state,PGTH_P_WIDTH);

  /* Manually set border size */
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->div->split = m;
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error toolbar_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  self->out = &self->in->next;

  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_TOOLBAR;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->sub = &self->in->div->div;

  return success;
}

void toolbar_remove(struct widget *self) {
  /* If this is an application toolbar, and there is a popup in the 
   * nontoolbar area only, redraw the screen */
  if (self->isroot && popup_toolbar_passthrough()) {
    struct divtree *tree;

    for (tree=dts->top;tree;tree=tree->next) {
      tree->head->flags |= DIVNODE_NEED_RECALC;
      tree->flags |= DIVTREE_NEED_RECALC | DIVTREE_ALL_REDRAW;
    }
  }

  r_divnode_free(self->in);
}

g_error toolbar_set(struct widget *self,int property, glob data) {
   switch (property) {
      
    case PG_WP_TRANSPARENT:
      self->in->div->build = data ? (&build_nothing) : (&build_bgfill_only);
      resizewidget(self);
      set_widget_rebuild(self);
      break;	

    default:
      return mkerror(ERRT_PASS,0);
   }
   return success;
}

glob toolbar_get(struct widget *self,int property) {
  return widget_base_get(self,property);
}

/* The End */




