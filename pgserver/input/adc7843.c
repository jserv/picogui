/* $Id: adc7843.c,v 1.5 2002/05/22 10:01:20 micahjd Exp $
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

/* 
 * For keeping track of the coordinates so we know them to
 * be send with a PG_TRIGGER_UP event...
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
   ts.state = ( ( (buffer[0]) << 8 ) + buffer[1] );
   ts.x = ( ( (buffer[2]) << 8 ) + buffer[3] );
   ts.y = ( ( (buffer[4]) << 8 ) + buffer[5] );
   x = ts.x;
   y = ts.y;

   /* Converte to screen coordinates... */
   x = ( ( ts.x - cal->min_x ) * cal->x_res / ( cal->max_x - cal->min_x ) );
   y = ( ( ts.y - cal->min_y ) * cal->y_res / ( cal->max_y - cal->min_y ) );
   
   /* What type of pointer event? */
   if (ts.state) {
      if (down)
 	     trigger = PG_TRIGGER_MOVE;
      else
	     trigger = PG_TRIGGER_DOWN;
   }
   else {
      if (down)
	     trigger = PG_TRIGGER_UP;
      else
	return 1;
   }
   
   /* If we got this far, accept the new state and send the event */
   state = (trigger != PG_TRIGGER_UP);

   
   if (ts.state)
       down = true;
   else
       down = false;

   if ( trigger == PG_TRIGGER_UP )
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
  i->fd_activate = &adc7843_fd_activate;
  i->fd_init = &adc7843_fd_init;
  return success;
}

/* The End */
