/* $Id: linear1.c,v 1.6 2001/04/12 20:09:37 bauermeister Exp $
 *
 * Video Base Library:
 * linear1.c - For 1-bit packed pixel devices (most black and white displays)
 *
 * BIG FAT WARNING:
 * This is just a stub that only implements pixel() getpixel() and the color
 * conversions. A fast linear1 is on the way, but this will substitute for now.
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
#define PIXELBYTE(x,y) (((x)>>3)+LINE(y))

/* Table of masks used to isolate pixels within a byte */
unsigned const char notmask1[]  = { 0x7F, 0xBF, 0xDF, 0xEF,
                                    0xF7, 0xFB, 0xFD, 0xFE };
unsigned const char pxlmask1[]  = { 0x80, 0x40, 0x20, 0x10,
                                    0x08, 0x04, 0x02, 0x01 };
unsigned const char slabmask1[] = { 0xFF, 0x7F, 0x3F, 0x1F,
                                    0x0F, 0x07, 0x03, 0x01 };

/************************************************** Minimum functionality */

void linear1_pixel(int x,int y,hwrcolor c) {
   char *p = PIXELBYTE(x,y);
   if (c)
     *p |= pxlmask1[x&7];
   else
     *p &= notmask1[x&7];
}

hwrcolor linear1_getpixel(int x,int y) {
   return ((*PIXELBYTE(x,y)) >> (7-(x&7))) & 1;
}
   
/*********************************************** Accelerated (?) primitives */

/* 'Simple' horizontal line */
void linear1_slab(int x,int y,int w,hwrcolor c) {
   char *p = PIXELBYTE(x,y);
   u8 mask, remainder = x&7;
   int bw;
   c = c ? 0xFF : 0x00;                    /* Expand color to 8 bits */
   
   /* If the slab is completely contained within one byte,
    * use a different method */
   if ( (remainder + w) < 8 ) {
      mask  = slabmask1[remainder];        /* Isolate the necessary pixels */
      mask &= ~slabmask1[remainder+w];
      *p &= ~mask;
      *p |= c & mask;
      return;
   }
   
   if (remainder) {                        /* Draw the partial byte before */
      mask = slabmask1[remainder];
      *p &= ~mask;
      *p |= c & mask;
      p++;
      w-=8-remainder;
   }
   if (w<1)                                /* That it? */
     return;
   __memset(p,c,bw = (w>>3));              /* Full bytes */
   if (remainder = (w&7)) {                /* Partial byte afterwards */
      p+=bw;
      mask = slabmask1[remainder];
      *p &= mask;
      *p |= c & ~mask;
   }
}

void linear1_bar(int x,int y,int h,hwrcolor c) {
   /* Compute the masks ahead of time and reuse! */   
   char *p = PIXELBYTE(x,y);
   u8 remainder = x&7;
   u8 mask  = notmask1[remainder];
   if (c) c = pxlmask1[remainder];
   
   for (;h;h--,p+=vid->fb_bpl) {
      *p &= mask;
      *p |= c;
   }
}

/*********************************************** 180 deg stuff */

#define X180  (vid->xres-1-x)
#define Y180  (vid->yres-1-y)
#define XW180 (vid->xres-x-w)
#define YH180 (vid->yres-y-h)


void linear1_pixel_180(int x,int y,hwrcolor c) {
  linear1_pixel(X180, Y180, c);
}

hwrcolor linear1_getpixel_180(int x,int y) {
  return linear1_getpixel(X180, Y180);
}

void linear1_slab_180(int x,int y,int w,hwrcolor c) {
  linear1_slab(XW180, Y180, w, c);
}

void linear1_bar_180(int x,int y,int h,hwrcolor c) {
  linear1_bar(X180, YH180, h, c);
}

#ifdef CONFIG_ROTATE180
# define LINEAR1_PIXEL      linear1_pixel_180
# define LINEAR1_GETPIXEL   linear1_getpixel_180
# define LINEAR1_SLAB       linear1_slab_180
# define LINEAR1_BAR        linear1_bar_180
# define LINEAR1_LINE       linear1_line_180
# define LINEAR1_RECT       linear1_rect_180
# define LINEAR1_GRADIENT   linear1_gradient_180
# define LINEAR1_DIM        linear1_dim_180
# define LINEAR1_SCROLLBLIT linear1_scrollblit_180
# define LINEAR1_CHARBLIT   linear1_charblit_180
# define LINEAR1_CHARBLIT_V linear1_charblit_v_180
# define LINEAR1_TILEBLIT   linear1_tileblit_180
# define LINEAR1_BLIT       linear1_blit_180
# define LINEAR1_UNBLIT     linear1_unblit_180
#else
# define LINEAR1_PIXEL      linear1_pixel
# define LINEAR1_GETPIXEL   linear1_getpixel
# define LINEAR1_SLAB       linear1_slab
# define LINEAR1_BAR        linear1_bar
# define LINEAR1_LINE       linear1_line
# define LINEAR1_RECT       linear1_rect
# define LINEAR1_GRADIENT   linear1_gradient
# define LINEAR1_DIM        linear1_dim
# define LINEAR1_SCROLLBLIT linear1_scrollblit
# define LINEAR1_CHARBLIT   linear1_charblit
# define LINEAR1_CHARBLIT_V linear1_charblit_v
# define LINEAR1_TILEBLIT   linear1_tileblit
# define LINEAR1_BLIT       linear1_blit
# define LINEAR1_UNBLIT     linear1_unblit
#endif

/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear1(struct vidlib *vid) {
   /* Start with the defaults */
   setvbl_default(vid);
   
   /* Minimum functionality */
   vid->pixel          = &LINEAR1_PIXEL;
   vid->getpixel       = &LINEAR1_GETPIXEL;
   
   /* Accelerated functions */
   vid->slab           = &LINEAR1_SLAB;
   vid->bar            = &LINEAR1_BAR;
//  vid->line           = &LINEAR1_LINE;
//  vid->rect           = &LINEAR1_RECT;
//  vid->gradient       = &LINEAR1_GRADIENT;
//  vid->dim            = &LINEAR1_DIM;
//  vid->scrollblit     = &LINEAR1_SCROLLBLIT;
//  vid->charblit       = &LINEAR1_CHARBLIT;
//  vid->charblit_v     = &LINEAR1_CHARBLIT_v;
//  vid->tileblit       = &LINEAR1_TILEBLIT;
//  vid->blit           = &LINEAR1_BLIT;
//  vid->unblit         = &LINEAR1_UNBLIT;
}

/* The End */

