/* $Id: tsinput.c,v 1.16 2001/06/04 16:52:23 bauermeister Exp $
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

#include <unistd.h>

#include <pgserver/input.h>
#include <pgserver/widget.h>    /* for dispatch_pointing */
#include <pgserver/pgnet.h>

#include <linux/mc68328digi.h>
#include <rm/rm_client.h>


#define POLL_USEC                100

/*
 * timeout for pointing display, backlight (not yet implemented)
 * and sleep mode
 */
#define POINTING_IDLE_MAX_SEC      1
#define SLEEP_IDLE_MAX_SEC       100
//#define BACKLIGHT_IDLE_MAX_SEC    50

static const char *DEVICE_FILE_NAME = "/dev/ts";
static const char *_file_ = __FILE__; 
static const char *PG_TS_ENV_NAME = "PG_TS_CALIBRATION";

static int fd=0;
static int bytes_transfered=0;
static int iIsPenUp = 1;
static int iIsPointingDisplayed = 1;

static struct timeval lastEvent;


/******************************************** Implementations */

int tsinput_sleep(void) {
#ifdef DEBUG_EVENT
  printf("-- going to sleep mode\n");
#endif

  /* call bios sleep function throught Ressources Manager */
  rm_sleep(RM_WAKE_ON_BUTTON);

  /*
   * the hit to wake up the ChipSlice isn't catch by the tsinput driver.
   * It's then necessary to re-initiate the time of the last event.
   */
  gettimeofday(&lastEvent,NULL);
}

void tsinput_poll(void) {
  struct ts_pen_info pen_info;
  
  pen_info.x = -1; pen_info.y = -1;
  
  bytes_transfered=read(fd,(char *)&pen_info,sizeof(pen_info));

  if(pen_info.x != -1) {

    switch(pen_info.event) {
    case EV_PEN_UP:
      if(pen_info.x > 350) {
	tsinput_sleep();
	break;
      }
      dispatch_pointing(TRIGGER_UP,pen_info.x,pen_info.y,0);
      gettimeofday(&lastEvent,NULL);
      iIsPenUp = 1;
      iIsPointingDisplayed = 1;
      break;
      
    case EV_PEN_DOWN:
      dispatch_pointing(TRIGGER_DOWN,pen_info.x,pen_info.y,1);
      gettimeofday(&lastEvent,NULL);
      iIsPenUp = 0;
      iIsPointingDisplayed = 1;
      break;
      
    case EV_PEN_MOVE:
      /*
       * don't display pointing device when move for speed reason 
       * this may certainly change in the future
       */
      dispatch_pointing(TRIGGER_MOVE,pen_info.x,pen_info.y,1);
      //      if(iIsPointingDisplayed) {
      //	VID(sprite_hide) (cursor);
      //	iIsPointingDisplayed = 0;
      //      }
      gettimeofday(&lastEvent,NULL);
      iIsPenUp = 0;
      break;

    default:
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

  /* If pen is up, test if there is some activity or not */
  if(iIsPenUp) {
    struct timeval lastIdle;
    int delay_sec;

    gettimeofday(&lastIdle,NULL);
    delay_sec = lastIdle.tv_sec - lastEvent.tv_sec;

    /* Management for pointing display, sleep mode and backlight */
    if((delay_sec > POINTING_IDLE_MAX_SEC) && iIsPointingDisplayed) {
      VID(sprite_hide) (cursor);
      iIsPointingDisplayed = 0;
#ifdef DEBUG_EVENT
      printf("-- hide pointing\n");
#endif
    }
    if(delay_sec > SLEEP_IDLE_MAX_SEC)
      tsinput_sleep();
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
    int offx = 0, offy = 0;
    char* pg_ts_env;

    ret_val=ioctl(fd,TS_PARAMS_GET,&ts_params);
    if(ret_val < 0) {
      printf("ioctl get error: %s\n",strerror(errno));
      goto error_close;
    }

    ts_params.version_req    = MC68328DIGI_VERSION;
    ts_params.event_queue_on = 1;
    ts_params.deglitch_ms    = 0;
    ts_params.sample_ms      = 10;
    ts_params.follow_thrs    = 2;
    ts_params.mv_thrs        = 5;
    ts_params.xy_swap        = 1;

#ifdef CONFIG_XCOPILOT
    ts_params.y_max          = 159 + 66;  /* to allow scribble area */
    ts_params.y_min          = 0;
    ts_params.x_max          = 159;
    ts_params.x_min          = 0;
    /* overwrite xy_swap for XCopilot environment */
    ts_params.xy_swap        = 0;

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
/*
    mx1 =  440; ux1 =   0;
    my1 = 3350; uy1 =   0;
    mx2 = 3680; ux2 = 320;
    my2 =  710; uy2 = 240;
*/
    mx1 = 3680; ux1 =   0;
    my1 = 3350; uy1 =   0;
    mx2 =  440; ux2 = 240;
    my2 =  710; uy2 = 320;

    /* env var will override default values (but only in ChipSlice!!) */
    if( pg_ts_env = (char*)getenv(PG_TS_ENV_NAME) ) {
      sscanf(pg_ts_env, "%d %d %d %d %d %d",
	     &mx1, &my1, &mx2, &my2, &offx, &offy);
#  ifdef DEBUG_INIT
      printf("%s: taking m1 and m2 points for env var: '%s'\n",
	     _file_, pg_ts_env);
      printf("  mx1=%d my1=%d mx2=%d my2=%d offx=%d offy=%d\n",
	     mx1, my1, mx2, my2, offx, offy);
#  endif
    }
#endif

    ux1 += offx;
    uy1 += offy;
    ux2 += offx;
    uy2 += offy;

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

    /* init the Ressources Manager */
    rm_init();

    gettimeofday(&lastEvent,NULL);
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
#ifdef DEBUG_INIT
  printf("%s: Closing device %s\n",_file_, DEVICE_FILE_NAME);
#endif

  if(fd)
    close(fd);

/* Disconnects the client from the Resource Manager */
  rm_exit ();
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
