/* $Id: linear2.c,v 1.4 2001/04/12 20:09:37 bauermeister Exp $
 *
 * Video Base Library:
 * linear2.c - For 1-bit packed pixel devices (most black and white displays)
 *
 * BIG FAT WARNING:
 * This is just a stub that only implements pixel() getpixel() and the color
 * conversions. A fast linear2 is on the way, but this will substitute for now.
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
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch>
 *   2001-03-23 180 degree rotation
 * 
 */

#include <pgserver/common.h>
#include <pgserver/inlstring.h>    /* inline-assembly __memcpy if possible*/
#include <pgserver/video.h>

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((y)*vid->fb_bpl+vid->fb_mem)
#define PIXELBYTE(x,y) (((x)>>2)+LINE(y))

/* Table of masks used to isolate one pixel within a byte */
unsigned const char notmask2[] = { 0x3F, 0xCF, 0xF3, 0xFC };

/************************************************** Minimum functionality */

void linear2_pixel(int x,int y,hwrcolor c) {
   char *p = PIXELBYTE(x,y);
   *p &= notmask2[x&3];
   *p |= c << ((3-(x&3))<<1);
}
hwrcolor linear2_getpixel(int x,int y) {
   return ((*PIXELBYTE(x,y)) >> ((3-(x&3))<<1)) & 0x03;
}
   
/*********************************************** Accelerated (?) primitives */


   
/*********************************************** 180 deg stuff */

#define X180  (vid->xres-1-x)
#define Y180  (vid->yres-1-y)
#define XW180 (vid->xres-x-w)
#define YH180 (vid->yres-y-h)


void linear2_pixel_180(int x,int y,hwrcolor c) {
  linear2_pixel(X180, Y180, c);
}

hwrcolor linear2_getpixel_180(int x,int y) {
  return linear2_getpixel(X180, Y180);
}

/******** >>>>> waiting for accelerated primitives
void linear2_slab_180(int x,int y,int w,hwrcolor c) {
  linear2_slab(XW180, Y180, w, c);
}

void linear2_bar_180(int x,int y,int h,hwrcolor c) {
  linear2_bar(X180, YH180, h, c);
}

>>>>> waiting for accelerated primitives <<<<< ********/


#ifdef CONFIG_ROTATE180
# define LINEAR2_PIXEL      linear2_pixel_180
# define LINEAR2_GETPIXEL   linear2_getpixel_180
# define LINEAR2_SLAB       linear2_slab_180
# define LINEAR2_BAR        linear2_bar_180
# define LINEAR2_LINE       linear2_line_180
# define LINEAR2_RECT       linear2_rect_180
# define LINEAR2_GRADIENT   linear2_gradient_180
# define LINEAR2_DIM        linear2_dim_180
# define LINEAR2_SCROLLBLIT linear2_scrollblit_180
# define LINEAR2_CHARBLIT   linear2_charblit_180
# define LINEAR2_CHARBLIT_V linear2_charblit_v_180
# define LINEAR2_TILEBLIT   linear2_tileblit_180
# define LINEAR2_BLIT       linear2_blit_180
# define LINEAR2_UNBLIT     linear2_unblit_180
#else
# define LINEAR2_PIXEL      linear2_pixel
# define LINEAR2_GETPIXEL   linear2_getpixel
# define LINEAR2_SLAB       linear2_slab
# define LINEAR2_BAR        linear2_bar
# define LINEAR2_LINE       linear2_line
# define LINEAR2_RECT       linear2_rect
# define LINEAR2_GRADIENT   linear2_gradient
# define LINEAR2_DIM        linear2_dim
# define LINEAR2_SCROLLBLIT linear2_scrollblit
# define LINEAR2_CHARBLIT   linear2_charblit
# define LINEAR2_CHARBLIT_V linear2_charblit_v
# define LINEAR2_TILEBLIT   linear2_tileblit
# define LINEAR2_BLIT       linear2_blit
# define LINEAR2_UNBLIT     linear2_unblit
#endif

/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear2(struct vidlib *vid) {
  /* Start with the defaults */
  setvbl_default(vid);

  /* Minimum functionality */
  vid->pixel          = &linear2_pixel;
  vid->getpixel       = &linear2_getpixel;
   
  /* Accelerated functions */
//  vid->slab           = &linear2_slab;
//  vid->bar            = &linear2_bar;
//  vid->line           = &linear2_line;
//  vid->rect           = &linear2_rect;
//  vid->gradient       = &linear2_gradient;
//  vid->dim            = &linear2_dim;
//  vid->scrollblit     = &linear2_scrollblit;
//  vid->charblit       = &linear2_charblit;
//  vid->charblit_v     = &linear2_charblit_v;
//  vid->tileblit       = &linear2_tileblit;
//  vid->blit           = &linear2_blit;
//  vid->unblit         = &linear2_unblit;
}

/* The End */

