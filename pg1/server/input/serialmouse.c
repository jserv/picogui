/* $Id$
 *
 * serialmouse.c - input driver for serial mice.
 *
 *     ------ Editor's Note ------
 * 
 * This driver was contributed by Yunus Altunbicak 
 * <yunus.altunbicak@eliar.com.tr>. He developed it for the uCdimm, but it
 * should work on many platforms. I have made the following changes from the
 * original submission:
 * 
 *   - Added standard pgserver file header
 *   - Renamed driver from tbb6500 to serialmouse
 *   - Added conversions between physical and logical coordinates
 *   - Removed hardcoded screen resolution
 *   - Moved mouse device and acceleration parameters to the configfile
 *   - Return a g_error if it can't open the mouse port
 *   - Register our fd in the select() instead of polling
 *
 * -- Micah
 *
 *     ---------------------------
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
 * Yunus Altunbicak <yunus.altunbicak@eliar.com.tr>  (see below)
 * 
 */

/***************************************************************************
                          serialmouse.c  -  description
                             -------------------
    begin                : Fri Sep 14 2001
    copyright            : (C) 2001 by Yunus Yucel Altunbicak
    email                : yunus.altunbicak@eliar.com.tr

***************************************************************************/

/*  Serial mouse driver for PicoGUI on uCdimm.
    it's so simply and it works fine :) i'm using it with A4 Tech mouse.

    Note:Don't forget put +5V on DTR/RTS of mouse.
    Because uCdimm's serial ports can't be used to provide power to peripherial
    devices such as a mouse. */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/configfile.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/termios.h>

int mouse_fd;
int multiplier;
struct termios options;
struct cursor *serialmouse_cursor;

int serialmouse_fd_activate(int fd) {
  u8 buttons;
  s8 dx,dy;
  static u8 packet[3];
  static int pos;

  if (fd != mouse_fd)
    return 0;

  /* Read a correctly-aligned mouse packet. If the first byte isn't 0x40,
   * it isn't correctly aligned. The mouse packet is 3 or 4 bytes long.
   * On fast machines, we can't read a whole packet at once, so we have
   * to maintain the state in a static variable.
   */
  
  if (read(mouse_fd,packet+pos,1) != 1)
    return 1;
  if (!(packet[0] & 0x40))
    return 1;
  if (pos++ < 2)
    return 1;
  pos = 0;
   
  buttons = ((packet[0] & 0x20) >> 5) | ((packet[0] & 0x10) >> 2);
  dx = ((packet[0] & 0x03) << 6) | (packet[1] & 0x3F);
  dy = ((packet[0] & 0x0C) << 4) | (packet[2] & 0x3F);
  
#ifdef DEBUG_EVENT
  {
    int a,b,c;
    
    a = packet[0];
    b = packet[1];
    c = packet[2];
    printf("fd=%d, dx=%d, dy=%d, buttons=%d packet = %02X %02X %02X\n",
	   mouse_fd,dx,dy,buttons,a,b,c);
  }
#endif    
  
  infilter_send_pointing(PG_TRIGGER_PNTR_RELATIVE, multiplier*dx, 
			 multiplier*dy, buttons, serialmouse_cursor);  
  return 1;
}

g_error serialmouse_init(void) {
  g_error e;

  multiplier = get_param_int("input-serialmouse","multiplier",2);

  mouse_fd = open(get_param_str("input-serialmouse","device","/dev/ttyS0"),
	    O_RDONLY | O_NOCTTY | O_NDELAY);

  if(mouse_fd < 0)
    return mkerror(PG_ERRT_IO,43);   /* Error opening mouse device */

  tcgetattr(mouse_fd, &options);
  cfsetispeed(&options, B1200); /* 1200 baud rates */
  options.c_cflag |= (CLOCAL | CREAD); /* Enable
					  receiver and set the local mode */
  options.c_cflag &= ~PARENB; /* None parity */
  options.c_cflag &= ~CSIZE; /* Mask the character bit
				size */
  options.c_cflag |= CS7; /* 7 data bits */
  options.c_cflag &= ~CSTOPB; /* 1 stop bits */
  options.c_cflag &= ~CRTSCTS;/* Disable hardware flow
				 control */
  options.c_iflag &= ~(IXON | IXOFF | IXANY);/* Disable
						software flow control */
  /* Set raw input and output */
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;
                                                              
  tcsetattr(mouse_fd, TCSANOW, &options); /* set parameters */

  e = cursor_new(&serialmouse_cursor,NULL,-1);
  errorcheck;

  return success;
}

void serialmouse_fd_init(int *n,fd_set *readfds,struct
		     timeval *timeout) {
   if ((*n)<(mouse_fd+1))
     *n = mouse_fd+1;
   if (mouse_fd>0)
     FD_SET(mouse_fd,readfds);
}

void serialmouse_close(void){
  pointer_free(-1,serialmouse_cursor);
  close(mouse_fd);
}

g_error serialmouse_regfunc(struct inlib *i) {
  i->init = &serialmouse_init;
  i->fd_activate = &serialmouse_fd_activate;
  i->fd_init = &serialmouse_fd_init;
  i->close = &serialmouse_close;
  return success;
}
