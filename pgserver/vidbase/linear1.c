/* $Id: linear1.c,v 1.4 2001/04/11 02:28:59 micahjd Exp $
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
 * 
 * 
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

/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear1(struct vidlib *vid) {
   /* Start with the defaults */
   setvbl_default(vid);
   
   /* Minimum functionality */
   vid->pixel          = &linear1_pixel;
   vid->getpixel       = &linear1_getpixel;
   
   /* Accelerated functions */
   vid->slab           = &linear1_slab;
   vid->bar            = &linear1_bar;
//  vid->line           = &linear1_line;
//   vid->rect           = &linear1_rect;
//  vid->gradient       = &linear1_gradient;
//  vid->dim            = &linear1_dim;
//  vid->scrollblit     = &linear1_scrollblit;
//  vid->charblit       = &linear1_charblit;
//  vid->charblit_v     = &linear1_charblit_v;
//  vid->tileblit       = &linear1_tileblit;
//  vid->blit           = &linear1_blit;
//  vid->unblit         = &linear1_unblit;
}

/* The End */

