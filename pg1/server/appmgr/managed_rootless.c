/* $Id$
 *
 * managed_rootless.c - Application management for rootless modes 
 *                      managed by a host GUI
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
#include <pgserver/appmgr.h>

struct divtree *managed_rootless_background;


/**************************************** Public interface */

g_error appmgr_managed_rootless_init(void) {
  g_error e;

  /* Try to put the driver in rootless mode */
  e = video_setmode(0,0,0,PG_FM_ON,PG_VID_ROOTLESS);
  errorcheck;
  if (!VID(is_rootless)())
    return mkerror(PG_ERRT_BADPARAM,142);  /* Requires rootless mode */

  e = dts_new();
  errorcheck;

  /* We can optionally create a background window holding a
   * background widget and any toolbar apps.
   */
  if (get_param_int("appmgr-managed-rootless","background",0)) {
    struct widget *bgwidget;

    /* New divtree for the background */
    e = dts_push();
    errorcheck;
    managed_rootless_background = dts->top;
    VID(window_set_flags)(dts->top->display, PG_WINDOW_BACKGROUND);

    /* Make the background widget */
    e = widget_create(&bgwidget,&res[PGRES_BACKGROUND_WIDGET],
		      PG_WIDGET_BACKGROUND,dts->top, 0, -1);
    errorcheck;
    e = widget_attach(bgwidget, dts->top, &dts->top->head->next,0);
    errorcheck;   

    /* Turn off the background's DIVNODE_UNDERCONSTRUCTION flags */
    activate_client_divnodes(-1);
  }

  return success;
}

g_error appmgr_managed_rootless_reg(struct app_info *i) {
  struct widget *w;
  g_error e;
  
  if (i->type == PG_APP_TOOLBAR && managed_rootless_background) {
    /* Toolbars are added to the background divtree, if we have one */

    e = widget_create(&w,&i->rootw,PG_WIDGET_TOOLBAR,managed_rootless_background, 0, i->owner);
    errorcheck;
    e = widget_attach(w,managed_rootless_background,&managed_rootless_background->head->next,0);
    errorcheck;

    e = widget_set(w,PG_WP_SIDE,i->side);
    errorcheck;

    w->isroot = 1;

    return success;
  }
  
  /* All other apps have normal managed windows */
  e = widget_create(&w,&i->rootw,PG_WIDGET_MANAGEDWINDOW,dts->root, 0, i->owner);
  errorcheck;
  
  e = widget_set(w,PG_WP_TEXT,i->name);
  errorcheck;

  return success;
}

int appmgr_managed_rootless_widget_map(int w) {
  switch (w) {
    
    /* Convert popups into managed windows */
  case PG_WIDGET_POPUP:
    return PG_WIDGET_MANAGEDWINDOW;
    
  }
  return w;
}

/**************************************** Registration */

struct appmgr appmgr_managed_rootless = {
             /*name:  */"managed_rootless", 
             /*init:  */appmgr_managed_rootless_init,
         /*shutdown:  */NULL, 
              /*reg:  */appmgr_managed_rootless_reg, 
            /*unreg:  */NULL,
  /*nontoolbar_area:  */NULL, 
       /*widget_map:  */appmgr_managed_rootless_widget_map
};

/* The End */



