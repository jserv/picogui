/* $Id$
 * 
 * pgmon.c  - Network monitor, based on lcdnet.c, from lcdclock application
 *
 * - This is very simple and has no error checking!
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
 *               Olivier Bornet, SMARTDATA
 * 
 * 
 * 
 */

#include <picogui.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define NET_PROC_FILE "/proc/net/dev"
#define DEFAULT_NETWORK_INTERFACE "eth0"
#define IDLE_TIME     500
#define SAMPLE_TIME   1000

pghandle wTXlabel, wTX, wRXlabel, wRX, wDeviceName;
short bDisplayTotal = 0;

void sysIdle(void) {
  FILE *net;
  char buf[256];
  char *p;
  float rx_bytes, tx_bytes;
  float rx_kbps, tx_kbps;
  float elapsed;
  static float max_kbps = 0, rx_bytes_old = 0, tx_bytes_old = 0;
  struct timeval now;
  static struct timeval then = {0,0};
  static int first_sample = 1;

  /* Read transmitted and received bytes from the kernel
   */
  net = fopen(NET_PROC_FILE,"r");
  while (!feof(net)) {
    fgets(buf,256,net);
    if (strstr (buf,pgGetString (pgGetWidget (wDeviceName, PG_WP_TEXT))))
      if (p = strchr(buf,':')) {
	sscanf(p+1, "%f %*f %*f %*f %*f %*f %*f %*f %f", &rx_bytes, &tx_bytes);
      }   
  }
  fclose(net);


  /* Time for the next sample?
   */

  gettimeofday(&now,NULL);
  if ( first_sample || (now.tv_sec-then.tv_sec)*1000 + 
       (now.tv_usec-then.tv_usec)/1000 >= SAMPLE_TIME ) {
    if (!first_sample) {
      
      /* Calculations
       */
      elapsed = (now.tv_sec - then.tv_sec) + (now.tv_usec - then.tv_usec)/1000000.0;
      tx_kbps = (tx_bytes - tx_bytes_old) / elapsed / 1024.0;
      rx_kbps = (rx_bytes - rx_bytes_old) / elapsed / 1024.0;
      
      if (tx_kbps > max_kbps) max_kbps = tx_kbps;
      if (rx_kbps > max_kbps) max_kbps = rx_kbps;
      
      /* Update PicoGUI display
       */
      pgSetWidget(wTX, PG_WP_VALUE, (int)( 100*tx_kbps/max_kbps ), 0);
      pgSetWidget(wRX, PG_WP_VALUE, (int)( 100*rx_kbps/max_kbps ), 0);
      
      /* display either the total or the current traffic */
      if (bDisplayTotal) {
	pgReplaceTextFmt(wRXlabel, "%.3f KB", rx_bytes/1024);
	pgReplaceTextFmt(wTXlabel, "%.3f KB", tx_bytes/1024);
      } else {
	pgReplaceTextFmt(wRXlabel, "%.4f KB/s", rx_kbps);
	pgReplaceTextFmt(wTXlabel, "%.4f KB/s", tx_kbps);
      }
    }
    then = now;
    rx_bytes_old = rx_bytes;
    tx_bytes_old = tx_bytes;
    first_sample = 0;
  }
  pgUpdate();
}

int selectInterface (struct pgEvent *evt) {
  /* change the interface to monitor */
  pghandle str;

  str = pgInputDialog ("Interface selection",
		       "Which interface you want to monitor ?",
		       pgGetWidget (wDeviceName, PG_WP_TEXT));

  if (str) {
    /* Normally we would use pgReplaceText, but we already have a
     * handle so we can save a step. */
    
    pgDelete (pgGetWidget( wDeviceName, PG_WP_TEXT));
    pgSetWidget (wDeviceName, PG_WP_TEXT, str, 0);

  }

  return 0;

}

int changeDisplay (struct pgEvent *evt) {
  /* toggle the display between the total and the current load */

  bDisplayTotal = ! bDisplayTotal;
  
  return 0;

}

int main(int argc, char **argv) {
  pghandle wAllBox, wTXbox, wRXbox;

  /* initialise and register the application */
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Network Monitor",0);
  
  /* create a box for putting all the ting inside it */
  wAllBox = pgNewWidget(PG_WIDGET_BOX,0,0);

  /* create a widget which have the name of the interface to monitor */
  wDeviceName = pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_INSIDE, wAllBox);
  pgSetWidget (PGDEFAULT,
	       //PG_WP_DIRECTION,PG_DIR_VERTICAL,
	       PG_WP_SIDE, PG_S_RIGHT,
	       PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN,
	       0);
  pgReplaceTextFmt (wDeviceName, "%s", DEFAULT_NETWORK_INTERFACE);
  pgBind (PGDEFAULT, PG_WE_PNTR_DOWN, &selectInterface, NULL);

  /* create the widgets for monitor the RX traffic */
  wRXlabel = pgNewWidget(PG_WIDGET_BUTTON, 0, 0);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_TOP,
	       PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN,
	       0);
  pgBind (PGDEFAULT, PG_WE_PNTR_DOWN, &changeDisplay, NULL);
  wRXbox = pgNewWidget(PG_WIDGET_BOX, 0,0);

  /* create the widgets for monitor the TX traffic */
  wTXlabel = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget (PGDEFAULT,
	       PG_WP_SIDE, PG_S_TOP,
	       PG_WP_EXTDEVENTS, PG_EXEV_PNTR_DOWN,
	       0);
  pgBind (PGDEFAULT, PG_WE_PNTR_DOWN, &changeDisplay, NULL);
  wTXbox = pgNewWidget(PG_WIDGET_BOX,0,0);

  /* add the RX gauge */
  pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, wRXbox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("RX"),
	      PG_WP_SIDE, PG_S_RIGHT,
	      0);
  wRX = pgNewWidget(PG_WIDGET_INDICATOR, 0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      0);

  /* add the TX gauge */
  pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, wTXbox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("TX"),
	      PG_WP_SIDE, PG_S_RIGHT,
	      0);
  wTX = pgNewWidget(PG_WIDGET_INDICATOR, 0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      0);

  /* put the monitoring in the idle time */
  pgSetIdle(IDLE_TIME,&sysIdle);

  /* the main loop */
  pgEventLoop();

  /* bye */
  return 0;

}
   
/* The End */
