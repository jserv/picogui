/* $Id: linear1.c,v 1.8 2001/05/29 20:33:35 micahjd Exp $
 *
 * Video Base Library:
 * linear1.c - For 1-bit packed pixel devices (most black and white displays)
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

/* Macros to easily access the members of vid->display */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((y)*FB_BPL+FB_MEM)
#define PIXELBYTE(x,y) (((x)>>3)+LINE(y))

/* Table of masks used to isolate pixels within a byte */
unsigned const char notmask1[]  = { 0x7F, 0xBF, 0xDF, 0xEF,
                                    0xF7, 0xFB, 0xFD, 0xFE };
unsigned const char pxlmask1[]  = { 0x80, 0x40, 0x20, 0x10,
                                    0x08, 0x04, 0x02, 0x01 };
unsigned const char slabmask1[] = { 0xFF, 0x7F, 0x3F, 0x1F,
                                    0x0F, 0x07, 0x03, 0x01 };

/************************************************** Minimum functionality */

void linear1_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
   u8 *p;
   if (lgop != PG_LGOP_NONE) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }
   
   p = PIXELBYTE(x,y);
   if (c)
     *p |= pxlmask1[x&7];
   else
     *p &= notmask1[x&7];
}

hwrcolor linear1_getpixel(hwrbitmap dest, s16 x,s16 y) {
   return ((*PIXELBYTE(x,y)) >> (7-(x&7))) & 1;
}
   
/*********************************************** Accelerated (?) primitives */

/* 'Simple' horizontal line */
void linear1_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
   u8 *p;
   u8 mask, remainder;
   s16 bw;
   
   if (lgop != PG_LGOP_NONE) {
      def_slab(dest,x,y,w,c,lgop);
      return;
   }
   
   p = PIXELBYTE(x,y);
   remainder = x&7;   
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

void linear1_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
   /* Compute the masks ahead of time and reuse! */   
   char *p;
   u8 mask,remainder;

   if (lgop != PG_LGOP_NONE) {
      def_bar(dest,x,y,h,c,lgop);
      return;
   }
   
   remainder = x&7;
   mask  = notmask1[remainder];
   if (c) c = pxlmask1[remainder];
   p = PIXELBYTE(x,y);
   for (;h;h--,p+=FB_BPL) {
      *p &= mask;
      *p |= c;
   }
}

/* Raster-optimized version of Bresenham's line algorithm */
void linear1_line(hwrbitmap dest, s16 x1,s16 yy1,s16 x2,s16 yy2,hwrcolor c,
		  s16 lgop) {
  s16 stepx, stepy;
  s16 dx;
  s16 dy;
  s16 fraction;
  u32 y1 = yy1,y2 = yy2;   /* Convert y coordinates to 32-bits because
			    * they will be converted to framebuffer offsets */
  
  if (lgop != PG_LGOP_NONE) {
     def_line(dest,x1,y1,x2,y2,c,lgop);
     return;
  }
   
  dx = x2-x1;
  dy = y2-y1;

  if (dx<0) { 
    dx = -(dx << 1);
    stepx = -1; 
  } else {
    dx = dx << 1;
    stepx = 1;
  }
  if (dy<0) { 
    dy = -(dy << 1);
    stepy = -FB_BPL; 
  } else {
    dy = dy << 1;
    stepy = FB_BPL;
  }

  y1 *= FB_BPL;
  y2 *= FB_BPL;

  if (c)
     FB_MEM[(x1>>3)+y1] |= pxlmask1[x1&7];
   else
     FB_MEM[(x1>>3)+y1] &= notmask1[x1&7];

  /* Major axis is horizontal */
  if (dx > dy) {
    fraction = dy - (dx >> 1);
    while (x1 != x2) {
      if (fraction >= 0) {
	y1 += stepy;
	fraction -= dx;
      }
      x1 += stepx;
      fraction += dy;

      if (c)
	 FB_MEM[(x1>>3)+y1] |= pxlmask1[x1&7];
       else
	 FB_MEM[(x1>>3)+y1] &= notmask1[x1&7];
    }
  } 
  
  /* Major axis is vertical */
  else {
    fraction = dx - (dy >> 1);
    while (y1 != y2) {
      if (fraction >= 0) {
	x1 += stepx;
	fraction -= dy;
      }
      y1 += stepy;
      fraction += dx;
       
      if (c)
	 FB_MEM[(x1>>3)+y1] |= pxlmask1[x1&7];
       else
	 FB_MEM[(x1>>3)+y1] &= notmask1[x1&7];
    }
  }
}

/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear1(struct vidlib *vid) {
   setvbl_default(vid);
   
   vid->pixel          = &linear1_pixel;
   vid->getpixel       = &linear1_getpixel;
   vid->slab           = &linear1_slab;
   vid->bar            = &linear1_bar;
   vid->line           = &linear1_line;
}

/* The End */

