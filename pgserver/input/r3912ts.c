/* $Id: r3912ts.c,v 1.7 2002/01/16 19:47:26 lonetech Exp $
 *
 * r3912ts.c - input driver for r3912 touch screen found on the VTech Helio
 *             and others. Other touch screens using the same data format should
 *             work too.
 *
 * Much of this code is copied from Jay Carlson's Helio patches for the W window
 * system, available at vhl-tools.sourceforge.net
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>    /* For sending events */
#include <pgserver/touchscreen.h>

#include <stdio.h>              /* For reading the device */

int r3912ts_fd;

struct tpanel_sample {
	unsigned short state;
	unsigned short x;
	unsigned short y;
};

/******************************************** Implementations */

g_error r3912ts_init(void) {
   g_error e;

   e=touchscreen_init();
   errorcheck;
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
   struct tpanel_sample ts;
   static u8 state = 0;
   int trigger, x, y;
   
   /* Read raw data from the driver */
   if (fd!=r3912ts_fd)
     return 0;
   if (read(r3912ts_fd,&ts,sizeof(ts)) < sizeof(ts))
     return 1;
   
   /* Convert to screen coordinates */
   x = ts.x;
   y = ts.y;
   touchscreen_pentoscreen(&x, &y);

   /* What type of pointer event? */
   if (ts.state) {
      if (state)
	trigger = TRIGGER_MOVE;
      else
	trigger = TRIGGER_DOWN;
   }
   else {
      if (state)
	trigger = TRIGGER_UP;
      else
	return 1;
   }
   
   /* If we got this far, accept the new state and send the event */
   state = (trigger != TRIGGER_UP);
   dispatch_pointing(trigger,x,y,state);
   
   return 1;
}
   
/******************************************** Driver registration */

g_error r3912ts_regfunc(struct inlib *i) {
  i->init = &r3912ts_init;
  i->close = &r3912ts_close;
  i->message = &touchscreen_message;
  i->fd_activate = &r3912ts_fd_activate;
  i->fd_init = &r3912ts_fd_init;
  return success;
}

/* The End */
