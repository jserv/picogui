/* $Id$
 *
 * sc3ts.c - input driver for sc3 compatible touch screens
 *
 * (C) 2001 SSV Embedded Systems GmbH, Arnd Bergmann <abe@ist1.de>
 *
 * Based on serial mouse driver, see serialmouse.c for complete
 * changelog
 *
 *     ---------------------------
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 ***************************************************************************/

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/configfile.h>
#include <pgserver/touchscreen.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/termios.h>

int sc3_fd;
struct termios options;

struct sc3_event {
  u8 y_high    : 3;    /* lower half of horizontal position */
  u8 x_high    : 3;    /* lower half of vertical position */
  u8 pressure  : 1;    /* button state */
  u8 sync      : 1;    /* set to 1 on first byte, 0 otherwise */
  u8 x_low;            /* upper half of horizontal position */
  u8 y_low;            /* upper half of vertical position */
  u8 pad;              /* pad to 32 bit */
};                                                                                    
int sc3_fd_activate(int fd) {
  u8 buttons;
  static u8 packet[3];
  static int pos;
  struct sc3_event *event = (struct sc3_event *)packet;
  int cursorx,cursory;

  if (fd != sc3_fd)
    return 0;

  /* Read a correctly-aligned sc3 packet. If the first byte isn't 0x80,
   * it isn't correctly aligned. The sc3 packet is 3 bytes long.
   */
  
  if (read(sc3_fd,packet+pos,1) != 1)
    return 1;
  if (!(packet[0] & 0x80))
    return 1;
  if (pos++ < 2)
    return 1;
  pos = 0;

  buttons = event->pressure;
  cursorx = event->x_high << 7 | event->x_low;
  cursory = event->y_high << 7 | event->y_low;

#ifdef DEBUG_EVENT
  {
    int a,b,c;

    a = packet[0];
    b = packet[1];
    c = packet[2];
    printf("fd=%d, x=%4d, y=%4d, buttons=%2d packet = %02X %02X %02X\n",
	   sc3_fd,cursorx,cursory,buttons,a,b,c);
  }
#endif    
  
  infilter_send_touchscreen(cursorx,cursory,pressure,pressure);
  
  return 1;
}

g_error sc3_init(void) {
  sc3_fd = open(get_param_str("input-sc3ts","device","/dev/ttyS1"),
		O_RDONLY | O_NOCTTY | O_NDELAY);

  if(sc3_fd < 0)
    return mkerror(PG_ERRT_IO,43);   /* Error opening sc3 device */

  tcgetattr(sc3_fd, &options);
  cfsetispeed(&options, B2400); /* baud rate 2400 */
  options.c_cflag |= (CLOCAL | CREAD); /* Enable
					  receiver and set the local mode */
  options.c_cflag &= ~PARENB; /* None parity */
  options.c_cflag &= ~CSIZE; /* Mask the character bit
				size */
  options.c_cflag |= CS8; /* 8 data bits */
  options.c_cflag &= ~CSTOPB; /* 1 stop bits */
  options.c_cflag &= ~CRTSCTS;/* Disable hardware flow
				 control */
  options.c_iflag &= ~(IXON | IXOFF | IXANY);/* Disable
						software flow control */
  /* Set raw input and output */
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;

  tcsetattr(sc3_fd, TCSANOW, &options); /* set parameters */
  return success;
}

void sc3_fd_init(int *n,fd_set *readfds,struct
		     timeval *timeout) {
   if ((*n)<(sc3_fd+1))
     *n = sc3_fd+1;
   if (sc3_fd>0)
     FD_SET(sc3_fd,readfds);
}

void sc3_close(void){
  close(sc3_fd);
}

g_error sc3_regfunc(struct inlib *i) {
  i->init = &sc3_init;
  i->fd_activate = &sc3_fd_activate;
  i->fd_init = &sc3_fd_init;
  i->close = &sc3_close;
  return success;
}

/* The End */
