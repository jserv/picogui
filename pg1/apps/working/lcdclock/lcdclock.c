/* $Id$
 * 
 * lcdclock.c - Clock application designed for use with a wall-mounted
 *              LCD (see README) but maybe with other uses
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

#include <time.h>        /* For clock */
#include <picogui.h>

pghandle wBigClock,wSmallClock;

void sysIdle(void) {
   time_t now;
   struct tm *ltime;
   char buf[40];
   static char blink = 0;

   time(&now);                         /* Get time */
   ltime = localtime(&now);
   strftime(buf,40,"%l:%M",ltime);     /* Large hours and minutes */
   if (blink) buf[2] = ' ';
   blink = !blink;                     /* blinking colon */
   pgReplaceText(wBigClock,buf);
   strftime(buf,40,":%S \n%p\n%b\n%e",  /* Small seconds, AM/PM, and date */
	    ltime);
   pgReplaceText(wSmallClock,buf);
   pgFlushRequests();                  /* update screen */
   pgUpdate();
}

int main(int argc, char **argv) {
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Clock",0);

   /* Small text along the side */
   wSmallClock = pgNewWidget(PG_WIDGET_LABEL,0,0);
   pgSetWidget(PGDEFAULT,
	       PG_WP_SIDE,PG_S_RIGHT,
	       0);

   /* Big text */
   wBigClock = pgNewWidget(PG_WIDGET_LABEL,0,0);
   pgSetWidget(PGDEFAULT,
	       PG_WP_SIDE,PG_S_ALL,
	       PG_WP_FONT,pgNewFont(NULL,30,PG_FSTYLE_FIXED),
	       0);
   
   pgSetIdle(250,&sysIdle);
   pgEventLoop();
   return 0;
}
   
/* The End */
