/* $Id: pgl-clock.c,v 1.1 2001/08/09 19:10:23 micahjd Exp $
 * 
 * pgl-clock.c - This is a simple clock applet for PGL
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <time.h>

pghandle wClock;

/* Update the clock */
void sysIdle(void) {
   time_t now;
   struct tm *ltime;
   char buf[20];
   time(&now);
   ltime = localtime(&now);
   strftime(buf,40,"%l:%M %p",ltime);
   pgReplaceText(wClock,buf);
   pgSubUpdate(wClock);
}

int main(int argc,char **argv) {
  pghandle wPGLbar;
  pgInit(argc,argv);

  /* Find the applet container */
  wPGLbar = pgFindWidget("PGL-AppletBar");
  if (!wPGLbar) {
    pgMessageDialog(argv[0],"This applet requires PGL",PG_MSGICON_ERROR);
    return 1;
  }
 
  /* Clock widget */
  wClock = pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wPGLbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TRANSPARENT,0,
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);

  /* Update the clock every 30 seconds */
  pgSetIdle(30000,&sysIdle);
  pgEventLoop();
  return 0;
}

/* The End */




