/* $Id$
 *
 * r3912ts.c - input driver for r3912 touch screen found on the VTech Helio
 *             and others. Other touch screens using the same data format should
 *             work too.
 *
 * Much of this code is copied from Jay Carlson's Helio patches for the W window
 * system, available at vhl-tools.sourceforge.net
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int r3912ts_fd;

struct r3912_sample {
  u16 state;
  u16 x;
  u16 y;
};

g_error r3912ts_init(void) {
  r3912ts_fd = open("/dev/tpanel",O_NONBLOCK);
  if (r3912ts_fd <= 0)
    return mkerror(PG_ERRT_IO, 74);
 
  return success;
}

void r3912ts_close(void) {
  close(r3912ts_fd);
}

void r3912ts_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if ((*n)<(r3912ts_fd+1))
    *n = r3912ts_fd+1;
  FD_SET(r3912ts_fd,readfds);
}

int r3912ts_fd_activate(int fd) {
  struct r3912_sample ts;
  
  /* Read raw data from the driver */
  if (fd!=r3912ts_fd)
    return 0;
  if (read(r3912ts_fd,&ts,sizeof(ts)) < sizeof(ts))
    return 1;
  
  infilter_send_touchscreen(ts.x, ts.y, ts.state, ts.state);
  return 1;
}

g_error r3912ts_regfunc(struct inlib *i) {
  i->init = &r3912ts_init;
  i->close = &r3912ts_close;
  i->fd_activate = &r3912ts_fd_activate;
  i->fd_init = &r3912ts_fd_init;
  return success;
}

/* The End */
