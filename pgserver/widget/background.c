/* $Id: background.c,v 1.12 2002/05/20 19:18:38 micahjd Exp $
 *
 * background.c - an internal widget for drawing the screen background
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

void build_background(struct gropctxt *c,unsigned short state,struct widget *self) {
  
  /* Tweak the coordinates a bit so that the background is relative
     to the screen, not the clipping rectangle imposed by the divnode. */  
  c->x = -self->in->x;
  c->y = -self->in->y;
  c->w = vid->lxres;
  c->h = vid->lyres;

  /* As normal... */
  exec_fillstyle(c,state,PGTH_P_BGFILL);
}

g_error background_install(struct widget *self) {
  g_error e;
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->build = &build_background;
  self->in->state = PGTH_O_BACKGROUND;
  self->out = &self->in->next;

  self->trigger_mask = TRIGGER_DOWN;
   
  return success;
}

void background_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error background_set(struct widget *self,int property, glob data) {
  return success;
}

glob background_get(struct widget *self,int property) {
  return 0;
}

void background_trigger(struct widget *self,s32 type,union trigparam *param) {
  if (sysevent_owner)
     post_event(PG_NWE_BGCLICK,NULL,param->mouse.chbtn,sysevent_owner,NULL);
}

void background_resize(struct widget *self) {
   self->in->pw = self->in->ph = 0;
}

/* The End */











