/* $Id$
 *
 * if_touchscreen.c - Touchscreen calibration and filtering
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/configfile.h>
#include <pgserver/init.h>      /* For starting tpcal */
#include <pgserver/widget.h>	/* for screen size and pointer owner */
#include <stdio.h>

struct ts_calibration {
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

  int valid;  /* Nonzero if this calibration is valid */
};

void touchscreen_cal_load(struct ts_calibration *tsc, const char *file);
void touchscreen_cal_set(struct ts_calibration *tsc, handle cal_string, int save_to_file);

int touchscreen_filter(int *x, int *y, int pendown);
void touchscreen_pentoscreen(struct ts_calibration *tsc, int *x, int *y);

/* Global calibration */
static const char *calib_file=NULL;
struct ts_calibration ts_global_cal;

/******************************************* Input filter ******/

void infilter_touchscreen_handler(struct infilter *self, u32 trigger, union trigparam *param) {
  struct ts_calibration *tsc;
  struct ts_calibration local_cal;

  /* Is the calibration being set? 
   */
  if (trigger==PG_TRIGGER_TS_CALIBRATE) {
    touchscreen_cal_set(&ts_global_cal,param->mouse.ts_calibration,1);
    return;
  }

  /* Get a calibration structure to use 
   */
  if (param->mouse.ts_calibration) {
    tsc = &local_cal;
    touchscreen_cal_set(tsc,param->mouse.ts_calibration,0);
  }
  else {
    tsc = &ts_global_cal;
  }

  if (touchscreen_filter(&param->mouse.x, &param->mouse.y, param->mouse.btn))
    return;

  touchscreen_pentoscreen(tsc, &param->mouse.x, &param->mouse.y);

  /* Change the event into a normal mouse status event now that it's in screen coords 
   */
  infilter_send(self, PG_TRIGGER_PNTR_STATUS, param);

  /* If the pen is up, move the invisible cursor offscreen */
  if (!param->mouse.btn) {
    param->mouse.x = param->mouse.y = -1;
    infilter_send(self, PG_TRIGGER_MOVE, param);
  }
}

struct infilter infilter_touchscreen = {
  accept_trigs: PG_TRIGGER_TOUCHSCREEN | PG_TRIGGER_TS_CALIBRATE,
  absorb_trigs: PG_TRIGGER_TOUCHSCREEN | PG_TRIGGER_TS_CALIBRATE,
  handler: &infilter_touchscreen_handler,
};


/******************************************* Public functions ******/

g_error touchscreen_init(void) {
  int calwidth, calheight, values;
  g_error e;

  calib_file = get_param_str("pgserver", "pointercal", "/etc/pointercal");
  calwidth   = get_param_int("pgserver", "calwidth", 640);
  calheight  = get_param_int("pgserver", "calheight", 480);
  touchscreen_cal_load(&ts_global_cal, calib_file);

  /* Trigger running the tpcal app if we don't have a valid configuration */
  if (!ts_global_cal.valid) {
    e = childqueue_push(get_param_str("pgserver","tpcal",NULL));
    errorcheck;
  }

  return success;
}

/******************************************* Touchscreen calibration ******/

/* Load touchscreen calibration from file
 */
void touchscreen_cal_load(struct ts_calibration *tsc, const char *file) {
  FILE *fp;
  int values;
  int calwidth, calheight;

  fp=fopen(calib_file, "r");
  if(fp!=NULL)
    {
      values=fscanf(fp, "%d %d %d %d %d %d %d %d %d", &tsc->a, &tsc->b,
		    &tsc->c, &tsc->d, &tsc->e, &tsc->f, &tsc->s, &calwidth, &calheight);

      /* tsc->s will normally be 65536, enough to cover the
       * inaccuracies of this scaling, I hope - otherwise
       * we need to scale it here */
      if(values==9)
	{
	  tsc->a/=calwidth;
	  tsc->b/=calwidth;
	  tsc->c/=calwidth;
	  tsc->d/=calheight;
	  tsc->e/=calheight;
	  tsc->f/=calheight;
	  tsc->valid=1;
	}
      else if(values==7)
	tsc->valid = 1;
      fclose(fp);
    }
}

/* Set the calibration from a string
 */
void touchscreen_cal_set(struct ts_calibration *tsc, handle cal_string, int save_to_file) {
  struct pgstring *str;

  tsc->valid = 0;
  if(iserror(rdhandle((void**)&str, PG_TYPE_PGSTRING, -1,
		      cal_string)) || !str) 
    return;
  if (iserror(pgstring_convert(&str, PGSTR_ENCODE_UTF8, str)))
    return;

  if(sscanf(str->buffer, "COEFFv1 %d %d %d %d %d %d %d",
	    &tsc->a, &tsc->b, &tsc->c, &tsc->d, &tsc->e,
	    &tsc->f, &tsc->s)==7) {
    FILE *fp=NULL;
   
    if (save_to_file) {
      fp=fopen(calib_file, "w");
      if(fp!=NULL)
	{
	  fprintf(fp, "%d %d %d %d %d %d %d %d %d\n",
		  tsc->a, tsc->b, tsc->c, tsc->d, tsc->e, tsc->f, tsc->s,
		  vid->xres, vid->yres);
	  fclose(fp);
	}
    }

    tsc->a/=vid->xres;
    tsc->b/=vid->xres;
    tsc->c/=vid->xres;
    tsc->d/=vid->yres;
    tsc->e/=vid->yres;
    tsc->f/=vid->yres;
    
    tsc->valid = 1;
  }  
}

/* Transform from pen to screen coordinates using the given calibration 
 */
void touchscreen_pentoscreen(struct ts_calibration *tsc, int *x, int *y) {
  int m,n;
  if(tsc->valid && tsc->s) {
    m = (tsc->a * (*x) + tsc->b * (*y) + tsc->c) * vid->xres / tsc->s;
    n = (tsc->d * (*x) + tsc->e * (*y) + tsc->f) * vid->yres / tsc->s;
    *x = m;
    *y = n;
  }
}

/******************************************* Touchscreen IIR filter ******/

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
  const int iir_gain = 1;       /* NOTE: I know this gain should be 8, but for some reason that
				 *       breaks calibration??
				 */
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

/* The End */




