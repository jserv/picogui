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

#define KDSETMODE       0x4B3A  /* set text/graphics mode */
#define KD_TEXT         0x00
#define KD_GRAPHICS     0x01

#include <pgserver/common.h>
 
#ifdef DRIVER_TUXTS
 
#include <unistd.h>
 
#include <pgserver/input.h>
#include <pgserver/widget.h>    /* for dispatch_pointing */
#include <pgserver/pgnet.h>

#define POLL_USEC                500
 
/*
 *  * timeout for pointing display, backlight
 */
#define POINTING_IDLE_MAX_SEC      1
#define BACKLIGHT_IDLE_MAX_SEC    120

static const char *DEVICE_FILE_NAME = "/dev/ucb1200-ts";
static const char *_file_ = __FILE__;
static const char *PG_TS_ENV_NAME = "PG_TS_CALIBRATION";

/* file descriptor for touch panel */
static int fd = -1;
static int tty = -1;
static int PEN_DOWN = 0;
static int BACKLIGHT_ON = 1;
static int CURSOR_SHOWN = 0;
static struct timeval lastEvent;

g_error tuxts_init(void)
{
 	/*
	 * open up the touch-panel device.
	 * Return the fd if successful, or negative if unsuccessful.
	 */

        /* open tty, enter graphics mode*/
	tty = open ("/dev/tty0", O_RDWR);
        if(tty < 0) {
	        printf("Error can't open /dev/tty0: %m\n");
	        return mkerror(PG_ERRT_IO, 74);
  	}
        if(ioctl (tty, KDSETMODE, KD_GRAPHICS) == -1) {
		printf("Error setting graphics mode: %m\n");
		close(tty);
	        return mkerror(PG_ERRT_IO, 74);
	}
        close(tty);
					
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
	
	gettimeofday(&lastEvent,NULL);

	return sucess;
}

void tuxts_close(void)
{
 	/* Close the touch panel device. */
#ifdef DEBUG_EVENT
	printf("tuxts_close called\n");
#endif
	if (fd >= 0)
		close(fd);
	fd = -1;

        tty = open ("/dev/tty0", O_RDWR);
        if(tty < 0) {
		printf("Error can't open /dev/tty0: %m\n");
		return;
	}
	if(ioctl (tty, KDSETMODE, KD_GRAPHICS) == -1) {
		printf("Error setting text mode: %m\n");
	}
	close(tty);
}

void tuxts_poll(void)
{
	/* read a data point */
	short data[4];
	int bytes_read;
	int x, y, z, b;
	int mouse = -1;
       
	bytes_read = read(fd, data, 4 * sizeof(short));
	
	if (bytes_read != (4 * sizeof(short))) {
	  	struct timeval lastIdle;
		int delay_sec;
		     
		gettimeofday(&lastIdle,NULL);
		delay_sec = lastIdle.tv_sec - lastEvent.tv_sec;
	
/*		if((delay_sec > POINTING_IDLE_MAX_SEC) && CURSOR_SHOWN) {
			VID(sprite_hide) (cursor);
			CURSOR_SHOWN = 0;
		}
*/		
		if((delay_sec > BACKLIGHT_IDLE_MAX_SEC) && BACKLIGHT_ON) {
			ioctl(fd, 64, 0);
			BACKLIGHT_ON = 0;
		}
		
		return;
	}

	if(!BACKLIGHT_ON) { 
		ioctl(fd, 64, 1);
		BACKLIGHT_ON = 1;
	}
	
	x = data[1];
	y = data[2];
	b = data[0];

	if(b>0) {
		if(PEN_DOWN) {
			dispatch_pointing(TRIGGER_MOVE,x,y,1);
			VID(sprite_hide) (cursor);
			CURSOR_SHOWN = 0;
#ifdef DEBUG_EVENT
			printf("Pen Move\n");
#endif
		} else {
			if(!BACKLIGHT_ON) { 
				ioctl(fd, 64, 1);
				BACKLIGHT_ON = 1;
			} else dispatch_pointing(TRIGGER_DOWN,x,y,1);
#ifdef DEBUG_EVENT
			printf("Pen Down\n");
#endif
			PEN_DOWN = 1;
			}
	} else {
		dispatch_pointing(TRIGGER_UP,x,y,0);
#ifdef DEBUG_EVENT
		printf("Pen Up\n");
#endif
		VID(sprite_show) (cursor);
		CURSOR_SHOWN = 1;
		PEN_DOWN = 0;
	}

#ifdef DEBUG_EVENT
	printf("%d,%d,%d\n", x, y, b);
#endif	
	gettimeofday(&lastEvent,NULL);
}

/* Polling time for the input driver */
void tuxts_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
	timeout->tv_sec = 0;
	timeout->tv_usec = POLL_USEC;
}
 
/******************************************** Driver registration */
 
g_error tuxts_regfunc(struct inlib *i) {
	i->init = &tuxts_init;
	i->close = &tuxts_close;
	i->poll = &tuxts_poll;
	i->fd_init = &tuxts_fd_init;
	return sucess;
}

#endif
