/* $Id$
 *
 * background.c - an internal widget for drawing the screen background
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

void build_background(struct gropctxt *c,unsigned short state,struct widget *self) {
  
  /* Tweak the coordinates a bit so that the background is relative
     to the screen, not the clipping rectangle imposed by the divnode. */  
  c->r.x = -self->in->div->r.x;
  c->r.y = -self->in->div->r.y;
  c->r.w = vid->lxres;
  c->r.h = vid->lyres;

  /* As normal... */
  exec_fillstyle(c,state,PGTH_P_BGFILL);
}

g_error background_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_ALL;

  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_background;
  self->in->div->state = PGTH_O_BACKGROUND;

  self->out = &self->in->next;
   
  return success;
}

void background_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error background_set(struct widget *self,int property, glob data) {
  return mkerror(ERRT_PASS,0);
}

glob background_get(struct widget *self,int property) {
  return widget_base_get(self,property);
}

void background_trigger(struct widget *self,s32 type,union trigparam *param) {
  /* FIXME: add a way for apps to be notified of background clicks, like the
   *        old 'sysevent' ownership but less crufty.
   */
}

void background_resize(struct widget *self) {
   self->in->preferred.w = self->in->preferred.h = 0;
}

/* The End */
