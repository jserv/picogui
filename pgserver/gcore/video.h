/* $Id: video.h,v 1.4 2000/04/27 03:27:36 micahjd Exp $
 *
 * video.h - generic hardware defines (common to all drivers)
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#ifndef __H_VIDEO
#define __H_VIDEO

#include <mode.h>
#include <g_error.h>

/* Generic bitmap structure in hardware-dependant format
*/
struct bitmap {
  devbmpt bits;
  int w,h;
  int freebits;    /* Should 'bits' be freed also when bitmap is freed? */
};

/* This defines the current clipping rectangle.  It is necessary for proper
   scrolling!  Passed to all hwr_* functions. Absolute coordinates, just
   like everything else in hardware.h
*/
struct cliprect {
  int x,y;        /* These upper-left and lower-right pixels are both */
  int x2,y2;      /* inside the clipping rect. */
};

g_error hwr_init();
void hwr_release();
void hwr_clear();
void hwr_update();

void hwr_pixel(struct cliprect *clip,int x,int y,devcolort c);
void hwr_slab(struct cliprect *clip,int x,int y,int l,devcolort c);
void hwr_bar(struct cliprect *clip,int x,int y,int l,devcolort c);
void hwr_line(struct cliprect *clip,int x1,int y1,int x2,int y2,devcolort c);
void hwr_rect(struct cliprect *clip,int x,int y,int w,int h,devcolort c);
void hwr_vgradient(struct cliprect *clip,int x,int y,int w,int h,
		   devcolort c1,devcolort c2,int sx,int sy);
void hwr_frame(struct cliprect *clip,int x,int y,int w,int h,devcolort c);
void hwr_dim(struct cliprect *clip);  /* This dims (in a method appropriate
					 to the hardware) all pixels in the
					 clipping rectangle
				      */

#define LGOP_NULL        0   /* Don't blit */
#define LGOP_NONE        1   /* Blit, but don't use an LGOP */
#define LGOP_OR          2
#define LGOP_AND         3
#define LGOP_XOR         4
#define LGOP_INVERT      5
#define LGOP_INVERT_OR   6
#define LGOP_INVERT_AND  7
#define LGOP_INVERT_XOR  8
#define LGOPMAX          8   /* For error-checking */

/* The generic all-purpose blitter.
   Either of the bitmaps can be null, indicating to use the screen
   instead of an off-screen bitmap. lgop is an optional logical operation
   to preform on the blitted data.  See above #define's.
   Clipping rectangle if specified is applied relative to destination
   bitmap.
*/
void hwr_blit(struct cliprect *clip, int lgop,
	      struct bitmap *src, int src_x, int src_y,
	      struct bitmap *dest, int dest_x, int dest_y,
	      int w, int h);

/* This blit is for displaying one glyph of a font.  The 1bpp character data
   is in chardat, the width and height of the glyph in w and h.
   If lines is > 0, then every 'lines' lines, the image is left-shifted
   one pixel.
*/
void hwr_chrblit(struct cliprect *clip, unsigned char *chardat,
		 int dest_x, int dest_y, int w, int h,int lines,devcolort col);

/* Bitmap converters, creating a bitmap structure with various types of
   data. This is how most bitmaps should be created.
*/
g_error hwrbit_xbm(struct bitmap **bmp,
		   unsigned char *data, int w, int h,
		   devcolort fg, devcolort bg);
g_error hwrbit_pnm(struct bitmap **bmp,
		   unsigned char *data,unsigned long datalen);

/* Free an allocated bitmap */
void hwrbit_free(struct bitmap *b);

/* Converts a color from the portable format */
devcolort cnvcolor(unsigned long c);

#endif /* __GUWI_HARDWARE */

/* The End */







