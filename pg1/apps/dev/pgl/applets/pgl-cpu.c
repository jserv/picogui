/* $Id$
 * 
 * pgl-cpu.c - PGL applet to display a simple CPU monitor
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
#include <stdio.h>       /* file IO for getting CPU load */

pghandle wLoad;

void sysIdle(void) {
  char *ct;
  char buf[50];
  FILE *f;
  unsigned long cpu_user,cpu_nice,cpu_sys,cpu_idle;
  unsigned long crun,ctotal,wval;
  static unsigned long ocrun = 0,octotal = 1,owval = 0;

  /* Get CPU load */
  f = fopen("/proc/stat","r");
  fgets(buf,50,f);
  fclose(f);
  sscanf(buf,"cpu %lu %lu %lu %lu",&cpu_user,&cpu_nice,&cpu_sys,&cpu_idle);  
  crun = cpu_user + cpu_sys;
  ctotal = crun + cpu_nice + cpu_idle;
  if (crun==ocrun || ctotal==octotal)
    wval = 0;
  else
    wval = (crun-ocrun) * 100 / (ctotal-octotal);

  if (wval!=owval) {
    pgSetWidget(wLoad,PG_WP_VALUE,wval,0);
    pgSubUpdate(wLoad);
  }

  owval = wval;
  ocrun = crun;
  octotal = ctotal;
}

int main(int argc, char **argv) {
  pghandle wPGLbar,wLoadbox;
  pgInit(argc,argv);

  /* Find the applet container */
  wPGLbar = pgFindWidget("PGL-AppletBar");
  if (!wPGLbar) {
    pgMessageDialog(argv[0],"This applet requires PGL",PG_MSGICON_ERROR);
    return 1;
  }
 
  wLoadbox = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,wPGLbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);

  pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,wLoadbox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("CPU"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);

  wLoad = pgNewWidget(PG_WIDGET_INDICATOR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);  

  /* Run it. */
  pgSetIdle(100,&sysIdle);
  pgEventLoop();
   
  return 0;
}
   
/* The End */
