/* $Id$
 *
 * Video Base Library:
 * linear2.c - For 2-bit packed pixel devices (4 grayscales)
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
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch>
 *   2001-03-23 180 degree rotation
 * 
 */

#include <pgserver/common.h>
#include <pgserver/inlstring.h>    /* inline-assembly __memcpy if possible*/
#include <pgserver/video.h>

/* Macros to easily access the destination bitmap */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)
#define FB_ISNORMAL(bmp,lgop) (lgop == PG_LGOP_NONE && ((struct stdbitmap*)bmp)->bpp == vid->bpp)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((y)*FB_BPL+FB_MEM)
#define PIXELBYTE(x,y) (((x)>>2)+LINE(y))

/* Table of masks used to isolate one pixel within a byte */
const u8 notmask2[]  = { 0x3F, 0xCF, 0xF3, 0xFC };
const u8 slabmask2[] = { 0xFF, 0x3F, 0x0F, 0x03, 0x00 };

/************************************************** Minimum functionality */

void linear2_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
  char *p;
  if (!FB_ISNORMAL(dest,lgop)) {
    def_pixel(dest,x,y,c,lgop);
    return;
  }
  p = PIXELBYTE(x,y);
  *p &= notmask2[x&3];
  *p |= (c & 3) << ((3-(x&3))<<1);
}
hwrcolor linear2_getpixel(hwrbitmap dest, s16 x, s16 y) {
  if (!FB_ISNORMAL(dest,PG_LGOP_NONE))
    return def_getpixel(dest,x,y);

  return ((*PIXELBYTE(x,y)) >> ((3-(x&3))<<1)) & 0x03;
}
   
/*********************************************** Accelerated (?) primitives */

/* 'Simple' horizontal line with stippling */
void linear2_slab_stipple(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c) {
   u8 *p;
   u8 mask, remainder, stipple;
   s16 bw;
   
   p = PIXELBYTE(x,y);
   remainder = x&3;
   stipple = y&1 ? 0xCC : 0x33;
   c = c | (c<<2) | (c<<4) | (c<<6);       /* Expand color to 8 bits */
      
   /* If the slab is completely contained within one byte,
    * use a different method */
   if ( (remainder + w) < 4 ) {
      mask  = slabmask2[remainder];        /* Isolate the necessary pixels */
      mask &= ~slabmask2[remainder+w];
      mask &= stipple;
      *p &= ~mask;
      *p |= c & mask;
      return;
   }
   
   if (remainder) {                        /* Draw the partial byte before */
      mask = slabmask2[remainder];
      mask &= stipple;
      *p &= ~mask;
      *p |= c & mask;
      p++;
      w-=4-remainder;
   }
   if (w<1)                                /* That it? */
     return;
   
   /* Full bytes */
   c &= stipple;
   for (bw = w >> 2;bw;bw--,p++) {
      *p &= ~stipple;
      *p |= c;
   }
   
   if ((remainder = (w&3))) {              /* Partial byte afterwards */
      mask = slabmask2[remainder];
      mask = (~mask) & stipple;
      *p &= ~mask;
      *p |= c & mask;
   }
}

/* 'Simple' horizontal line */
void linear2_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
   u8 *p;
   u8 mask, remainder;
   s16 bw;
   
   if (lgop == PG_LGOP_STIPPLE) {
      linear2_slab_stipple(dest,x,y,w,c);
      return;
   }
   else if (!FB_ISNORMAL(dest,lgop)) {
      def_slab(dest,x,y,w,c,lgop);
      return;
   }

   c = c | (c<<2) | (c<<4) | (c<<6);      /* Expand color to 8 bits */
   
   p = PIXELBYTE(x,y);
   remainder = x&3;   
   
   /* If the slab is completely contained within one byte,
    * use a different method */
   if ( (remainder + w) < 4 ) {
      mask  = slabmask2[remainder];        /* Isolate the necessary pixels */
      mask &= ~slabmask2[remainder+w];
      *p &= ~mask;
      *p |= c & mask;
      return;
   }
   
   if (remainder) {                        /* Draw the partial byte before */
      mask = slabmask2[remainder];
      *p &= ~mask;
      *p |= c & mask;
      p++;
      w-=4-remainder;
   }
   if (w<1)                                /* That it? */
     return;
   bw = w>>2;
   __memset(p,c,bw);                       /* Full bytes */
   if ((remainder = (w&3))) {              /* Partial byte afterwards */
      p+=bw;
      mask = slabmask2[remainder];
      *p &= mask;
      *p |= c & ~mask;
   }
}

void linear2_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
   /* Compute the masks ahead of time and reuse! */   
   char *p;
   u8 mask,remainder;

   if (!FB_ISNORMAL(dest,lgop)) {
     def_bar(dest,x,y,h,c,lgop);
     return;
   }

   remainder = x&3;
   mask  = notmask2[remainder];
   c <<= (3-remainder) << 1;
   p = PIXELBYTE(x,y);
   for (;h;h--,p+=FB_BPL) {
     *p &= mask;
     *p |= c;
   }
}

/* Raster-optimized version of Bresenham's line algorithm */
void linear2_line(hwrbitmap dest, s16 x1,s16 yy1,s16 x2,s16 yy2,hwrcolor c,
		  s16 lgop) {
  s16 stepx, stepy;
  s16 dx;
  s16 dy;
  s16 fraction;
  u32 y1 = yy1,y2 = yy2;   /* Convert y coordinates to 32-bits because
			    * they will be converted to framebuffer offsets */
  u8 *p;
   
  if (!FB_ISNORMAL(dest,lgop)) {
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

  p = FB_MEM + (x1>>2) + y1;
  *p &= notmask2[x1&3];
  *p |= c << ((3-(x1&3))<<1);

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

      p = FB_MEM + (x1>>2) + y1;
      *p &= notmask2[x1&3];
      *p |= c << ((3-(x1&3))<<1);
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
       
      p = FB_MEM + (x1>>2) + y1;
      *p &= notmask2[x1&3];
      *p |= c << ((3-(x1&3))<<1);
    }
  }
}

/*
 * This is a relatively complicated 1bpp packed-pixel blit that does
 * handle LGOPs but currently doesn't handle tiling by itself.
 * Note that it can read (but not modify) one byte past the boundary of the
 * bitmap, but this is alright.
 */
void linear2_blit(hwrbitmap dest,
		  s16 dst_x, s16 dst_y,s16 w, s16 h,
		  hwrbitmap sbit,s16 src_x,s16 src_y,
		  s16 lgop) {
   u8 *src, *srcline, *dst, *dstline, mask;
   struct stdbitmap *srcbit = (struct stdbitmap *) sbit;
   int bw,xb,s,rs,tp,lp,rlp;
   int i;

   if (!FB_ISNORMAL(dest,PG_LGOP_NONE)) {
     def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
   }
  
   /* Pass on the blit if it is an unsupported LGOP */
   switch (lgop) {
    case PG_LGOP_NONE:
    case PG_LGOP_OR:
    case PG_LGOP_AND:
    case PG_LGOP_XOR:
      break;
    default:
      def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
      return;
   }

   /* Initializations */ 
   src = srcline = srcbit->bits + (src_x>>2) + src_y*srcbit->pitch;
   dst = dstline = PIXELBYTE(dst_x,dst_y);
   xb = dst_x & 3;
   s = (src_x & 3) - xb;
   if (s<0) {
      s += 4;
      src=--srcline;
   }
   s <<= 1;
   rs = 8-s;

   /* The blitter core is a macro so various LGOPs can be used */
   
#define BLITCORE                                                          \
   /* Special case when it fits entirely within one byte */               \
   if ((xb+w)<=4) {                                                       \
      mask = slabmask2[xb] & ~slabmask2[xb+w];                            \
      for (;h;h--,src+=srcbit->pitch,dst+=FB_BPL)                         \
	BLITCOPY(((src[0] << s) | (src[1] >> rs)),mask);                  \
   }                                                                      \
   else {                                                                 \
      tp = (dst_x+w)&3;        /* Trailing pixels */                      \
      lp = (4-xb)&3;           /* Leading pixels */                       \
      bw = (w-tp-lp)>>2;       /* Width in whole bytes */                 \
      rlp = 4-lp;                                                         \
                                                                          \
      /* Bit-banging blitter loop */                                      \
      for (;h;h--,src=srcline+=srcbit->pitch,dst=dstline+=FB_BPL) {       \
	 if (lp) {                                                        \
	    BLITCOPY(((src[0] << s) | (src[1] >> rs)),slabmask2[rlp]);    \
	    src++,dst++;                                                  \
	 }                                                                \
	 for (i=bw;i>0;i--,src++,dst++)                                   \
	   BLITMAINCOPY(((src[0] << s) | (src[1] >> rs)));                \
	 if (tp)                                                          \
	    BLITCOPY(((src[0] << s) | (src[1] >> rs)),(~slabmask2[tp]));  \
      }                                                                   \
   }
   
   /* Select a blitter based on the current LGOP mode */
   switch (lgop) {
   
    case PG_LGOP_NONE:
#define BLITCOPY(d,m)   *dst = (d & m) | (*dst & ~m)
#define BLITMAINCOPY(d) *dst = d
   BLITCORE
#undef BLITMAINCOPY
#undef BLITCOPY
	return;

    case PG_LGOP_OR:
#define BLITCOPY(d,m)   *dst |= d & m
#define BLITMAINCOPY(d) *dst |= d
   BLITCORE
#undef BLITMAINCOPY
#undef BLITCOPY
	return;
      
    case PG_LGOP_AND:
#define BLITCOPY(d,m)   *dst &= d | ~m
#define BLITMAINCOPY(d) *dst &= d
   BLITCORE
#undef BLITMAINCOPY
#undef BLITCOPY
	return;
      
    case PG_LGOP_XOR:
#define BLITCOPY(d,m)   *dst ^= d & m
#define BLITMAINCOPY(d) *dst ^= d
   BLITCORE
#undef BLITMAINCOPY
#undef BLITCOPY
	return;  
      
   }
}
   
/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear2(struct vidlib *vid) {
  setvbl_default(vid);

  vid->pixel          = &linear2_pixel;
  vid->getpixel       = &linear2_getpixel;
  vid->slab           = &linear2_slab;
  vid->bar            = &linear2_bar;
  vid->line           = &linear2_line;
  vid->blit           = &linear2_blit;
}

/* The End */

