/* $Id$
 *
 * vr3ts.c - input driver for the Agenda VR3. This contains code from
 *           Agenda's xfree86 patch along with the framework from
 *           the r3912ts driver.
 *
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 *
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>    /* For sending events */

#include <stdio.h>              /* For reading the device */

#include <linux/tpanel.h>

int vr3ts_fd;

//Stuff from the xfree86 diff that Agenda made
int scan_interval = 20000; //default scan interval in microseconds (50 Hz)
int scan_settle_time = 480; //default settle time in microseconds
int low_z_limit = 800; //ignore z measurement data below this value

/******************************************** Implementations */

static int VrTpanelInit() {
  /*
   * Open up the touch-panel device.
   * Return the fd if successful, or negative if unsuccessful.
   */

  struct scanparam s;
  int result;
  int fd;

  /* Open the touch-panel device. */
  fd = open("/dev/tpanel", O_NONBLOCK);
  if(fd < 0) {
    fprintf(stderr, "Error %d opening touch panel\n", errno);
    return -1;
  }

  s.interval = scan_interval;
  s.settletime = scan_settle_time;
  result = ioctl(fd, TPSETSCANPARM, &s);
  if(result < 0 )
    fprintf(stderr, "Error %d, result %d setting scan parameters.\n", result, errno);

  return fd;
}

g_error vr3ts_init(void) {
  vr3ts_fd = VrTpanelInit();
  if (vr3ts_fd <= 0)
    return mkerror(PG_ERRT_IO, 74);
   
  return success;
}

void vr3ts_close(void) {
  close(vr3ts_fd);
}
   
void vr3ts_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if ((*n)<(vr3ts_fd+1))
    *n = vr3ts_fd+1;
  FD_SET(vr3ts_fd,readfds);
}

int vr3ts_fd_activate(int fd) {
  short data[6], data_x, data_y, data_z;
  int bytes_read;

  /* Is this for us? */
  if (fd!=vr3ts_fd)
    return 0;

  /* read a data point */
  bytes_read = read(fd, data, sizeof(data));
  if(bytes_read != sizeof(data))
    return 1;

  /* did we lose any data? */
  if((data[0] & 0x2000))
    fprintf(stderr, "Lost touch panel data\n");
  
  /*
   * Combine the complementary panel readings into one value (except z)
   * This effectively doubles the sampling freqency, reducing noise by approx 3db.
   * Again, please don't quote the 3db figure.
   * I think it also cancels out changes in the overall resistance of the panel
   * such as may be caused by changes in panel temperature.
   */
  data_x = data[2] - data[1];
  data_y = data[4] - data[3];
  data_z = data[5];

  infilter_send_touchscreen(data_x, data_y, data_x, data_z > low_z_limit);

  return 1;
}
   
/******************************************** Driver registration */

g_error vr3ts_regfunc(struct inlib *i) {
  i->init = &vr3ts_init;
  i->close = &vr3ts_close;
  i->fd_activate = &vr3ts_fd_activate;
  i->fd_init = &vr3ts_fd_init;
  return success;
}

/* The End */
