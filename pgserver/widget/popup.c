/* $Id: popup.c,v 1.10 2000/08/27 05:54:28 micahjd Exp $
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
g_error create_popup(int x,int y,int w,int h,struct widget **wgt,int owner) {
  g_error e;

  /* Freeze the existing layer and make a new one */
  e = dts_push();
  errorcheck;

  /* Add the new popup widget - a simple theme-enabled container widget */
  e = widget_create(wgt,WIDGET_POPUP,dts->top,&dts->top->head->next,0,owner);
  errorcheck;

  (*wgt)->isroot = 1;  /* This widget has no siblings, so no point going
			  outside it anyway */

  /* Positioning, centering, and clipping */
  if (((signed short)x)==-1) x=(vid->xres>>1)-(w>>1); /*-1 centers */ 
  if (((signed short)y)==-1) y=(vid->yres>>1)-(h>>1);
  (*wgt)->in->div->x = x-current_theme[E_POPUP_BORDER].width;
  (*wgt)->in->div->y = y-current_theme[E_POPUP_BORDER].width;
  (*wgt)->in->div->w = w+(current_theme[E_POPUP_BORDER].width<<1);
  (*wgt)->in->div->h = h+(current_theme[E_POPUP_BORDER].width<<1);
  if ((*wgt)->in->div->x <0) (*wgt)->in->div->x = 0;
  if ((*wgt)->in->div->y <0) (*wgt)->in->div->y = 0;
  if ((*wgt)->in->div->x+(*wgt)->in->div->w >= vid->xres)
    (*wgt)->in->div->w = vid->xres-(*wgt)->in->div->x-1;
  if ((*wgt)->in->div->y+(*wgt)->in->div->h >= vid->yres)
    (*wgt)->in->div->h = vid->yres-(*wgt)->in->div->y-1;

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
  errorcheck;
  self->in->flags = DIVNODE_SPLIT_IGNORE;

  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->on_recalc = &popup;
  self->in->div->flags = DIVNODE_SPLIT_BORDER;
  self->in->div->split = current_theme[E_POPUP_BORDER].width;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;

  return sucess;
}

void popup_remove(struct widget *self) {
  struct divtree *p;

  if (!in_shutdown) {
    r_divnode_free(self->in);
    dts_pop();
    self->dt = NULL;
  }

  /* Redraw the top and everything below it */
  p = dts->top;
  while (p) {
    p->flags |= DIVTREE_ALL_REDRAW;
    p = p->next;
  }
}

g_error popup_set(struct widget *self,int property, glob data) {
  /* Because the layer(s) under a popup are 'frozen' it can't be moved
     after it is created.  Therefore, there isn't anything to change.
  */
  return mkerror(ERRT_BADPARAM,40);
}

glob popup_get(struct widget *self,int property) {
  return 0;
}

/* The End */


