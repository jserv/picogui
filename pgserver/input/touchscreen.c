/*
 * touchscreen.c
 *
 * derived from transform.h which is
 * Copyright (C) 1999 Bradley D. LaRonde <brad@ltc.com>
 *
 * This program is free software; you may redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <pgserver/common.h>
#include <pgserver/configfile.h>
#include <pgserver/widget.h>	/* for screen size and pointer owner */
#include <stdio.h>

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

static const char *calib_file=NULL;
u8 touchscreen_calibrated=0;
static TRANSFORMATION_COEFFICIENTS tc={0,0,0,0,0,0,0};

/*
 * The following filter routine was adapted from the Agenda VR3 touchscreen driver
 * by Bradley D. Laronde. (I think.. please correct this if i'm wrong --Micah)
 *
 * If the filter returns nonzero, the sample should be discarded.
 */
int touchscreen_filter(int *x, int *y, int pendown) {
  
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
  static int last_pendown = 0;
  static int prev_out_x = 0;
  static int prev_out_y = 0;

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
  const int iir_gain = 1;
  static int iir_accum_x = 0;
  static int iir_accum_y = 0;
  static int iir_count = 0;

  /* Pen down */
  if (pendown && !last_pendown) {
    /* reset the limiter */
    have_last_data = 0;
    
    /* reset the filter */
    iir_count = 0;
    iir_accum_x = 0;
    iir_accum_y = 0;
    
    /* skip the next sample (first sample after pen down) */
    skip_samples = 1;

    last_pendown = pendown;
    
    /* ignore pen-down since we don't know where it is */
    return 1;
  }

  /* pen up */
  if (!pendown && last_pendown) {
    /* Always use the penup */
    *x = prev_out_x;
    *y = prev_out_y;
    last_pendown = pendown;
    return 0;
  }

  /* we have position data */
  
  /* Should we skip this sample? */
  if(skip_samples > 0) {
    skip_samples--;
    return 1;
  }

  /* has the position changed more than we will allow? */
  if(have_last_data )
    if((abs(*x - last_data_x) > data_change_limit)
       || ( abs(*y - last_data_y) > data_change_limit )) {
      return 1;
    }
  
  /* save last position */
  last_data_x = *x;
  last_data_y = *y;
  
  have_last_data = 1;
  
  /* is filter full? */
  if(iir_count == iir_sample_depth) {
    /* make room for new sample */
    iir_accum_x -= iir_accum_x >> iir_shift_bits;
    iir_accum_y -= iir_accum_y >> iir_shift_bits;
    iir_count--;
  }

  /* feed new sample to filter */
  iir_accum_x += *x;
  iir_accum_y += *y;
  iir_count++;

  /* aren't we over the threshold yet? */
  if(iir_count <= iir_output_threshold)
    return 1;
  
  /* figure filter output */
  /* TODO: optimize for shifts instead of divides (when possible)? */
  *x = (iir_accum_x * iir_gain) / iir_count;
  *y = (iir_accum_y * iir_gain) / iir_count;

  prev_out_x = *x;
  prev_out_y = *y;
  
  return 0;
}

void touchscreen_pentoscreen(int *x, int *y)
 {
  if(touchscreen_calibrated)
   {
    if(tc.s)
     {		/* apply the calibrated transformation */
      int m, n;
      m=(tc.a**x+tc.b**y+tc.c)*vid->xres/tc.s;
      n=(tc.d**x+tc.e**y+tc.f)*vid->yres/tc.s;
      *x=m;
      *y=n;
     }
   }
#ifndef CONFIG_NOEXCLUSIVE
  else if(pointer_owner)
   {
    /* Dispatch an event for the calibration program */
    unsigned char data[9];	/* x, y, rotation */
    *(s32*)data=htonl(*x);
    *((s32*)data+1)=htonl(*y);
#if defined(CONFIG_ROTATIONBASE_0) || defined(CONFIG_ROTATIONBASE_NOPOINTING)
    data[8]=vid->flags&PG_VID_ROTATEMASK;
#elif defined(CONFIG_ROTATIONBASE_90)
    switch(vid->flags&PG_VID_ROTATEMASK)
     {
      case 0:
	data[8]=PG_VID_ROTATE90;
	break;
      case PG_VID_ROTATE90:
	data[8]=PG_VID_ROTATE180;
	break;
      case PG_VID_ROTATE180:
	data[8]=PG_VID_ROTATE270;
	break;
      case PG_VID_ROTATE270:
	data[8]=0;
	break;
     }
#elif defined(CONFIG_ROTATIONBASE_180)
    switch(vid->flags&PG_VID_ROTATEMASK)
     {
      case 0:
	data[8]=PG_VID_ROTATE180;
	break;
      case PG_VID_ROTATE90:
	data[8]=PG_VID_ROTATE270;
	break;
      case PG_VID_ROTATE180:
	data[8]=0;
	break;
      case PG_VID_ROTATE270:
	data[8]=PG_VID_ROTATE90;
	break;
     }
#elif defined(CONFIG_ROTATIONBASE_270)
    switch(vid->flags&PG_VID_ROTATEMASK)
     {
      case 0:
	data[8]=PG_VID_ROTATE270;
	break;
      case PG_VID_ROTATE90:
	data[8]=0;
	break;
      case PG_VID_ROTATE180:
	data[8]=PG_VID_ROTATE90;
	break;
      case PG_VID_ROTATE270:
	data[8]=PG_VID_ROTATE180;
	break;
     }
#else
#error Err.. what screen rotation are you using?
#endif
    post_event(PG_NWE_CALIB_PENPOS, NULL, sizeof data, pointer_owner, data);
   }
#endif
 }

g_error touchscreen_init(void)
{
	FILE *fp=NULL;
	int calwidth, calheight, values;

	calib_file=get_param_str("pgserver", "pointercal", "/etc/pointercal");
	calwidth=get_param_int("pgserver", "calwidth", 640);
	calheight=get_param_int("pgserver", "calheight", 480);
	fp=fopen(calib_file, "r");
	if(fp!=NULL)
	{
		values=fscanf(fp, "%d %d %d %d %d %d %d %d %d", &tc.a, &tc.b,
		    &tc.c, &tc.d, &tc.e, &tc.f, &tc.s, &calwidth, &calheight);
		/* tc.s will normally be 65536, enough to cover the
		 * inaccuracies of this scaling, I hope - otherwise
		 * we need to scale it here */
		if(values==9)
		 {
		  tc.a/=calwidth;
		  tc.b/=calwidth;
		  tc.c/=calwidth;
		  tc.d/=calheight;
		  tc.e/=calheight;
		  tc.f/=calheight;
		  touchscreen_calibrated=1;
		 }
		else if(values==7)
		  touchscreen_calibrated=1;
		fclose(fp);
	}
	return success;
}

void touchscreen_message(u32 message, u32 param, u32 *ret)
{
	char *str;

	switch(message)
	{
#ifndef CONFIG_NOEXCLUSIVE
		case PGDM_INPUT_CALEN:
			if(pointer_owner)
			  touchscreen_calibrated = !param;
			break;
#endif
		case PGDM_INPUT_SETCAL:
			if(iserror(rdhandle((void**)&str, PG_TYPE_STRING, -1,
							param)) || !str) break;
			if(sscanf(str, "COEFFv1 %d %d %d %d %d %d %d",
					&tc.a, &tc.b, &tc.c, &tc.d, &tc.e,
					&tc.f, &tc.s)==7)
			{
				FILE *fp=NULL;

				if(!calib_file)
				{
				  /* report faulty driver */
#ifdef HAS_GURU
				  guru("Touchscreen driver didn't call touchscreen_init()");
#endif
				}
				else
				  fp=fopen(calib_file, "w");
				if(fp!=NULL)
				 {
				  fprintf(fp, "%d %d %d %d %d %d %d %d %d\n",
				      tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s,
				      vid->xres, vid->yres);
				      fclose(fp);
				 }
				tc.a/=vid->xres;
				tc.b/=vid->xres;
				tc.c/=vid->xres;
				tc.d/=vid->yres;
				tc.e/=vid->yres;
				tc.f/=vid->yres;
			}
			break;
	}
}

/* TODO: jitter compensation */
