/* $Id$
 *
 * if_key_dispatch.c - Send key events to widgets
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
#include <pgserver/input.h>
#include <pgserver/handle.h>
#include <pgserver/appmgr.h>
#include <pgserver/widget.h>

/* Iterator for dispatching a key to all widgets */
struct send_trigger_iterator_data {
  union trigparam *param;
  u32 type;
};
g_error send_trigger_iterator(const void **obj, void *extra);

/*************************************************** Input filter ****/

void infilter_key_dispatch_handler(struct infilter *self, u32 trigger, union trigparam *param) {
  struct send_trigger_iterator_data data;
  struct widget *kbdfocus;
  int kflags;
  struct app_info **app;
  struct app_info *ap;
  struct divtree *dt;
  struct widget *p;

  inactivity_reset();

  /* Find the divtree we should get the focused widget from.
   * Normally this is the topmost divtree, but it can be overridden
   * by the incoming trigger. This is necessary for rootless and multihead.
   */
  if (iserror(rdhandle((void**)&dt, PG_TYPE_DIVTREE, -1, param->kbd.divtree)) || !dt)
    dt = dts->top;

  /* Get the focused widget */
  kbdfocus = NULL;
  rdhandle((void**)&kbdfocus, PG_TYPE_WIDGET, -1, dt->focus);

  /*
   *  Now for the fun part... propagating this event to all the widgets.
   *  It will eventually end up at all the widgets, but the order is
   *  important.
   *
   *   1. all popup widgets and their children, from top to bottom
   *   2. all root widgets and their children, in decreasing pseudo-z-order
   *   3. all widgets (to handle unattached widgets)
   *   4. focused widget
   *
   *  Since PicoGUI doesn't have a real z-order for root widgets, the
   *  order will determined by how recently the root widget had a 
   *  focused child widget. 
   */

  /* Let widgets know we're starting a new propagation */
  data.param = param;
  data.type = PG_TRIGGER_KEY_START;
  param->kbd.flags = PG_KF_ALWAYS;
  handle_iterate(PG_TYPE_WIDGET,send_trigger_iterator,&data);

  /* 1. Popup widgets and their children
   */
  param->kbd.flags = PG_KF_ALWAYS;  
  param->kbd.consume = 0;
  for (dt=dts->top;dt && dt!=dts->root;dt=dt->next) {
    if (dt->head->next)
      r_send_trigger(dt->head->next->owner, trigger, param, &param->kbd.consume, 0);
    if (param->kbd.consume > 0)
      return;    
  }

  /* 2. Other root widgets and their children 
   */
  param->kbd.flags = PG_KF_ALWAYS | PG_KF_APP_TOPMOST;
  for (ap=applist;ap;ap=ap->next) {
    if (iserror(rdhandle((void**)&p,PG_TYPE_WIDGET,ap->owner,ap->rootw)))
      continue;

    r_send_trigger(p, trigger, param, &param->kbd.consume, 0);
    if (param->kbd.consume > 0)
      return;

    param->kbd.flags &= ~PG_KF_APP_TOPMOST;
  }

  /* 3. Any widget
   */
  data.param = param;
  data.type = trigger;
  param->kbd.flags = PG_KF_ALWAYS;
  handle_iterate(PG_TYPE_WIDGET,send_trigger_iterator,&data);
  if (param->kbd.consume > 0)
    return;
    
  if (kbdfocus) {
    kflags = PG_KF_ALWAYS;

    /* Is the focused widget in the topmost app?
     */
    app = appmgr_findapp(kbdfocus);
    if (app && (*app)==applist)
      kflags |= PG_KF_APP_TOPMOST;

    /* 4. focused widget 
     */
    param->kbd.flags = kflags | PG_KF_FOCUSED;
    send_trigger(kbdfocus,trigger,param);
    if (param->kbd.consume > 0)
      return;
  }

  /* If none of the widgets have consumed the event, pass it on */
  if (!param->kbd.consume)
    infilter_send(self,trigger,param);
}

struct infilter infilter_key_dispatch = {
  /*accept_trigs:  */PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP | PG_TRIGGER_CHAR,
  /*absorb_trigs:  */PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP | PG_TRIGGER_CHAR,
       /*handler:  */&infilter_key_dispatch_handler
};

/*************************************************** Private utilities ****/

g_error send_trigger_iterator(const void **obj, void *extra) {
  struct send_trigger_iterator_data *data = (struct send_trigger_iterator_data *) extra;
  struct widget *w = *((struct widget **) obj);

  if (!data->param->kbd.consume)
    send_trigger(w, data->type, data->param);

  return success;
}


/* The End */






