/* $Id$
 *
 * browserwin.h - User interface for a browser window in Atomic Navigator
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

#ifndef __H_BROWSERWIN
#define __H_BROWSERWIN

#include <picogui.h>
#include "url.h"

/********************************* General Constants */

#define BROWSER_TITLE "Atomic Navigator"
#define BROWSER_NAME  "atomicnav.browserwin"
#define USER_AGENT    "AtomicNavigator/0.0 (PicoGUI)"

/********************************* Structure */

struct browserwin {
  /* top-level widgets */
  pghandle wApp; 
  pghandle wNavTB; 
  pghandle wStatusTB;
  pghandle wMainBox;
  
  /* Inside wMainBox */
  pghandle wView;
  pghandle wScroll;

  /* Inside wStatusTB */
  pghandle wStatus;
  pghandle wProgress;

  /* Inside wNavTB */
  pghandle wBack;
  pghandle wForward;
  pghandle wStop;
  pghandle wURL;
  pghandle wGo;

  /* A STATUS_* constant */
  int status;

  /* URL of loaded web page */
  struct url *page;
};

/********************************* Methods */

struct browserwin *browserwin_new(void);
void browserwin_showstatus(struct browserwin *w, struct url *u);
void browserwin_errormsg(struct browserwin *w, const char *msg);
void browserwin_command(pghandle w, const char *command, const char *param);

#endif /* __H_BROWSERWIN */

/* The End */









