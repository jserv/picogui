/* $Id: main.c,v 1.1 2001/02/07 08:32:33 micahjd Exp $
 * 
 * main.c - Main source file for the PicoGui Launcher, PGL
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

#include <picogui.h>

/********* Event handlers */

/* Event handler to run the main launcher menu */
int evtMainMenu(struct pgEvent *evt) {
   
   pgMenuFromString("PicoGUI Launcher\n---------------------\n|Applications|"
		    "System|Cruft");
   
}

/********* Main program */

int main(int argc, char **argv) {
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_TOOLBAR,"PGL",0);

  /* Button to launch menu */
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Menu"),
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&evtMainMenu,NULL);

  /* Background click also launches menu */
  pgRegisterOwner(PG_OWN_SYSEVENTS);
  pgBind(PGBIND_ANY,PG_NWE_BGCLICK,&evtMainMenu,NULL);
   
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("PicoGUI Launcher"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_FONT,pgNewFont(NULL,0,PG_FSTYLE_BOLD),
	      0);

  pgEventLoop();   
  return 0;
}
   
/* The End */
