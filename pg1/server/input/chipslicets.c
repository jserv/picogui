/* $Id$
 *
 * chipslicets.c - input driver for touch screen
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

#ifdef DRIVER_CHIPSLICETS

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/configfile.h>

#include <linux/mc68328digi.h>

#define LOCAL_DEBUG 0
#define LOCAL_TRACE 0

/* ------------------------------------------------------------------------- */

static const char *DEVICE_FILE_NAME = "/dev/ts";

static int ts_fd = -1;
static int inhibited = 0;

/* ------------------------------------------------------------------------- */

#if LOCAL_DEBUG
# define DPRINTF(x...) printf(__FILE__": " x)
# define WARNF(x...)   printf(__FILE__": " x)
#else
# define DPRINTF(x...)
# define WARNF(x...)   printf(__FILE__": " x)
# undef LOCAL_TRACE
# define LOCAL_TRACE 0
#endif

#if LOCAL_TRACE
# define TRACEF(x...)  printf(__FILE__": " x)
#else
# define TRACEF(x...)
#endif

/* ------------------------------------------------------------------------- */
/*                      PicoGUI input events handling                        */
/* ------------------------------------------------------------------------- */

/* We insert our fd in the set for the server's select */ 
void chipslicets_fd_init(int *n,fd_set *readfds,struct timeval *timeout)
{
  if(ts_fd<0) return;

  if ((*n)<(ts_fd+1))
    *n = ts_fd+1;

  if (ts_fd>0)
    FD_SET(ts_fd, readfds);
}

/* ------------------------------------------------------------------------- */

static int chipslicets_fd_activate(int fd)
{
  struct ts_pen_info pen_info;
  int bytes_transferred;

#if LOCAL_TRACE
  printf(" a(%d:%d)",fd,ts_fd); fflush(stdout);
#endif
  /* is the fd mine ? */
  if(fd!=ts_fd)
    return 0; /* no */

#if LOCAL_TRACE
  printf("A"); fflush(stdout);
#endif

  bytes_transferred=read(ts_fd,(char *)&pen_info,sizeof(pen_info));

  if(bytes_transferred>0) {

    switch(pen_info.event) {
    case EV_PEN_UP:
      infilter_send_pointing(PG_TRIGGER_UP,pen_info.x,pen_info.y,0,NULL);
      break;
      
    case EV_PEN_DOWN:
      infilter_send_pointing(PG_TRIGGER_DOWN,pen_info.x,pen_info.y,1,NULL);
      break;
      
    case EV_PEN_MOVE:
      /*
       * don't display pointing device when move for speed reason 
       * this may certainly change in the future
       */
      infilter_send_pointing(PG_TRIGGER_MOVE,pen_info.x,pen_info.y,1,NULL);
      break;

    default:
    }

    DPRINTF("%c(%i,%i)\n",
	    pen_info.event == EV_PEN_UP ? 'U' :
	    pen_info.event == EV_PEN_DOWN ? 'D' :
	    pen_info.event == EV_PEN_MOVE ? 'M' :
	    '?',
	    pen_info.x, pen_info.y);
  }
  
  return 1;
}

/* ------------------------------------------------------------------------- */
/*                             Un/initializations                            */
/* ------------------------------------------------------------------------- */

int init_device()
{
  TRACEF(">>> init_device\n");

  if(ts_fd<0) {
    DPRINTF("Opening device %s\n", DEVICE_FILE_NAME);
    ts_fd = open(DEVICE_FILE_NAME,O_RDWR | O_NONBLOCK);
    if(ts_fd < 0) {
      fprintf(stderr, "%s: Can't open device: %s (%s)\n", __FILE__,
	      DEVICE_FILE_NAME, strerror(errno)
	      );
    }
  }
  return ts_fd;
}


void uninit_device()
{
  TRACEF(">>> uninit_device\n");
  DPRINTF("Closing device %s (fd=%d)\n", DEVICE_FILE_NAME, ts_fd);

  if(ts_fd>0)
    close(ts_fd);
  ts_fd = -1;
}


g_error chipslicets_init(void)
{
  struct ts_drv_params  ts_params;
  int                   ret_val;

  TRACEF(">>> chipslicets_init\n");

  ts_fd = -1;
  inhibited = 0;

  init_device();
  if(ts_fd<0) {
    fprintf(stderr, "%s: Can't open device: %s (%s)\n", __FILE__,
	    DEVICE_FILE_NAME, strerror(errno)
	    );
    goto error;
  }
  else {
    int mx1, mx2, my1, my2;
    int ux1, ux2, uy1, uy2;
    int offx = 0, offy = 0;
    const char* cal_string;

    ret_val=ioctl(ts_fd,TS_PARAMS_GET,&ts_params);
    if(ret_val < 0) {
      fprintf(stderr, "%s: ioctl get error: %s\n", __FILE__, strerror(errno));
      goto error_close;
    }

    ts_params.version_req    = MC68328DIGI_VERSION;
    ts_params.event_queue_on = 1;
    ts_params.deglitch_ms    = 20;
    ts_params.sample_ms      = 10;
    ts_params.follow_thrs    = 2;
    ts_params.mv_thrs        = 3;
    ts_params.xy_swap        = 1;

#ifdef CONFIG_XCOPILOT
    ts_params.y_max          = 159 + 66;  /* to allow scribble area */
    ts_params.y_min          = 0;
    ts_params.x_max          = 159;
    ts_params.x_min          = 0;

    /* overwrite xy_swap for XCopilot environment */
    ts_params.xy_swap        = 0;

    /* want no deglitch timeout on emulator */
    ts_params.deglitch_ms    = 0;

    /* according to mc68328digi.h 'How to calculate the parameters', we have
     * measured:
     */
    mx1 = 508; ux1 =   0;
    my1 = 508; uy1 =   0;
    mx2 = 188; ux2 = 159;
    my2 = 188; uy2 = 159;

#elif defined(CONFIG_SOFT_CHIPSLICE)
    /* limits are like for ChipSlice */
    ts_params.y_max          = 320-1;
    ts_params.y_min          = 0;
    ts_params.x_max          = 240-1;
    ts_params.x_min          = 0;

    /* overwrite xy_swap for XCopilot environment */
    ts_params.xy_swap        = 0;

    /* want no deglitch timeout on emulator */
    ts_params.deglitch_ms    = 0;

    /* ratio s are like for xcopilot, except that for big screen
     * height, y is divided by two
     */
    mx1 = 508; ux1 =   0;
    my1 = 508; uy1 =   0;
    mx2 = 188; ux2 = 159;
    my2 = 188; uy2 = 160*2-1;

#elif defined(CONFIG_CHIPSLICE)
    ts_params.y_max          = 320-1;
    ts_params.y_min          = 0;
    ts_params.x_max          = 240-1;
    ts_params.x_min          = 0;
    ts_params.y_max          = 400;
    ts_params.sample_ms      = 40;

    mx1 = 3680; ux1 =   0;
    my1 = 3350; uy1 =   0;
    mx2 =  440; ux2 = 240;
    my2 =  710; uy2 = 320;
#endif

    /*
     * (calibration parameters are no longer determined here; it is a
     *  shell utility that pre-calibrates the device prior to starting
     *  the pgserver)
     */

    /* put new params */
    ret_val=ioctl(ts_fd,TS_PARAMS_SET,&ts_params);
    if(ret_val < 0) {
      fprintf(stderr, "%s: ioctl set error: %s\n", __FILE__, strerror(errno));
      goto error_close;
    }

    return success;

  error_close:
    close(ts_fd);
    ts_fd = -1;
    goto error;
  }

  error:
    return mkerror(PG_ERRT_IO, 74);
}

/* ------------------------------------------------------------------------- */

void chipslicets_close(void)
{
  TRACEF(">>> chipslicets_close\n");

  uninit_device();
}

/* ------------------------------------------------------------------------- */
/*                      PicoGUI driver messages handling                     */
/* ------------------------------------------------------------------------- */

/* Process messages comming from PicoGui */
void chipslicets_message(u32 message, u32 param, u32 *ret)
{
  TRACEF(">>> chipslicets_message\n");

  DPRINTF("message=%d param=%d\n", message, param);

  if(ret) *ret = 0;

  switch (message) {
  case PGDM_INPUT_CALEN:
    DPRINTF("PGDM_INPUT_CALEN:%d\n", param);
    inhibited = param!=0;

    if(inhibited)
      uninit_device();
    else
      init_device();
  }
}

/* ------------------------------------------------------------------------- */
/*                          Driver registration                              */
/* ------------------------------------------------------------------------- */

g_error chipslicets_regfunc(struct inlib *i)
{
  i->init = &chipslicets_init;
  i->close = &chipslicets_close;
  i->fd_activate = &chipslicets_fd_activate;
  i->fd_init = &chipslicets_fd_init;
  i->message = &chipslicets_message;
  return success;
}

/* ------------------------------------------------------------------------- */

#endif /* DRIVER_CHIPSLICETS */
/* The End */
