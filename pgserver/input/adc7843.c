/* $Id: adc7843.c,v 1.2 2002/02/03 03:04:38 micahjd Exp $
 *
 * adc7843.c - input driver for adc7843.c touch screen found on the Psion 5mx
 *             Other touch screens using the same data format should
 *             work too.
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
 *   Thomas A. de Ruiter <thomas@de-ruiter.cx>
 * 
 * 
 */

/*
 * I didn't make the code much cleaner, but it works for me,
 * feel free to improve it ;-)
 *    Tader
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>    /* For sending events */
#include <pgserver/touchscreen.h>

#include <stdio.h>              /* For reading the device */

#define true 1
#define false 0

#define X_RES 640
#define Y_RES 240
#define MIN_X 335
#define MAX_X 3825
#define MIN_Y 330
#define MAX_Y 3490

int adc7843_fd;

/* 
 * For keeping track of the coordinates so we know them to
 * be send with a TRIGGER_UP event...
 */
int ox, oy;

/* Keep track of the pen state... Down or not (up)... */
bool down;

struct tpanel_sample {
	u16 state;
	u16 x;
	u16 y;
};

/******************************************** Implementations */

g_error adc7843_init(void) {
   g_error e;

   e=touchscreen_init();
   errorcheck;
   adc7843_fd = open("/dev/tpanel",O_NONBLOCK);
   if (adc7843_fd <= 0)
     return mkerror(PG_ERRT_IO, 74);
   
   return success;
}

void adc7843_close(void) {
   close(adc7843_fd);
}
   
void adc7843_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
   if ((*n)<(adc7843_fd+1))
     *n = adc7843_fd+1;
   FD_SET(adc7843_fd,readfds);
}

int adc7843_fd_activate(int fd) {
   struct tpanel_sample ts;
   static u8 state;
   char buffer[6];
   int trigger, x, y;
   
   /* Read raw data from the driver */
   if (fd!=adc7843_fd)
     return 0;
   if (read(adc7843_fd,buffer,sizeof(buffer)) < sizeof(buffer))
     return 1;
    
   /* Convert the bytes to unsigned integers... */
   ts.state = ( ( (buffer[0]) << 8 ) + buffer[1] );
   ts.x = ( ( (buffer[2]) << 8 ) + buffer[3] );
   ts.y = ( ( (buffer[4]) << 8 ) + buffer[5] );
   x = ts.x;
   y = ts.y;

   /* Converte to screen coordinates... */
   //   touchscreen_pentoscreen(&x, &y);

   //   this works better!
   x = ( ( ts.x - MIN_X ) * X_RES / ( MAX_X - MIN_X ) );
   y = ( ( ts.y - MIN_Y ) * Y_RES / ( MAX_Y - MIN_Y ) );
   
   /* What type of pointer event? */
   if (ts.state) {
      if (down)
 	     trigger = TRIGGER_MOVE;
      else
	     trigger = TRIGGER_DOWN;
   }
   else {
      if (down)
	     trigger = TRIGGER_UP;
      else
	return 1;
   }
   
   /* If we got this far, accept the new state and send the event */
   state = (trigger != TRIGGER_UP);

   
   if (ts.state)
       down = true;
   else
       down = false;

   if ( trigger == TRIGGER_UP )
      dispatch_pointing(trigger,ox,oy,state);
   else
      dispatch_pointing(trigger,x,y,state);

   /* Save the current coordinates... */
   ox = x;
   oy = y;
   
   
   return 1;
}
   
/******************************************** Driver registration */

g_error adc7843_regfunc(struct inlib *i) {
  i->init = &adc7843_init;
  i->close = &adc7843_close;
  i->message = &touchscreen_message;
  i->fd_activate = &adc7843_fd_activate;
  i->fd_init = &adc7843_fd_init;
  return success;
}

/* The End */
