/* $Id: demo.c,v 1.1 2001/10/11 05:37:24 gork Exp $
 *
 * demo.c -   source file for testing PicoGUI
 *
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
 * Contributors: Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 * 
 */

#include <picogui.h>

/*** Widget variables */

pghandle wLabel,wIndicator;

/*** Event handlers */

int btnHello(struct pgEvent *evt) {
  static int i=0;

  pgReplaceTextFmt(wLabel,"Hello World\n#%d",++i);
  pgSetWidget(wIndicator,PG_WP_VALUE,i,0);

  return 0;
}

int myDebugEvtHandler(struct pgEvent *evt) {
  printf("Received event in myDebugEvtHandler: "
	 "#%d from 0x%08X, param = 0x%08X\n",
	 evt->type,evt->from,evt->e.param);
  
  return 0;
}

int closeboxHandler(struct pgEvent *evt) {
  /* Present a dialog box. If the user doesn't want to close,
     return 1 to prevent further handling of the event */

  return pgMessageDialog("Little demo proggie",
			 "Are you sure you want to close me?",
			 PG_MSGBTN_YES | PG_MSGBTN_NO)
    == PG_MSGBTN_NO;
}  


/*** Main program */

int main(int argc, char *argv[])
{
  pghandle wToolbar,wBox,wText;

  pgInit(argc,argv);

  /**** Register our application */

  pgRegisterApp(PG_APP_NORMAL,"Demo Application",0);

  /**** First level of widgets */

  /* A normal (non-standalone) toolbar */
  wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);

  wBox = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(0,
	      PG_WP_SIZE,100,
	      0);

  /* Make a nifty little click meter */
  wIndicator = pgNewWidget(PG_WIDGET_INDICATOR,0,0);
  pgSetWidget(0,
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(0,
	      PG_WP_SIDE,PG_S_TOP,    /* Place the widget at the top of the remaining space */
	      PG_WP_ALIGN,PG_A_LEFT,  /* Put the text at the left side of the widget */
	      PG_WP_TEXT,pgNewString("100 clicks"),
	      0);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(0,
	      PG_WP_TRANSPARENT,0,
	      PG_WP_TEXT,pgNewString("Click the buttons above"),
	      PG_WP_FONT,pgNewFont("Courier",0,0),
	      0);
  
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(0,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_ALIGN,PG_A_LEFT,
	      PG_WP_TEXT,pgNewString("0 clicks"),
	      0);
  
  /* Label widget to put text in later */
  wLabel = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(0,
	      PG_WP_TEXT,pgNewString("Foo..."),
	      PG_WP_FONT,pgNewFont("Utopia",0,0),
	      PG_WP_SIDE,PG_S_ALL,    /* Expand to remaining space */
	      PG_WP_TRANSPARENT,0,
	      0);

  /**** Widgets inside the toolbar */

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
  pgSetWidget(0,PG_WP_TEXT,pgNewString("Button"),0);
   
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(0,
	      PG_WP_TEXT,pgNewString("Toggle"),
	      PG_WP_EXTDEVENTS,PG_EXEV_TOGGLE,
	      0);
   
  pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(0,
	      PG_WP_TEXT,pgNewString("Checkski!"),
	      PG_WP_SIDE,PG_S_LEFT, /* checkboxes default to vertical stacking */
	      0);
   
  pgNewWidget(PG_WIDGET_FLATBUTTON,0,0);
  pgSetWidget(0,
	      PG_WP_TEXT,pgNewString("Flatbutton"),
	      0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(0,
	      PG_WP_TEXT,pgNewString("Bitmap"),
	      PG_WP_BITMAP,pgNewBitmap(pgFromFile("demos/data/tux.pnm")),
	      PG_WP_BITMASK,pgNewBitmap(pgFromFile("demos/data/tux_mask.pnm")),
	      0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,             /* PGDEFAULT is the same as
					using 0 here (see client_c.h) */
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TEXT,pgNewString("Hello, World!"),
	      0);
  /* this button gets an event handler */
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnHello,NULL);

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
		 "of this millennium.\n"
		 "\n----------------------------------------------------\n\n"
		 "This is more text\n"
		 "            (for your scrolling pleasure)\n"
		 "Brought to you by pgSetWidget() and pgNewString()\n"),
	      0);

  /* Add a scrollbar */
//  pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_BEFORE,wText);
//  pgSetWidget(0,PG_WP_BIND,wText,0);
		  pgNewWidget(PG_WIDGET_SCROLL, PG_DERIVE_BEFORE, wBox);
		  pgSetWidget(0, PG_WP_BIND, wBox, 0);
		  
		  
  /**** Add a handler that catches everything and prints it */

  pgBind(PGBIND_ANY,PGBIND_ANY,&myDebugEvtHandler,NULL);

  /**** A handler to confirm closing the app */

  pgBind(PGBIND_ANY,PG_WE_CLOSE,&closeboxHandler,NULL);

  /**** Run it! ****/
  pgEventLoop();
  return 0;
}





/* The End */
