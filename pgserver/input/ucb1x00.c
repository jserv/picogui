/*
 * PicoGUI driver for Philips IS2630 AKA Shannon UCB1200 touchscreen
 * Written by: John Laur
 *
 * This supports "new style" ucb1x00 device driver that does not include
 * jitter correction or coordinate calibration. This driver now uses
 * pgserver's standard mechanisms for calibration and filtering.
 *
 * Requires ucb1x00-ts kernel driver
 */

#include <pgserver/common.h>
 
#include <unistd.h>
 
#include <pgserver/input.h>
#include <pgserver/widget.h>    /* for dispatch_pointing */
#include <pgserver/pgnet.h>
#include <pgserver/configfile.h>

#ifdef DEBUG_EVENT
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#define SAMPLE_SET_SZ		3	/* how many samples to average */
#define JITTER_TOLERANCE	3	/* Pixel radius to ignore jitter */
 
static const char *DEVICE_FILE_NAME = "/dev/ucb1x00-ts";

/* file descriptor for touch panel */
static int fd = -1;
static int pen_down=0;
static int polls_since_data=0;

/* show the cursor on any touchscreen activity */
static int showcursor, pressure_tolerance;

/* last place the cursor was and lat time it was there */
static int last_x;
static int last_y;

g_error ucb1x00_init(void)
{
  /*
   * open up the touch-panel device.
   * Return the fd if successful, or negative if unsuccessful.
   */

  fd = open(DEVICE_FILE_NAME, O_NONBLOCK);
  if (fd < 0) {
    DBG("Error %d opening touch panel\n", errno);
    return mkerror(PG_ERRT_IO, 74);
  }
	
  /* Store config for later */
  showcursor = get_param_int("input-ucb1x00","showcursor",0);
  pressure_tolerance = get_param_int("input-ucb1x00","pressure",45);
  
  return success;
}

void ucb1x00_close(void)
{
  /* Close the touch panel device. */
  if (fd >= 0)
    close(fd);
  fd = -1;
}

/* Internal function to handle filtering and dispatching a packet */
int ucb1x00_packet(int x, int y, int pressure) {
  int old_pendown;

  last_x = x;
  last_y = y;

  DBG("x=%d, y=%d, pressure=%d\n",x,y,pressure);

  /* Transform and filter */
  old_pendown = pen_down;
  pen_down = pressure > pressure_tolerance;
  if (touchscreen_filter(&x, &y, pen_down))
    return 1;
  touchscreen_pentoscreen(&x, &y);
  
  /* Show the cursor if 'showcursor' is on */
  if (showcursor)
    drivermessage(PGDM_CURSORVISIBLE,1,NULL);
  
  if(pen_down && old_pendown) {
    dispatch_pointing(TRIGGER_MOVE,x,y,1);
  }
  else if (pen_down && !old_pendown) {
    dispatch_pointing(TRIGGER_DOWN,x,y,1);
  }
  else if (!pen_down && old_pendown) {
    dispatch_pointing(TRIGGER_UP,x,y,1);
  }
}


int ucb1x00_fd_activate(int active_fd) {
  struct {
    u16 pressure;
    u16 x;
    u16 y;
    u16 pad;
    struct timeval time;
  } ts_data;
  int bytes_read;
   
  if (active_fd != fd) return 0;

  /* read a data point */
  bytes_read = read(fd, (char *)&ts_data, sizeof(ts_data));

  /* No data yet - (shouldnt ever actaully call this)*/
  if (bytes_read == 0)
    return 0;
 
  /* reset the fd_init call counter so we dont issue pen up */
  polls_since_data=0;

  ucb1x00_packet(ts_data.x, ts_data.y, ts_data.pressure);

  return 1;
}

void ucb1x00_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if(fd<0) return;
  
  if ((*n)<(fd+1))
    *n = fd+1;
 
  if (fd>0)
    FD_SET(fd, readfds);

  /* Set us up a polling time for detecting penups */
  if (pen_down) {
    timeout->tv_sec = 0;
    if (timeout->tv_usec > 300)
      timeout->tv_usec = 300;
  }
}

void ucb1x00_poll(void) {
  DBG("polls_since_data=%d\n",polls_since_data);

  /* If it's been a while since we've gotten a sample, release the pen */
  if (polls_since_data++ >= 3 && pen_down) {
    ucb1x00_packet(last_x,last_y,0);
    pen_down = 0;
  }
}

void ucb1x00_message(u32 message, u32 param, u32 *ret) {
  switch (message) {
    
    /* Tuxscreen ioctls:
     *
     * ioctl 64 -> backlight power
     * ioctl 65 -> 0x00-0xFF set brightness
     * ioctl 66 -> get brightness
     * ioctl 67 -> 0x00-0xFF set contrast
     * ioctl 68 -> get contrast
     */
    
    /* Control backlight through the power control driver message */
  case PGDM_POWER:
    ioctl(fd,64,param > PG_POWER_VIDBLANK);
    break;
  case PGDM_BRIGHTNESS:
    ioctl(fd,65,param);
    break;
  case PGDM_CONTRAST:
    ioctl(fd,67,param);
    break;
    
  default:
    touchscreen_message(message,param,ret);
  }
}

/******************************************** Driver registration */
  
g_error ucb1x00_regfunc(struct inlib *i) {
  i->init = &ucb1x00_init;
  i->close = &ucb1x00_close;
  i->fd_activate = &ucb1x00_fd_activate;
  i->fd_init = &ucb1x00_fd_init;
  i->poll = &ucb1x00_poll;
  i->message = &ucb1x00_message;

  return success;
}

/* The End */
