/*
 * PicoGUI driver for Philips IS2630 AKA Shannon UCB1200 touchscreen
 * Written by: John Laur
 * (And then really hacked up by Micah...)
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
#include <pgserver/pgnet.h>
#include <pgserver/configfile.h>

#ifdef DEBUG_EVENT
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

static const char *DEVICE_FILE_NAME = "/dev/ucb1x00-ts";

/* file descriptor for touch panel */
static int fd = -1;

static int pressure_tolerance;

g_error ucb1x00_init(void)
{
  g_error e;
  
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
  pressure_tolerance = get_param_int("input-ucb1x00","pressure",100);
  
  return success;
}

void ucb1x00_close(void)
{
  /* Close the touch panel device. */
  if (fd >= 0)
    close(fd);
  fd = -1;
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
  
  infilter_send_touchscreen(ts_data.x, ts_data.y, ts_data.pressure, 
			    ts_data.pressure > pressure_tolerance);

  return 1;
}

void ucb1x00_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if(fd<0) return;
  if ((*n)<(fd+1))
    *n = fd+1;
  if (fd>0)
    FD_SET(fd, readfds);
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
  }
}

/******************************************** Driver registration */
  
g_error ucb1x00_regfunc(struct inlib *i) {
  i->init = &ucb1x00_init;
  i->close = &ucb1x00_close;
  i->fd_activate = &ucb1x00_fd_activate;
  i->fd_init = &ucb1x00_fd_init;
  i->message = &ucb1x00_message;

  return success;
}

/* The End */
