/* $Id: managed_rootless.c,v 1.3 2002/10/24 03:32:40 micahjd Exp $
 *
 * managed_rootless.c - Application management for rootless modes 
 *                      managed by a host GUI
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
#include <pgserver/appmgr.h>


/**************************************** Public interface */

g_error appmgr_managed_rootless_init(void) {
  g_error e;

  /* Try to put the driver in rootless mode */
  e = video_setmode(0,0,0,PG_FM_ON,PG_VID_ROOTLESS);
  errorcheck;
  if (!VID(is_rootless)())
    return mkerror(PG_ERRT_BADPARAM,142);  /* Requires rootless mode */

  return success;
}

g_error appmgr_managed_rootless_reg(struct app_info *i) {
  struct widget *w;
  g_error e;
  
  /* FIXME: this needs to handle toolbar apps correctly */

  e = widget_create(&w,&i->rootw,PG_WIDGET_MANAGEDWINDOW,dts->root, 0, i->owner);
  errorcheck;

  e = widget_set(w,PG_WP_TEXT,i->name);
  errorcheck;

  return success;
}


/**************************************** Registration */

struct appmgr appmgr_managed_rootless = {
   name:  "managed_rootless",
   init:  appmgr_managed_rootless_init,
    reg:  appmgr_managed_rootless_reg,
};

/* The End */



