/* $Id: demo.c,v 1.8 2000/09/22 18:04:23 pney Exp $
 *
 * demo.c -   source file for testing PicoGUI
 *
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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
 * Contributors: Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 * 
 */

#include <picogui.h>

int main(int argc, char *argv[])
{
  pghandle wToolbar,wBox,wText;
  pghandle bw;

  pgInit(argc,argv);

  /**** Register our application */

  pgRegisterApp(PG_APP_NORMAL,"foo",
		PG_APPSPEC_WIDTH,100,
		PG_APPSPEC_SIDE,PG_S_LEFT,0);

  /**** First level of widgets */

  /* A normal (non-standalone) toolbar */
  wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);

  wBox = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(0,
	      PG_WP_SIZE,100,
	      PG_WP_BORDERCOLOR,0x000000,
	      0);

  /**** Widgets inside the toolbar */

  bw = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
  pgSetWidget(bw,
	      PG_WP_ALIGN,PG_A_LEFT,
	      PG_WP_TEXT,pgNewString("1"),
	      PG_WE_ACTIVATE,0x0000,
	      0);

  bw = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(bw,
	      PG_WP_ALIGN,PG_A_LEFT,
	      PG_WP_TEXT,pgNewString("2"),
	      PG_WE_ACTIVATE,0x0001,
	      0);

  bw = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(bw,
	      PG_WP_ALIGN,PG_A_LEFT,
	      PG_WP_TEXT,pgNewString("Hello, World!"),
	      PG_WE_ACTIVATE,0x0002,
	      0);

  /**** Text inside the box */
  
  wText = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wBox);
  pgSetWidget(0,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TEXT,pgNewString(
                 "Mobile internet appliances\n"
		 "will likely be the smart merge\n"
		 "of computer power, multimedia,\n"
		 "Internet, wireless telecommunication\n"
		 "technology and fashion; this is the\n"
		 "reason why this market is to be\n"
		 "considered as one of the major fast\n"
		 "growing markets at the beginning\n"
		 "of this millennium.\n"),
	      0);

  /* Add a scrollbar */
  pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_BEFORE,wText);
  pgSetWidget(0,
	      PG_WP_BIND,wText,
	      0);

  /**** Run it! ****/
  pgEventLoop();
  return 0;
}





/* The End */
