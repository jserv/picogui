/* $Id: tsinput.c,v 1.3 2001/03/19 22:58:15 bauermeister Exp $
 *
 * tsinput.c - input driver for touch screen
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
 *   Pascal Bauermeister <pascal.bauermeister@smardata.ch>
 * 
 */

#include <pgserver/common.h>

#ifdef DRIVER_TSINPUT

#include <pgserver/input.h>
#include <pgserver/widget.h>    /* for dispatch_pointing */
#include <pgserver/pgnet.h>

#include <pgserver/mc68328digi.h>


#define POLL_USEC 100

static const char *DEVICE_FILE_NAME = "/dev/ts";
static const char *_file_ = __FILE__; 

static int fd=0;
static int bytes_transfered=0;

/******************************************** Implementations */

void tsinput_poll(void) {
  struct ts_pen_info pen_info;
  
  pen_info.x = -1; pen_info.y = -1;
  
  bytes_transfered=read(fd,(char *)&pen_info,sizeof(pen_info));

  if(pen_info.x != -1) {

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

#ifdef DEBUG_EVENT
    printf("%s: %c(%i,%i)\n", _file_,
	   pen_info.event == EV_PEN_UP ? 'U' :
	   pen_info.event == EV_PEN_DOWN ? 'D' :
	   pen_info.event == EV_PEN_MOVE ? 'M' :
	   '?',
	   pen_info.x, pen_info.y);
#endif
  }
}


g_error tsinput_init(void) {
  struct ts_drv_params  ts_params;
  int                   ret_val;

#ifdef DEBUG_INIT
  printf("%s: Opening device %s\n", _file_, DEVICE_FILE_NAME);
#endif
  fd = open(DEVICE_FILE_NAME,O_RDWR | O_NONBLOCK);
  if(fd < 0) {
    printf("%s: Can't open device file: %s\n", _file_, DEVICE_FILE_NAME);
    printf("Error: %s\n",strerror(errno));
    goto error;
  }
  else {
    int mx1, mx2, my1, my2;
    int ux1, ux2, uy1, uy2;

    ret_val=ioctl(fd,TS_PARAMS_GET,&ts_params);
    if(ret_val < 0) {
      printf("ioctl get error: %s\n",strerror(errno));
      goto error_close;
    }

    ts_params.version_req    = MC68328DIGI_VERSION;
    ts_params.event_queue_on = 0;
    ts_params.deglitch_on    = 0;
    ts_params.sample_ms      = 10;
    ts_params.follow_thrs    = 0;
    ts_params.mv_thrs        = 2;
    ts_params.xy_swap        = 0;

#ifdef CONFIG_XCOPILOT
    ts_params.y_max          = 159 + 66;  /* to allow scribble area */
    ts_params.y_min          = 0;
    ts_params.x_max          = 159;
    ts_params.x_min          = 0;

    /* according to mc68328digi.h 'How to calculate the parameters', we have
     * measured:
     */
    mx1 = 508; ux1 =   0;
    my1 = 508; uy1 =   0;
    mx2 = 188; ux2 = 159;
    my2 = 188; uy2 = 159;
#elif defined(CONFIG_CHIPSLICE)
# error TODO: set _min, _max, and m1, m2, u1,u2 for CHIPSLICE
#endif
    ts_params.x_ratio_num    = ux1 - ux2;
    ts_params.x_ratio_den    = mx1 - mx2;
    ts_params.x_offset       =
      ux1 - mx1 * ts_params.x_ratio_num / ts_params.x_ratio_den;
    
    ts_params.y_ratio_num    = uy1 - uy2;
    ts_params.y_ratio_den    = my1 - my2;
    ts_params.y_offset       =
      uy1 - my1 * ts_params.y_ratio_num / ts_params.y_ratio_den;


    ret_val=ioctl(fd,TS_PARAMS_SET,&ts_params);
    if(ret_val < 0) {
      printf("ioctl set error: %s\n",strerror(errno));
      goto error_close;
    }

    return sucess;

  error_close:
    close(fd);
    fd = 0;
    goto error;
  }

  error:
    return mkerror(PG_ERRT_IO, 74);
}


void tsinput_close(void) {
#ifdef DEBUG_EVENT
  printf("%s: Closing device %s\n",_file_, DEVICE_FILE_NAME);
#endif

  if(fd)
    close(fd);
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
