/* $Id: panel_nobar.c,v 1.2 2001/03/30 18:46:41 micahjd Exp $
 *
 * panel_nobar.c - A simple replacement for panel that doesn't allow resizing
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

void resize_panel(struct widget *self) {
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
  self->resize = &resize_panel;
   
  return sucess;
}

/**** Properties */

void panel_remove(struct widget *self) {
  if (!in_shutdown)
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
  return sucess;
}

glob panel_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case PG_WP_SIZE:
    return self->in->split;

  }
  return 0;
}

/* The End */




