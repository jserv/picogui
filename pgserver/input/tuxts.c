/*
 * PicoGUI driver for Philips IS2630 AKA Shannon UCB1200 touchscreen
 * Written by: John Laur
 *
 * based on:
 * Microwindows touch screen driver for ADS Graphics Client (www.flatpanels.com)
 * Copyright (c) 2000 Century Software Embedded Technologies
 *
 * Requires ucb1200-ts kernel driver
 */

#include <pgserver/common.h>
 
#include <unistd.h>
 
#include <pgserver/input.h>
#include <pgserver/widget.h>    /* for dispatch_pointing */
#include <pgserver/pgnet.h>

#define POLL_USEC                500
 
static const char *DEVICE_FILE_NAME = "/dev/ucb1200-ts";
static const char *_file_ = __FILE__;
static const char *PG_TS_ENV_NAME = "PG_TS_CALIBRATION";

/* file descriptor for touch panel */
static int fd = -1;
static int PEN_DOWN;
static int backlight_on;

g_error tuxts_init(void)
{
  /*
   * open up the touch-panel device.
   * Return the fd if successful, or negative if unsuccessful.
   */

  fd = open(DEVICE_FILE_NAME, O_NONBLOCK);
  if (fd < 0) {
    printf("Error %d opening touch panel\n", errno);
    return mkerror(PG_ERRT_IO, 74);
  }
	
#ifdef DEBUG_EVENT
  printf("Opened the touchscreen on %d\n", fd);
#endif

  /* Set up us fixed defaults!!! */
  //	ioctl(fd,1,10);		/* pressure - unsupported */
  //	ioctl(fd,2,100);	/* updelay - unsupported */
  //	ioctl(fd,7,0);		/* fudge x - unsupported */
  //	ioctl(fd,8,0);		/* fudge y - unsupported */
  //	ioctl(fd,9,3);		/* average samples - unsupported */
  //	ioctl(fd,12,0);		/* xy swap (portrait) - unsupported */
  ioctl(fd,5,640);	/* res x */
  ioctl(fd,6,480);	/* res y */
	
/* Set up us the calibration datum!!! */
  ioctl(fd,3,964);	/* top left corner raw max x */
  ioctl(fd,4,964);	/*                 raw max y */
  ioctl(fd,10,46);	/* bottom  right corner raw min x */
  ioctl(fd,11,52);	/*                      raw min y */
  ioctl(fd,15,1);		/* reverse x axis */
  ioctl(fd,16,1);		/* reverse y axis */
  ioctl(fd,13,1);		/* enable calibration */

  /* Do other ioctls we gotta do!!! */
#ifdef DEBUG_EVENT
  ioctl(fd,17);		/* print driver paramaters */
#endif
  ioctl(fd,64,1);		/* turn on the backlight */

  return sucess;
}

void tuxts_close(void)
{
  /* turn on the backlight on exit */
  drivermessage(PGDM_BACKLIGHT,1);

 	/* Close the touch panel device. */
#ifdef DEBUG_EVENT
  printf("tuxts_close called\n");
#endif
  if (fd >= 0)
    close(fd);
  fd = -1;
}

void tuxts_poll(void) {
  /* read a data point */
  short data[4];
  int bytes_read;
  int x, y, z, b;
  int mouse = -1;
       
  bytes_read = read(fd, data, 4 * sizeof(short));
	
	/* No data yet */
  if (bytes_read != (4 * sizeof(short)))
    return;
	
  x = data[1];
  y = data[2];
  b = data[0];

  if(b>0) {
    if(PEN_DOWN) {
      if (backlight_on)
	dispatch_pointing(TRIGGER_MOVE,x,y,1);
#ifdef DEBUG_EVENT
      printf("Pen Move\n");
#endif
    } else {
      if (backlight_on) {
	dispatch_pointing(TRIGGER_DOWN,x,y,1);
	PEN_DOWN = 1;
      }
      else
	drivermessage(PGDM_BACKLIGHT,1);
#ifdef DEBUG_EVENT
      printf("Pen Down\n");
#endif
    }
  } else {
    if (backlight_on)
      dispatch_pointing(TRIGGER_UP,x,y,0);
#ifdef DEBUG_EVENT
    printf("Pen Up\n");
#endif
    PEN_DOWN = 0;
  }

#ifdef DEBUG_EVENT
  printf("%d,%d,%d\n", x, y, b);
#endif	
}

/* Polling time for the input driver */
void tuxts_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  timeout->tv_sec = 0;
  timeout->tv_usec = POLL_USEC;
}

void tuxts_message(u32 message, u32 param) {
  switch (message) {

    /* Allow control of the backlight from anywhere */
  case PGDM_BACKLIGHT:
    ioctl(fd,64,backlight_on = (param!=0));
    break;
  }
}
 
/******************************************** Driver registration */
 
g_error tuxts_regfunc(struct inlib *i) {
  i->init = &tuxts_init;
  i->close = &tuxts_close;
  i->poll = &tuxts_poll;
  i->fd_init = &tuxts_fd_init;
  i->message = &tuxts_message;
  return sucess;
}

/* The End */
