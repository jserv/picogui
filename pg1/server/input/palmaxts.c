/* $Id$
 *
 * palmaxts.c - input driver for Palmax touchscreens.
 *
 * derived from serialmouse.c
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
                          palmaxts.c  -  description
                             -------------------
    begin                : Fri Sep 14 2001
    copyright            : (C) 2001 by Yunus Yucel Altunbicak
    email                : yunus.altunbicak@eliar.com.tr

***************************************************************************/

/*  Serial touchscreen driver for PicoGUI on Palmax PD-1x00. */

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
#include <pgserver/touchscreen.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/termios.h>

int palmaxts_fd;
struct termios options;

int palmaxts_fd_activate(int fd) {
  u8 buttons;
  u8 packet[5];
  static int cursorx,cursory;

  if (fd != palmaxts_fd)
    return 0;

  /* Read a correctly-aligned touchscreen packet. If the first byte isn't 0xff,
   * it isn't correctly aligned. The touchscreen packet is 3 or 5 bytes long.
   */
  
  if (!read(palmaxts_fd,packet,1))
    return 1;
  if (!(packet[0] & 0xff))
    return 1;
  if (!read(palmaxts_fd,packet+1,2))
    return 1;
  if (packet[1]!=0xfe)
    {
      if (!read(palmaxts_fd,packet+3,2))
        return 1;
      if (packet[1]&0x3f || packet[3]&0x3f)
	return 1;	/* malformed */
    }
  else
    {
      if(packet[2]!=0xfe)
	return 1;	/* malformed */
    }

  infilter_send_touchscreen((packet[1]>>6|(s16)packet[2]<<2),
			    (packet[3]>>6|(s16)packet[4]<<2),
			    0,
			    packet[1]!=0xFE);
  return 1;
}

g_error palmaxts_init(void) {
  palmaxts_fd = open(get_param_str("input-palmaxts","device","/dev/ttyS0"),
	    O_RDONLY | O_NOCTTY | O_NDELAY);

  if(palmaxts_fd < 0)
    return mkerror(PG_ERRT_IO,43);   /* Error opening touchscreen device */

  tcgetattr(palmaxts_fd, &options);
  cfsetispeed(&options, B19200); /* 19200 baud rates */
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

  tcsetattr(palmaxts_fd, TCSANOW, &options); /* set parameters */
  return success;
}

void palmaxts_fd_init(int *n,fd_set *readfds,struct
		     timeval *timeout) {
   if ((*n)<(palmaxts_fd+1))
     *n = palmaxts_fd+1;
   if (palmaxts_fd>0)
     FD_SET(palmaxts_fd,readfds);
}

void palmaxts_close(void){
  close(palmaxts_fd);
}

g_error palmaxts_regfunc(struct inlib *i) {
  i->init = &palmaxts_init;
  i->fd_activate = &palmaxts_fd_activate;
  i->fd_init = &palmaxts_fd_init;
  i->close = &palmaxts_close;
  return success;
}
