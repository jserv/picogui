/* $Id: popup.c,v 1.20 2000/11/05 10:24:48 micahjd Exp $
 *
 * popup.c - A root widget that does not require an application:
 *           creates a new layer and provides a container for other
 *           widgets.  This is a 'special' widget that should only
 *           be created with a call to create_popup.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/widget.h>
#include <pgserver/appmgr.h>

/* We have a /special/ function to create a popup widget from scratch. */
g_error create_popup(int x,int y,int w,int h,struct widget **wgt,int owner) {
  g_error e;
  int margin;

  /* Freeze the existing layer and make a new one */
  e = dts_push();
  errorcheck;

  /* Add the new popup widget - a simple theme-enabled container widget */
  e = widget_create(wgt,PG_WIDGET_POPUP,dts->top,&dts->top->head->next,0,owner);
  errorcheck;

  (*wgt)->isroot = 1;  /* This widget has no siblings, so no point going
			  outside it anyway */

  /* Special positioning codes */

  if (((signed short)x) == PG_POPUP_CENTER) {
    x=(vid->xres>>1)-(w>>1);
    y=(vid->yres>>1)-(h>>1);
  }

  if (((signed short)x) == PG_POPUP_ATCURSOR) {
    x = pointer->x+(pointer->w/2);
    y = pointer->y-(pointer->h/2);;
 
    /* pop vertically if the cursor is on the bottom half of the screen */
    // y = (pointer->y > (vid->yres>>1)) ? (pointer->y - h) : pointer->y;

    (*wgt)->in->div->state = PGTH_O_POPUP_MENU;
    (*wgt)->in->state = PGTH_O_POPUP_MENU;
  }

  /* Clipping and things */
  (*wgt)->in->div->split = margin =
    theme_lookup((*wgt)->in->div->state,PGTH_P_MARGIN);
  (*wgt)->in->div->x = x-margin;
  (*wgt)->in->div->y = y-margin;
  (*wgt)->in->div->w = w+(margin<<1);
  (*wgt)->in->div->h = h+(margin<<1);
  if ((*wgt)->in->div->x+(*wgt)->in->div->w >= vid->xres)
    (*wgt)->in->div->x = vid->xres - (*wgt)->in->div->w - margin;
  if ((*wgt)->in->div->y+(*wgt)->in->div->h >= vid->yres)
    (*wgt)->in->div->y = vid->yres - (*wgt)->in->div->h - margin;
  if ((*wgt)->in->div->x <0) (*wgt)->in->div->x = 0;
  if ((*wgt)->in->div->y <0) (*wgt)->in->div->y = 0;
  if ((*wgt)->in->div->x+(*wgt)->in->div->w >= vid->xres)
    (*wgt)->in->div->w = vid->xres-(*wgt)->in->div->x-1;
  if ((*wgt)->in->div->y+(*wgt)->in->div->h >= vid->yres)
    (*wgt)->in->div->h = vid->yres-(*wgt)->in->div->y-1;

  /* Yahoo! */
  return sucess;
}

void build_popupbg(struct gropctxt *c,unsigned short state,struct widget *self) {
  /* exec_fillstyle knows not to use the default rectangle fill on a backdrop */
  exec_fillstyle(c,state,PGTH_P_BACKDROP);
}

g_error popup_install(struct widget *self) {
  g_error e;

  /* This is positioned absolutely, so don't bother with the layout engine,
     let create_popup position it.
  */
  e = newdiv(&self->in,self);
  self->in->build = &build_popupbg;
  self->in->state = PGTH_O_POPUP;
  errorcheck;
  self->in->flags = DIVNODE_SPLIT_IGNORE;

  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_POPUP;
  self->in->div->flags = DIVNODE_SPLIT_BORDER;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;
  
  self->trigger_mask = TRIGGER_DOWN;

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
  return mkerror(PG_ERRT_BADPARAM,40);
}

glob popup_get(struct widget *self,int property) {
  return 0;
}

void popup_trigger(struct widget *self,long type,union trigparam *param) {
  /* Only possible trigger (due to the mask) is a mouse down. 
   * If it's outside the panel, it's a DEACTIVATE */

  if (div_under_crsr == self->in)
    post_event(PG_WE_DEACTIVATE,self,0,0);
}

/* The End */


