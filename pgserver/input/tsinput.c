/* $Id: tsinput.c,v 1.1 2001/02/23 14:56:58 pney Exp $
 *
 * tsinput.h - input driver for touch screen
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Author:
 *   Philippe Ney <philippe.ney@smardata.ch>
 * 
 * Contributors:
 * 
 * 
 */

#include <pgserver/common.h>

#ifdef DRIVER_TSINPUT

#include <pgserver/input.h>
#include <pgserver/widget.h>    /* for dispatch_pointing */
#include <pgserver/pgnet.h>

#include <pgserver/mc68328digi.h>


#define POLL_USEC 100

#define DEVICE_FILE_NAME  "/dev/ts"


static int fd=0;
static int isTSopen=0;
static int  bytes_transfered=0;

/******************************************** Implementations */

static int conv_xy(int inval, int isx) {
  inval -= 400;
  inval  *= 320;
  if(isx) inval  /= 3500;
  else    inval  /= 3400;
  return inval;
}


void tsinput_poll(void) {
  struct ts_pen_info pen_info;
  
  pen_info.x = -1; pen_info.y = -1;
  
  bytes_transfered=read(fd,(char *)&pen_info,sizeof(pen_info));

  if(pen_info.x != -1) {
    pen_info.x = conv_xy(pen_info.x,1);
    pen_info.y = conv_xy(pen_info.y,0);

  switch(pen_info.event) {
  case EV_PEN_UP:
    dispatch_pointing(TRIGGER_UP,pen_info.x,pen_info.y,0);
    break;
    
  case EV_PEN_DOWN:
    dispatch_pointing(TRIGGER_DOWN,pen_info.x,pen_info.y,1);
    break;
    
  case EV_PEN_MOVE:
    dispatch_pointing(TRIGGER_MOVE,pen_info.x,pen_info.y,0);
    break;
  }

//  printf("(%i,%i)\n",pen_info.x,pen_info.y);
    
  }
}


g_error tsinput_init(void) {
  fd = open(DEVICE_FILE_NAME,O_RDWR | O_NONBLOCK);
  if(fd < 0) {
    printf("Can't open device file: %s\n",DEVICE_FILE_NAME);
    printf("Error: %s\n",strerror(errno));
    return -1;
  }
  else {
    printf("Device %s open\n");
    isTSopen=1;
    return 0;
  }
}


void tsinput_close(void) {
  close(fd);
  printf("Device %s closed.\n",DEVICE_FILE_NAME);
  isTSopen=0;
}


/* Polling time for the input driver */ 
void tsinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  timeout->tv_sec = 0;
  timeout->tv_usec = POLL_USEC;
}

/******************************************** Driver registration */

g_error tsinput_regfunc(struct inlib *i) {
  i->init = &tsinput_init;
  i->close = &tsinput_close;
  i->poll = &tsinput_poll;
  i->fd_init = &tsinput_fd_init;
  return sucess;
}

#endif /* DRIVER_TSINPUT */
/* The End */
