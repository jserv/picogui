/* $Id$
 *
 * submenuitem.c - a customized button, for menuitems opening submenus
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
#include <pgserver/hotspot.h>

g_error submenuitem_install(struct widget *self) {
  g_error e;

  /* Start with a button */
  e = button_install(self);
  errorcheck;

  /* Customize */
  widget_set(self, PG_WP_THOBJ_BUTTON,              PGTH_O_SUBMENUITEM);
  widget_set(self, PG_WP_THOBJ_BUTTON_ON,           PGTH_O_SUBMENUITEM_HILIGHT);
  widget_set(self, PG_WP_THOBJ_BUTTON_HILIGHT,      PGTH_O_SUBMENUITEM_HILIGHT);
  widget_set(self, PG_WP_THOBJ_BUTTON_ON_NOHILIGHT, PGTH_O_SUBMENUITEM_HILIGHT);

  /* We need extra events */
  widget_set(self,PG_WP_EXTDEVENTS,PG_EXEV_PNTR_UP | PG_EXEV_NOCLICK);

#ifdef CONFIG_SUBMENU_NAVIGATION
  /* TTH: Register submenuitem_trigger callback */
  self->def->trigger = submenuitem_trigger;
#endif /* CONFIG_SUBMENU_NAVIGATION */
  
  /* Stack vertically */
  widget_set(self,PG_WP_SIDE,PG_S_TOP);

  return success;
}

#ifdef CONFIG_SUBMENU_NAVIGATION
/* TTH: Submenuitem_trigger callback */
void submenuitem_trigger(struct widget *self,s32 type,union trigparam *param) {

  /* Handle hotkey_right as activate */
  switch (type) {

  case PG_TRIGGER_KEYUP:
  case PG_TRIGGER_KEYDOWN:
  case PG_TRIGGER_CHAR:
    if ( param->kbd.key == hotkey_right ) {      
      param->kbd.key = hotkey_activate;
    }
  }
  
  /* Anyway relay then all events to the regular button handler */
  button_trigger (self, type, param);
}
#endif /* CONFIG_SUBMENU_NAVIGATION */

/* The End */



