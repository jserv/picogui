/* $Id$
 *
 * rrts.c - input driver for ridgerun touch screen 
 *
 * Much of this code is copied from Jay Carlson's Helio patches for the W window
 * system, available at vhl-tools.sourceforge.net
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 * Copyright (C) 2001, RidgeRun, Inc (glonnon@ridgerun.com)
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
 * Eric Christianson (echristi@ridgerun.com)
 * Greg Lonnon (glonnon@ridgerun.com)
 * Alex McMains (aam@ridgerun.com) 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>    /* For sending events */

#include <stdio.h>              /* For reading the device */

int rrts_fd;
int btnstate;

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


int btnstate;
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
      PGKEY_F1, PGKEY_RETURN, PGKEY_F3,
      PGKEY_F2, PGKEY_ESCAPE, PGKEY_F4
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

g_error rrts_init(void) {
   if (GetPointerCalibrationData())
     return mkerror(PG_ERRT_IO, 74);
   rrts_fd = open("/dev/input/rrts0",O_NONBLOCK);
   if (rrts_fd <= 0)
     return mkerror(PG_ERRT_IO, 74);
   
   return success;
}

void rrts_close(void) {
   close(rrts_fd);
}
   
void rrts_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
   if ((*n)<(rrts_fd+1))
     *n = rrts_fd+1;
   FD_SET(rrts_fd,readfds);
}

void rrts_handle_input()
{
 struct mouse_packet 
  {
    int x;
    int y;
    int buttons;
  };

  int n;
  u_long flags = 0;
  struct mouse_packet mouse;
  XYPOINT dev,scr;
  //static FILE *out = 0;
  
  n = read(rrts_fd, &mouse, sizeof(mouse));

  if ( n != sizeof(mouse))
    return;

  //if ( !out )
   // out = fopen("/dev/ttyS0", "w");

  mouse.buttons &= 0x7;

  // Convert to screen coordinates
  dev.x = mouse.x;
  dev.y = mouse.y;
  scr = DeviceToScreen(dev);
  scr.x >>= 2;
  scr.y >>= 2;

  mouse.x = scr.x;
  mouse.y = scr.y;

  //fprintf(out, "mouse.buttons: %d\n", mouse.buttons);

  if ( mouse.buttons && !btnstate )
    {
      //fprintf(out, "TRIGGER_DOWN\n");
      dispatch_pointing(TRIGGER_DOWN,mouse.x,mouse.y,mouse.buttons);
      btnstate=1;
    }
  else
  if ( !mouse.buttons && btnstate )
    {
      //fprintf(out, "TRIGGER_UP\n");
      dispatch_pointing(TRIGGER_UP,mouse.x,mouse.y,mouse.buttons);
      btnstate=0;
    }
  //else
    dispatch_pointing(TRIGGER_MOVE, mouse.x, mouse.y, mouse.buttons);
}

int rrts_fd_activate(int fd)
{
  if ( fd == rrts_fd ) 
    {
      rrts_handle_input();
      return 1;
    }

  return 0;
}

/******************************************** Driver registration */

g_error rrts_regfunc(struct inlib *i) {
  i->init = &rrts_init;
  i->close = &rrts_close;
  i->fd_activate = &rrts_fd_activate;
  i->fd_init = &rrts_fd_init;
  return success;
}

/* The End */
