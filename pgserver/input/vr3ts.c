/* $Id: vr3ts.c,v 1.4 2001/08/10 23:42:32 sbarnes Exp $
 *
 * vr3ts.c - input driver for the Agenda VR3. This contains code from
 *           Agenda's xfree86 patch along with the framework from
 *           the r3912ts driver.
 *
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

#include <linux/tpanel.h> //Needed for the VR41xx tpanel code

int vr3ts_fd;

//Stuff from the xfree86 diff that Agenda made
int scan_interval = 20000; //default scan interval in microseconds (50 Hz)
int scan_settle_time = 480; //default settle time in microseconds
int low_z_limit = 800; //ignore z measurement data below this value
int Vr_enable_transform = 1; //Enable absolute coordinate transform

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

	items = fscanf(f, "%d %d %d %d %d %d %d %d %d %d",
		&tc.a, &tc.b, &tc.c, &tc.d, &tc.e, &tc.f, &tc.s,
		       &scan_interval, &scan_settle_time, &low_z_limit);

        fclose(f);
   
        if ( items < 7 )
	{
		fprintf(stderr, "Improperly formatted pointer calibration file %s.\n",
			cal_filename);
		return -1;
	}

	return 0;
}

inline XYPOINT DeviceToScreen(XYPOINT p) {
	/*
	 * Transform device coordinates to screen coordinates.
	 * Take a point p in device coordinates and return the corresponding
	 * point in fractional screen coodinates (fraction is
	 * 1 / TRANSFORMATION_UNITS_PER_PIXEL).
	 *
	 * It can scale, translate, rotate and/or skew, based on the coefficients
	 * calculated above based on the list of screen vs. device coordinates.
	 */

	static XYPOINT prev;
	/* Slop is how far the pointer has to move before we will allow the output
	 * to change.	Optimal is probably 3/4 of a pixel, but that might not be
	 * enough for some panels that are very noisey.
	 * Using larger values appears to makes the pointer movement more "blocky".
	 * This is noticeable, say,	when drawing a line at a 45 degree angle.
	 */
	/* TODO: make this configurable */
	const short slop = TRANSFORMATION_UNITS_PER_PIXEL * 3 / 4;
	XYPOINT new, out;

	/* Transform */
	new.x = (tc.a * p.x + tc.b * p.y + tc.c) / tc.s;
	new.y = (tc.d * p.x + tc.e * p.y + tc.f) / tc.s;

	/* Hysteresis (thanks to John Siau) */
	if(abs(new.x - prev.x) >= slop)
		out.x = (new.x | 0x3) ^ 0x3;
	else
		out.x = prev.x;

	if(abs(new.y - prev.y) >= slop )
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

static int VrTpanelInit() {
 	/*
	 * Open up the touch-panel device.
	 * Return the fd if successful, or negative if unsuccessful.
	 */

	struct scanparam s;
	int result;
	int fd;

	/* Read the calibration file first for scan interval and settling time. */
	GetPointerCalibrationData();

	/* Open the touch-panel device. */
	fd = open("/dev/tpanel", O_NONBLOCK);
	if(fd < 0) {
		fprintf(stderr, "Error %d opening touch panel\n", errno);
		return -1;
	}

	s.interval = scan_interval;
	s.settletime = scan_settle_time;
	result = ioctl(fd, TPSETSCANPARM, &s);
	if(result < 0 )
		fprintf(stderr, "Error %d, result %d setting scan parameters.\n", result, errno);

	return fd;
}

g_error vr3ts_init(void) {
  vr3ts_fd = VrTpanelInit();
   if (vr3ts_fd <= 0)
     return mkerror(PG_ERRT_IO, 74);
   
   return sucess;
}

void vr3ts_close(void) {
   close(vr3ts_fd);
}
   
void vr3ts_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
   if ((*n)<(vr3ts_fd+1))
     *n = vr3ts_fd+1;
   FD_SET(vr3ts_fd,readfds);
}

// This function is for reading /dev/tpanel (defined later in file)
static int VrTpanelRead(int fd, int *px, int *py, int *pz, unsigned int *pb);
#define LBUTTON		04
#define MBUTTON		02
#define RBUTTON		01

int vr3ts_fd_activate(int fd) {
  //   struct tpanel_sample ts;
   XYPOINT dev,scr;
   static u8 state = 0;
   int trigger;
   static int old_trigger=TRIGGER_UP;
   int x=0,y=0,z=0;
   static oldx=0,oldy=0;
   unsigned int b=0;
   int result;
   int noskip=0;
   result=VrTpanelRead(fd,&x,&y,&z,&b);

   switch (result) {
   case -1:
     // There was some sort of an error
     return 1;
   case 0:
     // No new data is available, so don't dispatch any events
     return 1;
   case 2:
     // We know the position, so use updated data
     noskip=1;
   case 3:
     // If we didn't fall through from above,
     // position data wasn't available, so use old position data
     if (b) {
       if (state)
	 trigger = TRIGGER_MOVE;
       else
	 trigger = TRIGGER_DOWN;
     }
     else {
       if (state)
	 trigger = TRIGGER_UP;
       else
	 return 1;
     }
     break;
   }

   // Avoid sending a flood of TRIGGER_MOVE's if stylus is in
   // the same place as last time
   if (old_trigger==TRIGGER_MOVE && oldx==x && oldy==y)
     return 1;

   old_trigger=trigger;

   // Handle the silkscreen buttons
   // remember that these aren't quite lined up to the screen
   if (x>=-6 && x<=164 && y>242 && trigger == TRIGGER_DOWN) {
     switch ((x+6)/(171/7)) {
     case 0: 
       dispatch_key(TRIGGER_KEYDOWN,PGKEY_F1,0);
       dispatch_key(TRIGGER_KEYUP,PGKEY_F1,0);
       break;
     case 1: 
       dispatch_key(TRIGGER_KEYDOWN,PGKEY_F2,0);
       dispatch_key(TRIGGER_KEYUP,PGKEY_F2,0);
       break;
     case 2: 
       dispatch_key(TRIGGER_KEYDOWN,PGKEY_F3,0);
       dispatch_key(TRIGGER_KEYUP,PGKEY_F3,0);
       break;
     case 3: 
       dispatch_key(TRIGGER_KEYDOWN,PGKEY_F4,0);
       dispatch_key(TRIGGER_KEYUP,PGKEY_F4,0);
       break;
     case 4: 
       dispatch_key(TRIGGER_KEYDOWN,PGKEY_F5,0);
       dispatch_key(TRIGGER_KEYUP,PGKEY_F5,0);
       break;
     case 5: 
       dispatch_key(TRIGGER_KEYDOWN,PGKEY_F6,0);
       dispatch_key(TRIGGER_KEYUP,PGKEY_F6,0);
       break;
     case 6: 
       dispatch_key(TRIGGER_KEYDOWN,PGKEY_F7,0);
       dispatch_key(TRIGGER_KEYUP,PGKEY_F7,0);
       break;
     }
     state = (trigger != TRIGGER_UP);
     return 1; // We don't need to dispatch any pointer info
   }
   /* If we got this far, accept the new state and send the event */
   state = (trigger != TRIGGER_UP);
   if (noskip) {
     // use the newest data
     dispatch_pointing(trigger,x,y,state);
     oldx=x;
     oldy=y;
   } else {
     // use the old data
     dispatch_pointing(trigger,oldx,oldy,state);
   }
   return 1;
}
   
/******************************************** Driver registration */

g_error vr3ts_regfunc(struct inlib *i) {
  i->init = &vr3ts_init;
  i->close = &vr3ts_close;
  i->fd_activate = &vr3ts_fd_activate;
  i->fd_init = &vr3ts_fd_init;
  return sucess;
}

//#define TEST 1
#define show_raw 0
#define show_filtered 1
static int VrTpanelRead(int fd, int *px, int *py, int *pz, unsigned int *pb) {
	/*
	 * This driver returns absolute postions, not deltas.
	 * Returns the position in px, py, pressure in pz, and buttons pb.
	 * Returns -1 on error.
	 * Returns 0 if no new data is available.
	 * Does not return 1 (which is for relative position data).
	 * Returns 2 if position data is absolute (i.e. touch panels).
	 * Returns 3 if position data is not available, but button data is.
	 * This routine does not block.
	 */

	/*
	 * I do some error masking by tossing out really wild data points.
	 * Lower data_change_limit value means pointer get's "left behind" more easily.
	 * Higher value means less errors caught.
	 * The right setting of this value is just slightly higher than the number of
	 * units traversed per sample during a "quick" stroke.
	 * I figure a fast pen scribble can cover about 3000 pixels in one second on
	 * a typical screen.
	 * There are typically about 3 to 6 points of touch-panel resolution per pixel.
	 * So 3000 pixels-per-second * 3 to 6 tp-points-per-pixel / 50 samples-per-second =
	 * 180 to 360 tp points per scan.
	 */
	/* TODO: make this configurable */
	const int data_change_limit = 360;
	static int have_last_data = 0;
	static int last_data_x = 0;
	static int last_data_y = 0;

	/* Skip (discard) next n-samples. */
	static int skip_samples = 0;

	/*
	 * Thanks to John Siau <jsiau@benchmarkmedia.com> for help with the noise filter.
	 * I use an infinite impulse response low-pass filter on the data to filter out
	 * high-frequency noise.  Results look better than a finite impulse response filter.
	 * If I understand it right, the nice thing is that the noise now acts as a dither
	 * signal that effectively increases the resolution of the a/d converter by a few bits
	 * and drops the noise level by about 10db.
	 * Please don't quote me on those db numbers however. :-)
	 * The end result is that the pointer goes from very fuzzy to much more steady.
	 * Hysteresis really calms it down in the end (elsewhere).
	 *
	 * iir_shift_bits effectively sets the number of samples used by the filter
	 * (number of samples is 2^iir_shift_bits).
	 *
	 * Setting iir_shift_bits lower allows shorter "taps" and less pointer lag, but a
	 * fuzzier pointer, which can be especially bad for display update when dragging.
	 *
	 * Setting iir_shift_bits higher requires longer "presses" and causes more pointer lag,
	 * but it provides steadier pointer.
	 *
	 * If you adjust iir_shift_bits, you may also want to adjust the sample interval
	 * in VrTpanelInit. 
	 *
	 * The filter gain is fixed at 8 times the input (gives room for increased resolution
	 * potentially added by the noise-dithering).
	 *
	 * The filter won't start outputing data until iir_count is above iir_output_threshold.
	 */
	const int iir_shift_bits = 2;
	const int iir_sample_depth = (1 << iir_shift_bits);
	const int iir_output_threshold = 1;
	const int iir_gain = 8;
	static int iir_accum_x = 0;
	static int iir_accum_y = 0;
	static int iir_accum_z = 0;
	static int iir_count = 0;
	int iir_out_x;
	int iir_out_y;
	int iir_out_z;

	int data_x, data_y, data_z;

	/* read a data point */
	short data[6];
	int bytes_read;
	bytes_read = read(fd, data, sizeof(data));
	if(bytes_read != sizeof(data)) {
		if(errno == EINTR || errno == EAGAIN)
			return 0;
		return -1;
	}

	/* did we lose any data? */
	if((data[0] & 0x2000))
		fprintf(stderr, "Lost touch panel data\n");

#if TEST
	if(show_raw) {
		printf("%s%s%s%s",
			data[0] & 0x8000 ? "d" : "-",
			data[0] & 0x4000 ? "p" : "-",
			data[0] & 0x2000 ? "s" : "-",
			data[0] & 0x1000 ? "h" : "-"
		);

		if(data[0] & 0x8000) {
			printf(" %5hd %5hd %5hd %s",
				data[2] - data[1],
				data[4] - data[3],
				data[5],
				data[5] <= low_z_limit ? "low" : "   "
			);

			printf(" f %.4hx x %5hd %5hd y %5hd %5hd z %5hd",
				data[0],
				data[1], data[2],
				data[3], data[4],
				data[5]
			);
		}

		printf("\n");
	}
#endif

	/* do we only have contact state data (no position data)? */
	if((data[0] & 0x8000) == 0) {
		/* is it a pen-release? */
		if((data[0] & 0x4000) == 0) {
			/* TODO: in order to provide maximum responsiveness
			 * maybe return the data in the filter
			 * if the filter count is still below
			 * the output threshold.
			 */

			/* reset the limiter */
			have_last_data = 0;

			/* reset the filter */
			iir_count = 0;
			iir_accum_x = 0;
			iir_accum_y = 0;
			iir_accum_z = 0;

			/* skip the next sample (first sample after pen down) */
			skip_samples = 1;

			/* return the pen (button) state only, */
			/* indicating that the pen is up (no buttons are down) */
			*pb = 0;
			return 3;
		}

		/* ignore pen-down since we don't know where it is */
		return 0;
	}

	/* we have position data */

	/* Should we skip this sample? */
	if(skip_samples > 0) {
#if TEST
		printf("Skipping sample.\n");
#endif
		skip_samples--;
		return 0;
	}

	/*
	 * Combine the complementary panel readings into one value (except z)
	 * This effectively doubles the sampling freqency, reducing noise by approx 3db.
	 * Again, please don't quote the 3db figure.
	 * I think it also cancels out changes in the overall resistance of the panel
	 * such as may be caused by changes in panel temperature.
	 */
	data_x = data[2] - data[1];
	data_y = data[4] - data[3];
	data_z = data[5];

	/* isn't z big enough for valid position data? */
	if(data_z <= low_z_limit) {
#if TEST
		printf("Discarding low-z sample.\n");
#endif
		return 0;
	}

	/* has the position changed more than we will allow? */
	if(have_last_data )
		if((abs(data_x - last_data_x) > data_change_limit)
			|| ( abs(data_y - last_data_y) > data_change_limit )) {
#if TEST
			printf("Discarding changed-too-much sample.\n");
#endif
			return 0;
		}

	/* save last position */
	last_data_x = data_x;
	last_data_y = data_y;
	have_last_data = 1;

	/* is filter full? */
	if(iir_count == iir_sample_depth) {
		/* make room for new sample */
		iir_accum_x -= iir_accum_x >> iir_shift_bits;
		iir_accum_y -= iir_accum_y >> iir_shift_bits;
		iir_accum_z -= iir_accum_z >> iir_shift_bits;
		iir_count--;
	}

	/* feed new sample to filter */
	iir_accum_x += data_x;
	iir_accum_y += data_y;
	iir_accum_z += data_z;
	/* TODO: optimize the unnecessary count--/++ when filter full */
	iir_count++;

	/* aren't we over the threshold yet? */
	if(iir_count <= iir_output_threshold)
		return 0;

	/* Touch-panel only reports left button. */
	/* I moved this up here in preparation for the below-threshold
	 * on pen-up filter output TODO above.
	 */
	*pb = LBUTTON;

	/* figure filter output */
	/* TODO: optimize for shifts instead of divides (when possible)? */
	iir_out_x = (iir_accum_x * iir_gain) / iir_count;
	iir_out_y = (iir_accum_y * iir_gain) / iir_count;
	iir_out_z = (iir_accum_z * iir_gain) / iir_count;

#if TEST
	if(show_filtered )
		printf("data: %hd, %hd, %hd	accum: %d, %d, %d	out: %d, %d, %d\n",
			data_x, data_y, data_z,
			iir_accum_x, iir_accum_y, iir_accum_z,
			iir_out_x, iir_out_y, iir_out_z);
#endif

	/* transformation enabled? */
	if(Vr_enable_transform) {
		/* Transform filtered position info to screen coordinates.
		 * Yes, x and y are meant to be swapped.
		 * Odd as it seems, the x value really represents the y position
		 * of the pen and the y value represents the x position of the pen.
		 * This has to do with the way the touch panel hardware works.
		 * It applies a voltage to the y axis to measure x, then applies a
		 * voltage to the x axis to measure y.
		 * X and y values from the touch-panel driver are those
		 * voltage measurements, not position measurements.
		 */
		XYPOINT transformed = {iir_out_y, iir_out_x};
		transformed = DeviceToScreen(transformed);

		/*
		 * HACK: move this from quarter pixels to whole pixels for now
		 * at least until I decide on the right interface to get the
		 * quarter-pixel data up to the next layer.
		 */
		*px = transformed.x >> 2;
		*py = transformed.y >> 2;
	}
	else {
		/*
		 * Return untransformed filtered position info.
		 * Same odd swapping of x and y due to the way that
		 * the touch panel works (see above).
		 */
		*px = iir_out_y;
		*py = iir_out_x;
	}

	/* Return filtered pressure.*/
	*pz = iir_out_z;

	return 2;
}

/* The End */
