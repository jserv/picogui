/* $Id$
 * 
 * pgl-battery-helio.c - Battery monitor applet for Helio
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
#include <stdio.h>       /* file IO for getting battery info */

pghandle wBatt;

void sysIdle(void) {
  FILE *f;
  char buf[50];
  int bat_raw, bat_percent;
  
  /* Get battery status */
  f = fopen("/proc/r39xxpm","r");
  if (f) {
    /* I don't think the format of /proc/r39xxpm is set in stone, so
     * this might break? Oh well, works for now. */
    fgets(buf,50,f);  /* CPU speed */
    fgets(buf,50,f);  /* Battery voltage */
    fclose(f);
    bat_raw = atoi(buf+9); /* Skip past "Battery: " */
    
    /* Translate to percent, assuming 259 (2.0 volts) is empty
     * and 402 (3.0 volts) is full. */
    bat_percent = ((bat_raw - 259) * 100) / 143;
    if (bat_percent < 0) bat_percent = 0;
    if (bat_percent > 100) bat_percent = 100;
    
    pgSetWidget(wBatt,PG_WP_VALUE,bat_percent,0);
    pgFlushRequests();
    pgSubUpdate(wBatt);  
  }
}

int main(int argc, char **argv) {
  pghandle wPGLbar,wBattbox;
  pgInit(argc,argv);

  /* Find the applet container */
  wPGLbar = pgFindWidget("PGL-AppletBar");
  if (!wPGLbar) {
    pgMessageDialog(argv[0],"This applet requires PGL",PG_MSGICON_ERROR);
    return 1;
  }
 
  wBattbox = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,wPGLbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);

  pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wBattbox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Batt"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);

  wBatt = pgNewWidget(PG_WIDGET_INDICATOR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);  

  /* Run it. */
  pgSetIdle(4000,&sysIdle);
  pgEventLoop();
   
  return 0;
}
   
/* The End */
