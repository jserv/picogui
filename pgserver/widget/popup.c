/* $Id: popup.c,v 1.44 2002/01/21 08:17:30 micahjd Exp $
 *
 * popup.c - A root widget that does not require an application:
 *           creates a new layer and provides a container for other
 *           widgets.  This is a 'special' widget that should only
 *           be created with a call to create_popup.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 * pgCreateWidget & pgAttachWidget functionality added by RidgeRun Inc.
 * Copyright (C) 2001 RidgeRun, Inc.  All rights reserved.
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
  struct divnode ntb;

  /* If the popup is not allowed to overlap toolbars, clip to the nontoolbar
   * area. Otherwise, just clip to the root divnode to make sure we stay on
   * the display. */
  if (div->flags & DIVNODE_POPUP_NONTOOLBAR)
    ntb = *appmgr_nontoolbar_area();
  else
    ntb = *dts->root->head;

  /* In case the display resolution has been changed recently and the
   * divtree hasn't been recalc'ed we should clip the ntb to the display size
   * to prevent segfaulting.
   */
  if ( (ntb.x+ntb.w) > vid->lxres )
    ntb.w = vid->lxres - ntb.x;
  if ( (ntb.y+ntb.h) > vid->lyres )
    ntb.h = vid->lyres - ntb.y;

  if (div->x+div->w >= ntb.x+ntb.w)
    div->x = ntb.x+ntb.w - div->w;
  if (div->y+div->h >= ntb.y+ntb.h)
    div->y = ntb.y+ntb.h - div->h;
  if (div->x < ntb.x) div->x = ntb.x;
  if (div->y < ntb.y) div->y = ntb.y;
  if (div->x+div->w >= ntb.x+ntb.w)
    div->w = ntb.x+ntb.w - div->x;
  if (div->y+div->h >= ntb.y+ntb.h)
    div->h = ntb.y+ntb.h - div->y;

  /* Pretend these were the actual calculated
   * coordinates so the x,y position propagates through
   * the layout engine also.
   */
  div->calcx = div->x;
  div->calcy = div->y;
  div->calcw = div->w;
  div->calch = div->h;
}

/* We have a /special/ function to create a popup widget from scratch. */
g_error create_popup(int x,int y,int w,int h,struct widget **wgt,int owner) {
  g_error e;

  /* Freeze the existing layer and make a new one */
  e = dts_push();
  errorcheck;
  
  /* Add the new popup widget - a simple theme-enabled container widget */
  e = widget_create(wgt,PG_WIDGET_POPUP,dts->top, 0, owner);
  errorcheck;
  
  e = widget_attach(*wgt,dts->top,&dts->top->head->next,0,owner);  
  errorcheck;

  (*wgt)->isroot = 1;  /* This widget has no siblings, so no point going
			  outside it anyway */

  /* Give it a menu theme if it's position is PG_POPUP_ATCURSOR */
  if (x==PG_POPUP_ATCURSOR) {
    (*wgt)->in->div->state = PGTH_O_POPUP_MENU;
    (*wgt)->in->state = PGTH_O_POPUP_MENU;
  }

  /* Get margin value */
  (*wgt)->in->div->split = theme_lookup((*wgt)->in->div->state,PGTH_P_MARGIN);

  /* Set the position and size verbatim, let the
   * layout engine sort things out */

  (*wgt)->in->div->calcx = x;
  (*wgt)->in->div->calcy = y;
  (*wgt)->in->div->calcw = w;
  (*wgt)->in->div->calch = h;
   
  /* Escape can close the popup box */
  install_hotkey(*wgt,PGKEY_ESCAPE);

  /* Yahoo! */
  return success;
}

void build_popupbg(struct gropctxt *c,unsigned short state,struct widget *self) {
  struct divnode *ntb;

  /* If the popup is not allowed to overlap toolbars, clip to the nontoolbar
   * area. Otherwise, just clip to the root divnode to make sure we stay on
   * the display. */
  if (self->in->div->flags & DIVNODE_POPUP_NONTOOLBAR)
    ntb = appmgr_nontoolbar_area();
  else
    ntb = dts->root->head;

  /* Set the popup backdrop to only display over the nontoolbar
   * area when applicable.
   */
  c->x = ntb->x;
  c->y = ntb->y;
  c->w = ntb->w;
  c->h = ntb->h;

  /* exec_fillstyle knows not to use the default
     rectangle fill on a backdrop */
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
  self->in->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->flags |= DIVNODE_SPLIT_IGNORE | DIVNODE_SPLIT_POPUP;

  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_POPUP;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;
  
  self->trigger_mask = TRIGGER_DOWN | TRIGGER_HOTKEY;

  return success;
}

void popup_remove(struct widget *self) {
  struct divtree *p;
  u32 oldflags;

  oldflags = self->in->div->flags;

  if (!in_shutdown) {
    r_divnode_free(self->in);
    dts_pop(self->dt);
    self->dt = NULL;
  }

  /* If applicable, don't redraw toolbars on the root divtree.
   * Normally we could just use popup_toolbar_passthrough() but we
   * also take into account the tree we just deleted.
   */
  if ((oldflags & DIVNODE_POPUP_NONTOOLBAR) && 
      (popup_toolbar_passthrough() ||
       dts->top==dts->root))
    dts->root->flags |= DIVTREE_ALL_NONTOOLBAR_REDRAW;

  /* Set the normal 'redraw everything' flags for all layers */
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


