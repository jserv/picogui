/* $Id: popup.c,v 1.34 2001/07/24 12:43:09 micahjd Exp $
 *
 * popup.c - A root widget that does not require an application:
 *           creates a new layer and provides a container for other
 *           widgets.  This is a 'special' widget that should only
 *           be created with a call to create_popup.
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

/* Clipping for popup boxes. Mainly used in create_popup, but it is
 * also called when switching video modes */
void clip_popup(struct divnode *div) {
  struct divnode *ntb;

  /* If the popup is not allowed to overlap toolbars, clip to the nontoolbar
   * area. Otherwise, just clip to the root divnode to make sure we stay on
   * the display. */
  if (div->flags & DIVNODE_POPUP_NONTOOLBAR)
    ntb = appmgr_nontoolbar_area();
  else
    ntb = dts->root->head;

  if (div->x+div->w >= ntb->x+ntb->w)
    div->x = ntb->x+ntb->w - div->w;
  if (div->y+div->h >= ntb->y+ntb->h)
    div->y = ntb->y+ntb->h - div->h;
  if (div->x < ntb->x) div->x = ntb->x;
  if (div->y < ntb->y) div->y = ntb->y;
  if (div->x+div->w >= ntb->x+ntb->w)
    div->w = ntb->x+ntb->w - div->x;
  if (div->y+div->h >= ntb->y+ntb->h)
    div->h = ntb->y+ntb->h - div->y;
}

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

  /* Get margin value */
  (*wgt)->in->div->split = theme_lookup((*wgt)->in->div->state,PGTH_P_MARGIN);

  /* Give it a menu theme if it's position is PG_POPUP_ATCURSOR */
  if (x==PG_POPUP_ATCURSOR) {
    (*wgt)->in->div->state = PGTH_O_POPUP_MENU;
    (*wgt)->in->state = PGTH_O_POPUP_MENU;
  }

  /* Set the position and size verbatim, let the
   * layout engine sort things out */

  (*wgt)->in->div->x = x;
  (*wgt)->in->div->y = y;
  (*wgt)->in->div->w = w;
  (*wgt)->in->div->h = h;
   
  /* Escape can close the popup box */
  install_hotkey(*wgt,PGKEY_ESCAPE);

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
  self->in->flags |= DIVNODE_SPLIT_IGNORE | DIVNODE_SPLIT_POPUP;

  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_POPUP;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;
  
  self->trigger_mask = TRIGGER_DOWN | TRIGGER_HOTKEY;

  return sucess;
}

void popup_remove(struct widget *self) {
  struct divtree *p;

  if (!in_shutdown) {
    r_divnode_free(self->in);
    dts_pop(self->dt);
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
  return mkerror(ERRT_PASS,0);
}

glob popup_get(struct widget *self,int property) {
  return 0;
}

void popup_trigger(struct widget *self,long type,union trigparam *param) {
  /* The DEACTIVATE event can be sent by a click outside the popup, or by the
     close hotkey */

  if (type==TRIGGER_DOWN && div_under_crsr != self->in)
    return;
  
  post_event(PG_WE_DEACTIVATE,self,0,0,NULL);
}

void popup_resize(struct widget *self) {
  /* This space intentionally left blank... */
}

/* The End */


