/* $Id$
 *
 * adc7843.c - input driver for adc7843.c touch screen found on the Psion 5mx
 *             Other touch screens using the same data format should
 *             work too.
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

#include <stdio.h>              /* For reading the device */

#define true 1
#define false 0

struct cal {
	int x_res, y_res;
	int min_x, max_x;
	int min_y, max_y;
};

/* psion revo/revo+/mako */

static struct cal revo =  {480, 160, 370, 3900, 330, 3220};

/* psion series 5mx */

static struct cal ps5mx = {640, 240, 335, 3825, 330, 3490};

static struct cal *cal = NULL;

int adc7843_fd;

/******************************************** Implementations */

g_error adc7843_init(void) {
   adc7843_fd = open("/dev/tpanel",O_NONBLOCK);

   /* devfs name */

   if (adc7843_fd <= 0)
      adc7843_fd = open("/dev/misc/adc7843", O_NONBLOCK);

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
   int state;
   char buffer[6];
   int trigger;
   static int x=0, y=0;

   if (!cal) {
      if (!vid)
	 return 0;
      else if (vid->xres == 480)
	 cal = &revo;
      else
	 cal = &ps5mx;
   }
   
   /* Read raw data from the driver */
   if (fd!=adc7843_fd)
     return 0;
   if (read(adc7843_fd,buffer,sizeof(buffer)) < sizeof(buffer))
     return 1;
    
   /* Convert the bytes to unsigned integers... */
   state = ( ( (buffer[0]) << 8 ) + buffer[1] );

   /* save x,y in static variables. when pen up, send the last known
      coordinates */

   if (state) { 
      x = ( ( (buffer[2]) << 8 ) + buffer[3] );
      y = ( ( (buffer[4]) << 8 ) + buffer[5] );

      /* Convert to screen coordinates... */
      x = ( ( x - cal->min_x ) * cal->x_res / ( cal->max_x - cal->min_x ) );
      y = ( ( y - cal->min_y ) * cal->y_res / ( cal->max_y - cal->min_y ) );
   }
   
   infilter_send_pointing(PG_TRIGGER_PNTR_STATUS,x,y,state,NULL);

   return 1;
}
   
/******************************************** Driver registration */

g_error adc7843_regfunc(struct inlib *i) {
  i->init = &adc7843_init;
  i->close = &adc7843_close;
  i->fd_activate = &adc7843_fd_activate;
  i->fd_init = &adc7843_fd_init;
  return success;
}

/* The End */
