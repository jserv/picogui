/* $Id$
 *
 * popup.c - A root widget for modal dialogs that display above the
 *           root divtree.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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

/* for memset() */
#include <string.h>

#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/appmgr.h>
#include <pgserver/hotspot.h>

struct popupdata {
  struct divtree *my_dt;
  /* TTH: Menu/submenu flag */
  unsigned int menu : 1;
  unsigned int submenu : 1;
};
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(popupdata)

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
  if ( (ntb.r.x+ntb.r.w) > vid->lxres )
    ntb.r.w = vid->lxres - ntb.r.x;
  if ( (ntb.r.y+ntb.r.h) > vid->lyres )
    ntb.r.h = vid->lyres - ntb.r.y;

  if (div->calc.x+div->calc.w >= ntb.r.x+ntb.r.w)
    div->r.x = div->calc.x = ntb.r.x+ntb.r.w - div->calc.w;
  if (div->calc.y+div->calc.h >= ntb.r.y+ntb.r.h)
    div->r.y = div->calc.y = ntb.r.y+ntb.r.h - div->calc.h;
  if (div->calc.x < ntb.r.x) 
    div->r.x = div->calc.x = ntb.r.x;
  if (div->calc.y < ntb.r.y) 
    div->r.y = div->calc.y = ntb.r.y;
  if (div->calc.x+div->calc.w >= ntb.r.x+ntb.r.w)
    div->r.w = div->calc.w = ntb.r.x+ntb.r.w - div->calc.x;
  if (div->calc.y+div->calc.h >= ntb.r.y+ntb.r.h)
    div->r.h = div->calc.h = ntb.r.y+ntb.r.h - div->calc.y;
}

void build_popupbg(struct gropctxt *c,unsigned short state,struct widget *self) {
  struct divnode *ntb;

  /* Don't bother with it if the popup itself hasn't been sized yet */
  if (!(self->in->div->r.w && self->in->div->r.h) || 
      self->in->div->r.x < 0 || self->in->div->r.y < 0)
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

  self->in->r.x = ntb->r.x;
  self->in->r.y = ntb->r.y;
  self->in->r.w = ntb->r.w;
  self->in->r.h = ntb->r.h;

  c->r.x = self->in->div->r.x - ntb->r.x;
  c->r.y = self->in->div->r.y - ntb->r.y;
  c->r.w = self->in->div->r.w;
  c->r.h = self->in->div->r.h;

  if (theme_lookup(self->in->div->state,PGTH_P_BACKDROP)) {
    /* exec_fillstyle knows not to use the default rectangle fill on a backdrop */
    exec_fillstyle(c,self->in->div->state,PGTH_P_BACKDROP);
  }
  else {
    /* Stick a no-op node in so that the backdrop is at least clickable. A divnode
     * can only be clicked if it has trigger handlers and it either has a build function
     * or a groplist. This is so that purely layout divnodes don't intercept events.
     * An alternative to this no-op method would be to put a fake build handler in
     * instead of deleting it altogether, but then we'd also have to turn on the raw build
     * flag. IMHO this is a slightly cleaner solution.
     */
    addgrop(c,PG_GROP_NOP);
  }

  /* Since the backdrop should only be rendered once, self-destruct this build handler */
  self->in->build = NULL;
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

  WIDGET_ALLOC_DATA(popupdata)
  
  /* TTH: Reset Menu/submenu flags */
  DATA->menu = 0;
  DATA->submenu = 0;

  /* Before freezing the current layer, make sure it's up to date */
  activate_client_divnodes(self->owner);
  update(NULL,1);

  /* Freeze the existing layer and make a new one */
  e = dts_push();
  errorcheck;
  DATA->my_dt = dts->top;

  e = newdiv(&self->in,self);
  self->in->build = &build_popupbg;
  errorcheck;
  self->in->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->flags |= DIVNODE_SPLIT_IGNORE | DIVNODE_SPLIT_POPUP;

  /* The DIVNODE_SPLIT_POPUP flag will cause the sizing to be
   * fully calculated at the next recalc, this gives the client
   * a chance to set the size and/or populate the popup and
   * let the autosizing work its magic.
   */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_popup;
  self->in->div->state = PGTH_O_POPUP;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->div->split = theme_lookup(self->in->div->state,PGTH_P_MARGIN);

  self->out = &self->in->next;
  self->sub = &self->in->div->div;
  
  self->trigger_mask = PG_TRIGGER_DOWN | PG_TRIGGER_KEYUP | PG_TRIGGER_KEYDOWN | PG_TRIGGER_CHAR;

  /* Attach ourselves as a root widget in the new divtree */
  e = widget_attach(self,DATA->my_dt,&DATA->my_dt->head->next,0);  
  errorcheck;
  self->isroot = 1;

  /* The client now has to set the position using widget properties before the
   * widget is first processed by the layout engine. There's no problem with races
   * here, because of DIVNODE_UNDERCONSTRUCTION flags.
   *
   * Set some defaults: automatic sizing, and centered
   */

  self->in->div->calc.x = PG_POPUP_CENTER;
  self->in->div->calc.y = PG_POPUP_CENTER;
  self->in->div->calc.w = 0;
  self->in->div->calc.h = 0;
  
  return success;
}

void popup_remove(struct widget *self) {
  struct divtree *p;
  u32 oldflags;

  oldflags = self->in->div->flags;

  /* Delete this widget's divnodes, and the divtree itself */
  r_divnode_free(self->in);
  dts_pop(DATA->my_dt);
 
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

  g_free(DATA);
}

g_error popup_set(struct widget *self,int property, glob data) {
  /* If the popup has already been 'frozen' by the layout engine,
   * give an error.
   */
  if (!(self->in->flags & DIVNODE_SPLIT_POPUP))
    switch (property) {
    case PG_WP_ABSOLUTEX:
    case PG_WP_ABSOLUTEY:
    case PG_WP_WIDTH:
    case PG_WP_HEIGHT:
      return mkerror(PG_ERRT_BADPARAM,44);   /* Can't reposition the popup */
    }

  switch (property) {
    
  case PG_WP_ABSOLUTEX:
    /* Give it a menu theme if it's position is PG_POPUP_ATCURSOR or _ATEVENT */
    if (data==PG_POPUP_ATCURSOR || data==PG_POPUP_ATEVENT) {
      self->in->div->state = PGTH_O_POPUP_MENU;
      self->in->state = PGTH_O_POPUP_MENU;
      self->in->div->split = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
    }
    self->in->div->calc.x = data;
    break;

  case PG_WP_ABSOLUTEY:
    self->in->div->calc.y = data;
    break;

  case PG_WP_WIDTH:
    self->in->div->calc.w = data;
    break;

  case PG_WP_HEIGHT:
    self->in->div->calc.h = data;
    break;
  
  /* TTH: Menu/submenu flags */
  case PG_WP_POPUP_IS_MENU:
    DATA->menu = data;
    self->in->div->state = PGTH_O_POPUP_MENU;
    self->in->state = PGTH_O_POPUP_MENU;
    self->in->div->split = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
    break;
  
  case PG_WP_POPUP_IS_SUBMENU:
    DATA->submenu = data;
    self->in->div->state = PGTH_O_POPUP_MENU;
    self->in->state = PGTH_O_POPUP_MENU;
    self->in->div->split = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
    break;

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob popup_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_ABSOLUTEX:
    return self->in->div->calc.x;

  case PG_WP_ABSOLUTEY:
    return self->in->div->calc.y;

  case PG_WP_WIDTH:
    return self->in->div->calc.w;

  case PG_WP_HEIGHT:
    return self->in->div->calc.h;

  }
  return widget_base_get(self,property);
}

/* The DEACTIVATE event can be sent by a click outside the popup, or by pressing escape
 */
void popup_trigger(struct widget *self,s32 type,union trigparam *param) {

  switch (type) {

  case PG_TRIGGER_KEYUP:
  case PG_TRIGGER_KEYDOWN:
  case PG_TRIGGER_CHAR:
#ifdef CONFIG_SUBMENU_NAVIGATION
    /* TTH: Menu/submenu popup : handle hotkey_left as deactivate */
    if ( (param->kbd.key != PGKEY_ESCAPE) &&
         ((param->kbd.key != hotkey_left) || 
         (!(DATA->menu) && !(DATA->submenu))))
      return;
#else 
    if (param->kbd.key != PGKEY_ESCAPE) 
      return;
#endif /* CONFIG_SUBMENU_NAVIGATION */
    
    param->kbd.consume++;
    if (type != PG_TRIGGER_KEYUP)
      return;
    break;

  case PG_TRIGGER_DOWN:
    if (param->mouse.cursor->ctx.div_under != self->in)
      return;

  }

  post_event(PG_WE_DEACTIVATE,self,0,0,NULL);
}

void popup_resize(struct widget *self) {
  /* This space intentionally left blank */
}

/* The End */


