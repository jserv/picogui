/* $Id: msgdialog.c,v 1.5 2001/02/02 07:42:06 micahjd Exp $
 *
 * msgdialog.c - message dialog box demo
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

int rundemo(struct pgEvent *evt) {
  if (PG_MSGBTN_OK ==
      pgMessageDialog("msgdialog.c test program",
		      "This is a test dialog box.\n\n"
		      "Hello\n"
		      "World!",
		      PG_MSGBTN_OK | PG_MSGBTN_CANCEL))
    pgMessageDialog("Yay!","You clicked 'Ok'",0);

  return 0;
}

int btnExit(struct pgEvent *evt) {
  exit(0);
}

/* a main() to make a little launcher for our demo */
int main(int argc, char *argv[])
{
  pgInit(argc,argv);

  /* Register app, and but a button on it */

  pgRegisterApp(PG_APP_TOOLBAR,"msgdialog demo",0);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Run"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&rundemo,NULL);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("X"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnExit,NULL);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Message dialog test program"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);

  /* Run it */
  pgEventLoop();

  return 0;
}

/* The End */
