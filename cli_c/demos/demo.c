/* $Id: demo.c,v 1.5 2000/09/21 17:34:37 pney Exp $
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
  pghandle tb,app;
  pghandle w1,w2;
  pghandle t1,t2,t3,t4;
  char str[500];
  
  pgInit(argc,argv);

  /* just for testing... (normally an app
   * only has one pgRegisterApp call) */
/*  pgRegisterApp(PG_APP_NORMAL,"foo",
		PG_APPSPEC_WIDTH,100,
		PG_APPSPEC_SIDE,PG_S_LEFT,
		0);
*/

  strcpy(str,"Mobile internet appliances\n");
  strcat(str,"will likely be the smart merge\n");
  strcat(str,"of computer power, multimedia,\n");
  strcat(str,"Internet, wireless telecommunication\n");
  strcat(str,"technology and fashion; this is the\n");
  strcat(str,"reason why this market is to be\n");
  strcat(str,"considered as one of the major fast\n");
  strcat(str,"growing markets at the beginning\n");
  strcat(str,"of this millennium.\n");
  t1 = pgNewString(" 1 ");
  t2 = pgNewString(" 2 ");
  t3 = pgNewString(" 3 ");
  t4 = pgNewString(str); 



  tb = pgRegisterApp(PG_APP_TOOLBAR,"Demo app",0);

  app = pgRegisterApp(PG_APP_NORMAL,"foo",
		       PG_APPSPEC_WIDTH,100,
		       PG_APPSPEC_SIDE,PG_S_LEFT,0);

printf("------------------------>>> 1\n");

  w1 = pgNewWidget(PG_WIDGET_BUTTON,0,tb);
  pgSetWidget(w1,PG_WP_ALIGN,PG_A_LEFT,PG_WP_TEXT,t1,0);
  w1 = pgNewWidget(PG_WIDGET_BUTTON,0,tb);
  pgSetWidget(w1,PG_WP_ALIGN,PG_A_LEFT,PG_WP_TEXT,t2,0);
  w1 = pgNewWidget(PG_WIDGET_BUTTON,0,tb);
  pgSetWidget(w1,PG_WP_ALIGN,PG_A_LEFT,PG_WP_TEXT,t3,0);

printf("------------------------>>> 2\n");

  w1 = pgNewWidget(PG_WIDGET_BOX,0,app);
  pgSetWidget(w1,PG_WP_SIZE,100,PG_WP_BORDERCOLOR,0x000000,0);
printf("------------------------>>> 3\n");
  w2 = pgNewWidget(PG_WIDGET_LABEL,0,w1);
  pgSetWidget(w2,PG_WP_TEXT,t4,0);
printf("------------------------>>> 4\n");
/*  w1 = pgNewWidget(PG_WIDGET_SCROLL,0,0);
  pgSetWidget(w1,PG_WP_BIND,w2,0);
*/
  pgEventLoop();

  return 0;
}





/* The End */
