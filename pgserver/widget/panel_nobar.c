/* $Id: panel_nobar.c,v 1.8 2002/09/28 10:58:10 micahjd Exp $
 *
 * panel_nobar.c - A simple replacement for panel that doesn't allow resizing
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
#include <pgserver/appmgr.h>

void panel_resize(struct widget *self) {
   self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
   self->in->div->split = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
}

g_error panel_install(struct widget *self) {
  g_error e;

  /* This split determines the size of the main panel area */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;

  /* This draws the panel background  */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_PANEL;

  self->sub = &self->in->div->div;
  self->out = &self->in->next;
   
  return success;
}

/**** Properties */

void panel_remove(struct widget *self) {
    r_divnode_free(self->in);
}

g_error panel_set(struct widget *self,int property, glob data) {
  char *str;

  switch (property) {

  case PG_WP_TEXT:
    break;

  default:
    return mkerror(ERRT_PASS,0);

  }
  return success;
}

glob panel_get(struct widget *self,int property) {
  return widget_base_get(self,property);
}

/* The End */




