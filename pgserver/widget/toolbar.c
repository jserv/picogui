/* $Id: toolbar.c,v 1.15 2001/06/25 00:48:50 micahjd Exp $
 *
 * toolbar.c - container widget for buttons
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
#include <pgserver/appmgr.h>

void toolbar_resize(struct widget *self) {
  int m = theme_lookup(self->in->div->state,PGTH_P_MARGIN);

  self->in->div->ph = theme_lookup(PGTH_O_BUTTON,PGTH_P_HEIGHT)+(m<<1);
  self->in->div->pw = theme_lookup(PGTH_O_BUTTON,PGTH_P_WIDTH)+(m<<1);

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

  return sucess;
}

void toolbar_remove(struct widget *self) {

  if (in_shutdown) return;

  /* If this widget is the htbboundary, set it to the previous
     widget instead. To do this we have to iterate through the
     divtree. */
  if (wtbboundary == self) {
    struct divnode *p = dts->root->head;
    while (p) {
      if ((&p->next == self->where) && p->owner) {
	htbboundary = hlookup(p->owner,NULL);
	break;
      }
      p = p->next;
    }
  }

  r_divnode_free(self->in);
}

g_error toolbar_set(struct widget *self,int property, glob data) {
  return mkerror(ERRT_PASS,0);
}

glob toolbar_get(struct widget *self,int property) {
  switch (property) {
     
  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  }
  return 0;
}

/* The End */




