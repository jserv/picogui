/* $Id: chipslicets.c,v 1.3 2001/11/06 09:10:17 bauermeister Exp $
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

#include <unistd.h>

#include <fcntl.h>     /* File control definitions (provide O_RDONLY) */
#include <linux/kd.h>  /* for KIOCSOUND and KDMKTONE */

#include <pgserver/input.h>
#include <pgserver/widget.h>    /* for dispatch_pointing */
#include <pgserver/pgnet.h>
#include <pgserver/configfile.h>

#include <linux/mc68328digi.h>
#include <rm_client.h>


#define IDLE                      0
#define RUN                       1

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

static int fd=0;
static int bytes_transfered=0;
static int iIsPenUp = 1;
static int iIsPointingDisplayed = 1;
static int chipslicetsSTATE = RUN;

static struct timeval lastEvent;

void chipslicets_message(u32 message, u32 param, u32 *ret);

/*
#define DEBUG_EVENT
*/

/******************************************** Implementations */

int chipslicets_sleep(void) {
#ifdef DEBUG_EVENT
  printf("-- send RM_EV_IDLE to RM\n");
#endif

  /* set state to IDLE */
  chipslicetsSTATE = IDLE;

  /* call bios sleep function throught Ressources Manager */
  rm_emit(RM_EV_IDLE);

  /*
   * the hit to wake up the ChipSlice isn't catch by the chipslicets driver.
   * It's then necessary to re-initiate the time of the last event.
   */
  gettimeofday(&lastEvent,NULL);
}

void chipslicets_poll(void) {
  struct ts_pen_info pen_info;
  
  pen_info.x = -1; pen_info.y = -1;

  bytes_transfered=read(fd,(char *)&pen_info,sizeof(pen_info));

  if(pen_info.x != -1) {

    if(!chipslicetsSTATE) chipslicetsSTATE = RUN;

    switch(pen_info.event) {
    case EV_PEN_UP:
      if(pen_info.x > 350) {
	chipslicets_sleep();
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
    if((delay_sec > SLEEP_IDLE_MAX_SEC) && (chipslicetsSTATE != IDLE))
      chipslicets_sleep();
  }
}


g_error chipslicets_init(void) {
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
    const char* calibration_string;

    ret_val=ioctl(fd,TS_PARAMS_GET,&ts_params);
    if(ret_val < 0) {
      printf("ioctl get error: %s\n",strerror(errno));
      goto error_close;
    }

    ts_params.version_req    = MC68328DIGI_VERSION;
    ts_params.event_queue_on = 1;
    ts_params.deglitch_ms    = 20;
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
    mx1 = 3680; ux1 =   0;
    my1 = 3350; uy1 =   0;
    mx2 =  440; ux2 = 240;
    my2 =  710; uy2 = 320;
#endif

    /* param may override default values */
  if(calibration_string = get_param_str("chipslicets", "calibration", 0)) {
      sscanf(calibration_string, "%d %d %d %d %d %d %d %d %d %d",
	     &mx1, &my1, &mx2, &my2, &offx, &offy, &ux1, &uy1, &ux2, &uy2);
#ifdef DEBUG_INIT
      printf("%s: taking m1 and m2 points from param: '%s'\n",
	     _file_, calibration_string);
      printf("  mx1=%d my1=%d mx2=%d my2=%d offx=%d offy=%d "
	     "ux1=%d uy1=%d ux2=%d uy2=%d\n",
	     mx1, my1, mx2, my2, offx, offy, ux1, uy1, ux2, uy2);
#endif
    }

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


void chipslicets_close(void) {
#ifdef DEBUG_INIT
  printf("%s: Closing device %s\n",_file_, DEVICE_FILE_NAME);
#endif

  if(fd)
    close(fd);

/* Disconnects the client from the Resource Manager */
  rm_exit ();
}


/* Polling time for the input driver */ 
void chipslicets_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  
  /* Don't increase the poll time, but try to decrease it to POLL_USEC */

  if (timeout->tv_sec) {
    timeout->tv_sec = 0;
    timeout->tv_usec = POLL_USEC;
  }
  else if (timeout->tv_usec > POLL_USEC)
    timeout->tv_usec = POLL_USEC;
}

/* message between driver to provide sound (for exemple) */
void chipslicets_message(u32 message, u32 param, u32 *ret) {

  int snd_type;
  *ret = 0;

  switch (message) {

#ifdef DRIVER_CHIPSLICETS_SND
  /* sound support through /dev/tty2 implemented in drivers/char/vt.c */
  case PGDM_SOUNDFX:

    switch(param) {

    case PG_SND_SHORTBEEP:
      snd_type = KDMKTONE;
      break;

    case PG_SND_KEYCLICK:
      snd_type = KDMKTONE;
      break;

    default:
      break;
    }

# if defined(CONFIG_XCOPILOT) || defined(CONFIG_SOFT_CHIPSLICE)
    printf("beep\n");

# elif defined(CONFIG_CHIPSLICE)
    {
      int snd_freq = get_param_int("input-chipslicets","snd_frequency",8000);
      int snd_leng = get_param_int("input-chipslicets","snd_length",50);
      int fd = 0;

      /* if no frequency defined, get_param_int return 5000.
       * if no length defined, get_param_int return 300.
       */

      /* open virtual terminal read only */
      fd = open("/dev/tty2",O_RDONLY);

      if(fd < 1) {
	printf("/dev/tty2: open error\n");
	return;
      }
      ioctl(fd,snd_type,(snd_freq + (snd_leng << 16)));
      close(fd);
    }
# endif /* defined(CONFIG_XCOPILOT) || defined(CONFIG_SOFT_CHIPSLICE) */
#endif /* DRIVER_CHIPSLICETS_SND */

  default:
    break;
  }
  return;
}



/******************************************** Driver registration */

g_error chipslicets_regfunc(struct inlib *i) {
  i->init = &chipslicets_init;
  i->close = &chipslicets_close;
  i->poll = &chipslicets_poll;
  i->fd_init = &chipslicets_fd_init;
  i->message = &chipslicets_message;
  return sucess;
}

#endif /* DRIVER_CHIPSLICETS */
/* The End */
