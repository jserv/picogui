/* $Id: popup.c,v 1.50 2002/03/26 03:47:20 instinc Exp $
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

  if (div->calcx+div->calcw >= ntb.x+ntb.w)
    div->x = div->calcx = ntb.x+ntb.w - div->calcw;
  if (div->calcy+div->calch >= ntb.y+ntb.h)
    div->y = div->calcy = ntb.y+ntb.h - div->calch;
  if (div->calcx < ntb.x) 
    div->x = div->calcx = ntb.x;
  if (div->calcy < ntb.y) 
    div->y = div->calcy = ntb.y;
  if (div->calcx+div->calcw >= ntb.x+ntb.w)
    div->w = div->calcw = ntb.x+ntb.w - div->calcx;
  if (div->calcy+div->calch >= ntb.y+ntb.h)
    div->h = div->calch = ntb.y+ntb.h - div->calcy;
}

/* We have a /special/ function to create a popup widget from scratch. */
g_error create_popup(int x,int y,int w,int h,struct widget **wgt,int owner) {
  g_error e;

  /* The mouse won't be over the current widget any more.. */
  if (div_under_crsr && div_under_crsr->owner) {
    union trigparam param;
    memset(&param,0,sizeof(param));
    param.mouse.x = cursor->x;
    param.mouse.y = cursor->y;
    send_trigger(under, TRIGGER_LEAVE, &param);
  }

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
   
  /* Yahoo! */
  return success;
}

void build_popupbg(struct gropctxt *c,unsigned short state,struct widget *self) {
  struct divnode *ntb;

  /* Don't bother with it if the popup itself hasn't been sized yet */
  if (!(self->in->div->w && self->in->div->h))
    return;

  /* If the popup is not allowed to overlap toolbars, clip to the nontoolbar
   * area. Otherwise, just clip to the root divnode to make sure we stay on
   * the display. */
  if (self->in->div->flags & DIVNODE_POPUP_NONTOOLBAR)
    ntb = appmgr_nontoolbar_area();
  else
    ntb = dts->root->head;

  /* Clip it to the nontoolbar area, but pass the coordinates of the popup itself
   * so that the backdrop can easily draw a drop shadow, or a halo, etc..
   */

  self->in->x = ntb->x;
  self->in->y = ntb->y;
  self->in->w = ntb->w;
  self->in->h = ntb->h;

  c->x = self->in->div->x - ntb->x;
  c->y = self->in->div->y - ntb->y;
  c->w = self->in->div->w;
  c->h = self->in->div->h;

  /* exec_fillstyle knows not to use the default rectangle fill on a backdrop */
  exec_fillstyle(c,self->in->div->state,PGTH_P_BACKDROP);
}

void build_popup(struct gropctxt *c,unsigned short state,struct widget *self) {
  /* Rebuild the backdrop first.. 
   * Normally this would be done automatically, but we skip the automatic call
   * because the popup's size hasn't been calculated yet.
   */
  div_rebuild(self->in);

  exec_fillstyle(c,state,PGTH_P_BGFILL);
}

g_error popup_install(struct widget *self) {
  g_error e;

  /* This is positioned absolutely, so don't bother with the layout engine,
     let create_popup position it.
  */
  e = newdiv(&self->in,self);
  self->in->build = &build_popupbg;
  errorcheck;
  self->in->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->flags |= DIVNODE_SPLIT_IGNORE | DIVNODE_SPLIT_POPUP;

  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_popup;
  self->in->div->state = PGTH_O_POPUP;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;
  
  self->trigger_mask = TRIGGER_DOWN | TRIGGER_KEYUP | TRIGGER_KEYDOWN | TRIGGER_CHAR;

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

/* The DEACTIVATE event can be sent by a click outside the popup, or by pressing escape
 */
void popup_trigger(struct widget *self,s32 type,union trigparam *param) {

  switch (type) {

  case TRIGGER_KEYUP:
  case TRIGGER_KEYDOWN:
  case TRIGGER_CHAR:
    if (param->kbd.key != PGKEY_ESCAPE)
      return;
    param->kbd.consume++;
    if (type != TRIGGER_KEYUP)
      return;
    break;

  case TRIGGER_DOWN:
    if (div_under_crsr != self->in)
      return;

  }

  post_event(PG_WE_DEACTIVATE,self,0,0,NULL);
}

void popup_resize(struct widget *self) {
  /* This space intentionally left blank... */
}

/* The End */


