/* $Id$
 * 
 * lcdnet.c - Network monitor designed for use with a wall-mounted
 *            LCD (see README) but maybe with other uses
 *
 * - This is very simple and has no error checking! Also, the interface
 *   it uses can only be changed at compiletime.
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

#include <picogui.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define NET_PROC_FILE "/proc/net/dev"
#define NET_IFACE     "eth0"
#define IDLE_TIME     500
#define SAMPLE_TIME   1000

pghandle wTXlabel, wTX, wRXlabel, wRX;

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
    if (strstr(buf,NET_IFACE))
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
      pgReplaceTextFmt(wRXlabel, "%.4f KB/s", rx_kbps);
      pgReplaceTextFmt(wTXlabel, "%.4f KB/s", tx_kbps);
      
    }
    then = now;
    rx_bytes_old = rx_bytes;
    tx_bytes_old = tx_bytes;
    first_sample = 0;
  }
}

int main(int argc, char **argv) {
  pghandle wTXbox, wRXbox;
  
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Network Monitor",0);
  
  wRXlabel = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TRANSPARENT, 0,
	      0);

  wRXbox = pgNewWidget(PG_WIDGET_BOX,0,0);

  wTXlabel = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TRANSPARENT, 0,
	      0);

  wTXbox = pgNewWidget(PG_WIDGET_BOX,0,0);
  
  pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, wRXbox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("RX"),
	      PG_WP_SIDE, PG_S_RIGHT,
	      0);

  wRX = pgNewWidget(PG_WIDGET_INDICATOR, 0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      0);


  pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, wTXbox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("TX"),
	      PG_WP_SIDE, PG_S_RIGHT,
	      0);

  wTX = pgNewWidget(PG_WIDGET_INDICATOR, 0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      0);

  pgSetIdle(IDLE_TIME,&sysIdle);
  pgEventLoop();
  return 0;
}
   
/* The End */
