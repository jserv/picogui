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
#include <pgserver/widget.h>	/* vid, for screen size */
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

void touchscreen_pentoscreen(s16 *x, s16 *y)
{
	if(tc.s && touchscreen_calibrated)
	{
		int m, n;
		m=(tc.a**x+tc.b**y+tc.c)*vid->xres/tc.s;
		n=(tc.d**x+tc.e**y+tc.f)*vid->yres/tc.s;
		*x=m;
		*y=n;
	}
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
		case PGDM_INPUT_CALEN:
			touchscreen_calibrated = !param;
			break;
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
				  guru("Touchscreen driver didn't call touchscreen_init()");
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
