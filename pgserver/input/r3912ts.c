/* $Id: r3912ts.c,v 1.1 2001/04/16 04:38:30 micahjd Exp $
 *
 * r3912ts.c - input driver for r3912 touch screen found on the VTech Helio
 *             and others. Other touch screens using the same data format should
 *             work too.
 *
 * Much of this code is copied from Jay Carlson's Helio patches for the W window
 * system, available at vhl-tools.sourceforge.net
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Contributors:
 *
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>    /* For sending events */

#include <stdio.h>              /* For reading the device */

int r3912ts_fd;

struct tpanel_sample {
	unsigned short state;
	unsigned short x;
	unsigned short y;
};

/******************************************** Utilities */
/* This section is pretty much all by Jay Carlson (or whoever he
 * 'borrowed' code from ;-) */

#define TRANSFORMATION_UNITS_PER_PIXEL 4

typedef struct
{
	/*
	 * Coefficients for the transformation formulas:
	 *
	 *     m = (ax + by + c) / s
	 *     n = (dx + ey + f) / s
	 *
	 * These formulas will transform a device point (x, y) to a
	 * screen point (m, n) in fractional pixels.  The fraction
	 * is 1 / TRANSFORMATION_UNITS_PER_PIXEL.
	 */

	int a, b, c, d, e, f, s;
} TRANSFORMATION_COEFFICIENTS;

typedef struct
{
        int x, y;
} XYPOINT;


int enable_pointing_coordinate_transform = 1;

static TRANSFORMATION_COEFFICIENTS tc;

int GetPointerCalibrationData()
{
        extern int errno;
        /*
	 * Read the calibration data from the calibration file.
	 * Calibration file format is seven coefficients separated by spaces.
	 */

	/* Get pointer calibration data from this file */
	const char cal_filename[] = "/etc/pointercal";

	int items;

	FILE* f = fopen(cal_filename, "r");
	if ( f == NULL )
	{
		fprintf(stderr, "Error %d opening pointer calibration file %s.\n",
			errno, cal_filename);
		return -1;
	}

	items = fscanf(f, "%d %d %d %d %d %d %d",
		&tc.a, &tc.b, &tc.c, &tc.d, &tc.e, &tc.f, &tc.s);

        fclose(f);
   
        if ( items != 7 )
	{
		fprintf(stderr, "Improperly formatted pointer calibration file %s.\n",
			cal_filename);
		return -1;
	}

	return 0;
}

inline XYPOINT DeviceToScreen(XYPOINT p)
{
	/*
	 * Transform device coordinates to screen coordinates.
	 * Take a point p in device coordinates and return the corresponding
	 * point in screen coodinates.
	 * This can scale, translate, rotate and/or skew, based on the coefficients
	 * calculated above based on the list of screen vs. device coordinates.
	 */

	static XYPOINT prev;
	/* set slop at 3/4 pixel */
	const short slop = TRANSFORMATION_UNITS_PER_PIXEL * 3 / 4;
	XYPOINT new, out;

	/* transform */
	new.x = (tc.a * p.x + tc.b * p.y + tc.c) / tc.s;
	new.y = (tc.d * p.x + tc.e * p.y + tc.f) / tc.s;

	/* hysteresis (thanks to John Siau) */
	if ( abs(new.x - prev.x) >= slop )
		out.x = (new.x | 0x3) ^ 0x3;
	else
		out.x = prev.x;

	if ( abs(new.y - prev.y) >= slop )
		out.y = (new.y | 0x3) ^ 0x3;
	else
		out.y = prev.y;

	prev = out;

	return out;
}

/* Currently this just maps the silkscreen buttons to keyboard keys.
 * Input in the handwriting area is currently ignored, I'll find the best way
 * to pass that on to a handwriting recognizer later. 
 */

void handle_silkscreen_buttons(int x, int y, int state) {
   int column = -1, row;
   short key;
   /* Mapping of silkscreen buttons to keys. */
   static short keymap[] = {
      PGKEY_MENU, PGKEY_RETURN, PGKEY_LSUPER,
      PGKEY_CAPSLOCK, PGKEY_ESCAPE, PGKEY_NUMLOCK
   };

   /* Ignore stylus dragging in buttons */
   if (state==2)
     return;
   
   if (y < 169)
     return;
   
   if (y < 187)
     row = 0; /* Edit/kbd */
   else if (y < 203)
     row = 1; /* OK/exit */
   else
     row = 2; /* main/123 */
   
   if (x < 20)
     column = 0;
   else if (x > 141)
     column = 1;
   
   if  (column == -1) {
      /* FIXME: handwriting input goes here */
      
      return;
   }
   
   /* Map silkscreen buttons to keyboard */
   key = keymap[3 * column + row];
   
   /* Send keyboard event */
   if (state) {
      if (key < 128)  /* Not a 'weird' key */
	dispatch_key(TRIGGER_CHAR,key,0);
      dispatch_key(TRIGGER_KEYDOWN,key,0);
   }
   else
     dispatch_key(TRIGGER_KEYUP,key,0);
}


/******************************************** Implementations */

g_error r3912ts_init(void) {
   if (GetPointerCalibrationData())
     return mkerror(PG_ERRT_IO, 74);
   r3912ts_fd = open("/dev/tpanel",O_NONBLOCK);
   if (r3912ts_fd <= 0)
     return mkerror(PG_ERRT_IO, 74);
   
   return sucess;
}

void r3912ts_close(void) {
   close(r3912ts_fd);
}
   
void r3912ts_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
   if ((*n)<(r3912ts_fd+1))
     *n = r3912ts_fd+1;
   FD_SET(r3912ts_fd,readfds);
}

int r3912ts_fd_activate(int fd) {
   struct tpanel_sample ts;
   XYPOINT dev,scr;
   
   /* Read raw data from the driver */
   if (fd!=r3912ts_fd)
     return 0;
   if (read(r3912ts_fd,&ts,sizeof(ts)) < sizeof(ts))
     return 1;
   
   /* Convert to screen coordinates */
   dev.x = ts.x;
   dev.y = ts.y;
   scr = DeviceToScreen(dev);
   scr.x >>= 2;
   scr.y >>= 2;
   
   /* Clip to screen resolution */

   if (scr.x < 0)
     scr.x = 0;
   else if (scr.x >= vid->xres)
     scr.x = vid->xres-1;

   if (scr.y < 0)
     scr.y = 0;
   else if (scr.y >= vid->yres) {
      /* If it's off the bottom of the screen, handle
       * the silk-screened buttons */
#ifdef DRIVER_R3912TS_HELIOSS
      handle_silkscreen_buttons(scr.x,scr.y,ts.state);
      return 1;
#else
      scr.y = vid->yres-1;
#endif
   }
   
   /* Send a pointer event */
   switch (ts.state) {
    case 0:
      dispatch_pointing(TRIGGER_UP,scr.x,scr.y,0);
      break;
    case 1:
      dispatch_pointing(TRIGGER_DOWN,scr.x,scr.y,1);
      break;
    case 2:
      dispatch_pointing(TRIGGER_MOVE,scr.x,scr.y,1);
      break;
   }
   
   return 1;
}
   
/******************************************** Driver registration */

g_error r3912ts_regfunc(struct inlib *i) {
  i->init = &r3912ts_init;
  i->close = &r3912ts_close;
  i->fd_activate = &r3912ts_fd_activate;
  i->fd_init = &r3912ts_fd_init;
  return sucess;
}

/* The End */
