/* $Id: popup.c,v 1.3 2000/06/03 17:50:43 micahjd Exp $
 *
 * popup.c - A root widget that does not require an application:
 *           creates a new layer and provides a container for other
 *           widgets.  This is a 'special' widget that should only
 *           be created with a call to create_popup.
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
#include <theme.h>

/* We have a /special/ function to create a popup widget from scratch. */
g_error create_popup(int x,int y,int w,int h,struct widget **wgt) {
  g_error e;

  /* Freeze the existing layer and make a new one */
  e = dts_push();
  if (e.type != ERRT_NONE) return e;

  /* Add the new popup widget - a simple theme-enabled container widget */
  e = widget_create(wgt,WIDGET_POPUP,dts->top,&dts->top->head->next);
  if (e.type != ERRT_NONE) return e;

  (*wgt)->isroot = 1;  /* This widget has no siblings, so no point going
			  outside it anyway */

  /* Clip the size to the screen */
  if (x<0) x=0; if (y<0) y=0;
  if (x+w>=HWR_WIDTH) w = HWR_WIDTH-x-1;
  if (y+h>=HWR_HEIGHT) h = HWR_HEIGHT-y-1;

  /* Positioning */
  (*wgt)->in->div->x = x;
  (*wgt)->in->div->y = y;
  (*wgt)->in->div->w = w;
  (*wgt)->in->div->h = h;

  /* If this is the first popup layer (after the root layer) dim the screen */
  if (dts->top->next==dts->root)
    grop_dim(&(*wgt)->in->grop);

  /* Yahoo! */
  return sucess;
}

void popup(struct divnode *d) {
  int x,y,w,h;
  x=y=0; w=d->w; h=d->h;

  addelement(d,&current_theme[E_POPUP_BORDER],&x,&y,&w,&h);
  addelement(d,&current_theme[E_POPUP_FILL],&x,&y,&w,&h);
}

g_error popup_install(struct widget *self) {
  g_error e;

  /* This is positioned absolutely, so don't bother with the layout engine,
     let create_popup position it.
  */
  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags = DIVNODE_SPLIT_IGNORE;

  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &popup;
  self->in->div->flags = DIVNODE_SPLIT_BORDER;
  self->in->div->split = current_theme[E_POPUP_BORDER].width;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;

  return sucess;
}

void popup_remove(struct widget *self) {
  r_divnode_free(self->in);
  dts_pop();
  self->dt = NULL;

  /* With the popup and the top layer gone, 
     force a redraw of the layer(s) below
  */
  update();
}

g_error popup_set(struct widget *self,int property, glob data) {
  /* Because the layer(s) under a popup are 'frozen' it can't be moved
     after it is created.  Therefore, there isn't anything to change.
  */
  return mkerror(ERRT_BADPARAM,"Invalid property for popup");
}

glob popup_get(struct widget *self,int property) {
  return 0;
}

/* The End */


