/*
 * PicoGUI driver for Philips IS2630 AKA Shannon UCB1200 touchscreen
 * Written by: John Laur
 *
 * This supports "new style" ucb1x00 device driver that does not include
 * jitter correction or coordinate calibration. You have to set these
 * values up in pgserver.conf before pgserver is started!
 *
 * Requires ucb1x00-ts kernel driver
 */

#include <pgserver/common.h>
 
#include <unistd.h>
 
#include <pgserver/input.h>
#include <pgserver/widget.h>    /* for dispatch_pointing */
#include <pgserver/pgnet.h>

#define SAMPLE_SET_SZ	3	/* how many samples to average */
#define POLL_USEC	200	/* how long after input is recieved to issue pen up */
 
static const char *DEVICE_FILE_NAME = "/dev/ucb1x00-ts";
static const char *_file_ = __FILE__;
static const char *PG_TS_ENV_NAME = "PG_TS_CALIBRATION";

/* file descriptor for touch panel */
static int fd = -1;
static int pen_down=0;
static int inits_since_data=0;

/* show the cursor on any touchscreen activity */
static int showcursor;
/* Calibration variables read from config file */
static int res_x, res_y, max_x, max_y, min_x, min_y, flip_x, flip_y;

/* last place the cursor was and lat time it was there */
static int last_x;
static int last_y;
static struct timeval last_time;
/* counts the number of samples taken (for averaging) */
static int sample_counter = 0;

g_error ucb1x00_init(void)
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

  /* Store config for later */
  showcursor = get_param_int("input-ucb1x00","showcursor",0);
  res_x = get_param_int("input-ucb1x00","res_x",640);
  res_y = get_param_int("input-ucb1x00","res_y",480);
  max_x = get_param_int("input-ucb1x00","max_x",950);
  max_y = get_param_int("input-ucb1x00","max_y",950);
  min_x = get_param_int("input-ucb1x00","min_x",50);
  min_y = get_param_int("input-ucb1x00","min_y",50);
  flip_x = get_param_int("input-ucb1x00","flip_x",1);
  flip_y = get_param_int("input-ucb1x00","flip_y",1);
  
  return sucess;
}

void ucb1x00_close(void)
{
  /* Close the touch panel device. */
#ifdef DEBUG_EVENT
  printf("ucb1x00_close called\n");
#endif
  if (fd >= 0)
    close(fd);
  fd = -1;
}

int ucb1x00_fd_activate(int active_fd) {
  int bytes_read;
  struct {
	  u16 p;
	  u16 x;
	  u16 y;
	  u16 pad;
	  struct timeval time;
  } ts_data;
  int cooked_x, cooked_y;
  
  /* these hold the sums of x,y samples to average */
  static int sum_x;
  static int sum_y;
       
  if (active_fd != fd) return 0;

  /* read a data point */
  bytes_read = read(fd, (char *)&ts_data, sizeof(ts_data));

  /* No data yet */
  if (bytes_read = 0) {
	  printf("NO BYTES READ!!!!!!!!!!!!\n");
	  return 0;
  }
 
  inits_since_data=0;
  
  /* cook the coordinates (perform calibration) */
  cooked_x = (flip_x) ? ((max_x - ts_data.x) * res_x) / (max_x - min_x) :
                ((ts_data.x - min_x) * res_x) / (max_x - min_x);
  cooked_y = (flip_y) ? ((max_y - ts_data.y) * res_y) / (max_y - min_y) :
                ((ts_data.y - min_y) * res_y) / (max_y - min_y);
  
  if(ts_data.p) {
	 
    /* Show the cursor if 'showcursor' is on */
    if (showcursor)
      drivermessage(PGDM_CURSORVISIBLE,1,NULL);

    if(pen_down && sample_counter == SAMPLE_SET_SZ) {
      last_x=sum_x/SAMPLE_SET_SZ;
      last_y=sum_y/SAMPLE_SET_SZ;
      last_time=ts_data.time;
      dispatch_pointing(TRIGGER_MOVE,last_x,last_y,1);
      sample_counter = 0;
      sum_x=0;
      sum_y=0;
#ifdef DEBUG_EVENT
      printf("Pen Move\n");
#endif
    } else if(sample_counter == SAMPLE_SET_SZ) {
      last_x=sum_x/SAMPLE_SET_SZ;
      last_y=sum_y/SAMPLE_SET_SZ;
      last_time=ts_data.time;
      dispatch_pointing(TRIGGER_DOWN,last_x,last_y,1);
      pen_down = 1;
      sample_counter = 0;
      sum_x=0;
      sum_y=0;
#ifdef DEBUG_EVENT
      printf("Pen Down\n");
#endif
    } else {
      sum_x += cooked_x;
      sum_y += cooked_y;
      sample_counter++;
    }
  
  }

#ifdef DEBUG_EVENT
  printf("(%d,%d:%d:%d) (%d, %d)\n", ts_data.x, ts_data.y, ts_data.p, cooked_x, cooked_y);
#endif	

  return 1;

}

/* Polling time for the input driver */
void ucb1x00_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if(fd<0) return;

  if ((*n)<(fd+1))
    *n = fd+1;

  if (fd>0)
    FD_SET(fd, readfds);

  /* a little unorthodox to do this here, but... simpler? */
  if(inits_since_data++ > 2 && pen_down) {
#ifdef DEBUG_EVENT
	  printf("Pen Up\n");
#endif
	  dispatch_pointing(TRIGGER_UP,last_x,last_y,0);
	  pen_down = 0;
  }
  
}

/******************************************** Driver registration */
 
g_error ucb1x00_regfunc(struct inlib *i) {
  i->init = &ucb1x00_init;
  i->close = &ucb1x00_close;
  i->fd_activate = &ucb1x00_fd_activate;
  i->fd_init = &ucb1x00_fd_init;
  return sucess;
}

/* The End */
