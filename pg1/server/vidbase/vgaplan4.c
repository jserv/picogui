/* $Id$
 *
 * Video Base Library:
 * vgaplan4.c - For VGA compatible 4bpp access, based on linear1.c
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 * Copyright (C) 2002 SSV Embedded Systems GmbH, Arnd Bergmann <abe@ist1.de>
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

 *****************************************************************************
 *  
 * This file was written to support the IPC32/19 / BB1 hardware from 
 * SSV Embedded Systems. It has a VGA compatible hardware with 320x240
 * pixels resolution on a slow 8 bit ISA bus, so the functions are optimized
 * to avoid accessing the VGA hardware directly and instead spend some more
 * CPU cycles trying to find the best method to get the data to screen.
 *
 * This may not be optimal for every case, but it should always give an 
 * acceptable result. With the exception of double buffering, everything should 
 * work on a regular Linux vga16fb with 640x480 as well. If it does not, tell me
 * about it.
 *
 * To use this driver, you will also have to select the options
 *  - Linear Framebuffer @ 1bpp
 *  - Linear Framebuffer @ 4bpp
 *  - Linux framebuffer device
 *  - Fix VR3 framebuffer
 *
 * The functions that are currently the biggest performance hog are 
 * blit_to_screen and blit_from_screen, because the offscreen bitmap format 
 * is very different from the VGA memory layout. 
 * If you want to make them faster, there are basically two possible ways:
 *
 * - change the bitmap format to something more like a planar frame buffer,
 *   so that blitting will be faster. The backdraw is that most likely, 
 *   offscreen /drawing/ will be a lot slower then.
 * - use a different sprite implementation that stores the sprites in 
 *   offscreen VGA memory, i.e. in the physical 0xa0000 segment, but after 
 *   the screen data.  Blitting from there to screen will be a lot faster 
 *   than now and most blit_?_screen calls are used for sprites.
 *   Unfortunately, this memory is limited to 524288 pixels, so for a high
 *   resolution, you won't be able to store much apart from the actual
 *   screen image.
 * 
 * Using the driver for SVGALib should be possible with a few minor changes, 
 * but I have not tested this.
 */

#include <pgserver/common.h>
#include <pgserver/video.h>
#include <pgserver/render.h>

#include <pgserver/appmgr.h>
#include <sys/io.h> /* for iopl */
#include <stdio.h>

/* Macros to easily access the destination bitmap */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)

#define FB_SMEM     (((struct stdbitmap*)sbit)->bits)
#define FB_SBPL     (((struct stdbitmap*)sbit)->pitch)

/* real video memory */
#define FB_VGAMEM  (((struct stdbitmap*)vid->display)->bits)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((y)*FB_BPL+FB_MEM)
#define PIXELBYTE(x,y) (((x)>>3)+LINE(y))

#define SLINE(y)        ((y)*FB_SBPL+FB_SMEM)
#define SPIXELBYTE(x,y) (((x)>>3)+SLINE(y))

/* Table of masks used to isolate pixels within a byte */
static const u8 pxlmask[]  = { 0x80, 0x40, 0x20, 0x10,
                               0x08, 0x04, 0x02, 0x01 };
static const u8 slabmask[] = { 0xFF, 0x7F, 0x3F, 0x1F,
                               0x0F, 0x07, 0x03, 0x01, 0x00 };
static const u8 islabmask[]= { 0x00, 0x80, 0xC0, 0xE0,
                               0xF0, 0xF8, 0xFC, 0xFE, 0xFF };

#define VGA_GC 0x3ce /* graphic controller */
#define VGA_SQ 0x3c4 /* sequencer */

/* standard access functions for VGA graphics controller */
#define SET_VGA_GC(i,x) outw((i) | x << 8, VGA_GC)
#define GET_VGA_GC(i) (outb((i), VGA_GC), inb(VGA_GC+1))

/* cached version, useful for slow ISA cards when writing
   the same value over and over again. */
static unsigned vga_gc_cache[9];
#define SET_C_VGA_GC(i,x) \
	do { \
		if (vga_gc_cache[(i)] != (x)) { \
			outw((i) | (x) << 8, VGA_GC); \
			vga_gc_cache[(i)] = (x); \
		} \
	} while (0)
#define GET_C_VGA_GC(i) vga_gc_cache[(i)]

/* VGA sequencer register */
#define SET_VGA_SQ(i,x) outw((i) | x << 8, VGA_SQ)
#define GET_VGA_SQ(i) (outb((i), VGA_SQ), inb(VGA_SQ+1))

#define SET_SRR(x)  SET_C_VGA_GC (0x00,(x)) /* set/reset */
#define SET_ESR(x)  SET_VGA_GC (0x01,(x)) /* enable set/reset */
#define SET_CCR(x)  SET_VGA_GC (0x02,(x)) /* color compare */
#define SET_FSDR(x) SET_VGA_GC (0x03,(x)) /* function select */
#define SET_RMS(x)  SET_VGA_GC (0x04,(x)) /* read map select */
#define SET_MODE(x) SET_VGA_GC (0x05,(x)) /* mode */
#define SET_MISC(x) SET_VGA_GC (0x06,(x)) /* miscellaneaous */
#define SET_CDC(x)  SET_VGA_GC (0x07,(x)) /* color don't care */
#define SET_BMR(x)  SET_VGA_GC (0x08,(x)) /* bit x */

#define GET_SRR  GET_C_VGA_GC (0x00) /* set/reset */
#define GET_ESR  GET_VGA_GC (0x01) /* enable set/reset */
#define GET_CCR  GET_VGA_GC (0x02) /* color compare */
#define GET_FSDR GET_VGA_GC (0x03) /* function select */
#define GET_RMS  GET_VGA_GC (0x04) /* read map select */
#define GET_MODE GET_VGA_GC (0x05) /* mode */
#define GET_MISC GET_VGA_GC (0x06) /* miscellaneaous */
#define GET_CDC  GET_VGA_GC (0x07) /* color don't care */
#define GET_BMR  GET_VGA_GC (0x08) /* bit x */

#define SET_MMR(x) SET_VGA_SQ (0x02,(x)) /* map mask register */
#define GET_MMR GET_VGA_SQ (0x02)  /* map mask register */

/* read-modify-write cycle: This needs to be done when 
   a part of the memory byte is masked out. */
#define RMW(p,x) do { (void) *(p) ; *(p) = (x); } while(0) 

/* mapping PG_LGOP values to VGA function select register values, 
   where possible */
static const u8 VGA_LGOPS[PG_LGOPMAX] = {
	0x00, /* NULL*/
	0x00, /* NONE */ 0x10, /*  OR */ 0x08, /*  AND */ 0x18, /*  XOR */
	0x00, /* INV  */ 0x10, /* IOR */ 0x08, /* IAND */ 0x18, /* IXOR */
};

/* HACK: Offscreen bitmaps are kept in linear4 format, so we need to 
   import the functions for accessing them. 
   These should be in a header file! */
extern void linear4_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop);
extern hwrcolor linear4_getpixel(hwrbitmap dest, s16 x,s16 y);
extern void linear4_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop);
extern void linear4_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop);
extern void linear4_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, 
			hwrcolor c, s16 lgop);
extern void linear4_blit(hwrbitmap dest, s16 dst_x, s16 dst_y,s16 w, s16 h,
		  hwrbitmap sbit,s16 src_x,s16 src_y, s16 lgop);
extern void linear1_blit(hwrbitmap dest, s16 dst_x, s16 dst_y,s16 w, s16 h,
		  hwrbitmap sbit,s16 src_x,s16 src_y, s16 lgop);
extern void def_scrollblit(hwrbitmap dest, s16 dst_x, s16 dst_y,s16 w, s16 h,
		  hwrbitmap sbit,s16 src_x,s16 src_y, s16 lgop);

#ifdef DEBUG_VIDEO 
/* Get all symbols for profiling */
# define local
# define inline
#else
# define local static
#endif
#ifdef DEBUG_VIDEO
/* print a short note for each function that is called but
   not implemented directly by this driver. */
# define debug_missing(x...) do {	\
	     static int printed;	\
	     if (!printed) {		\
		 fprintf(stderr, "vgaplan4: missing " x); \
		 printed++;		\
	     }				\
         } while (0)
#else
# define debug_missing(x...)
#endif

#if 0
/* Some functions have support for logic operations that should work
   but have not been tested so far. If you enable this and have a function
   using these lgops, the speed will be vastly improved but you might see
   unexpected results on the screen. */
#define EXPERIMENTAL_LGOPS
#endif

#if 0
/* VGA double buffering will work only with a resolution of at most 640x400
   pixels. Since most people have a higher resolution and there are unwanted
   side effects (aka bugs ;-) with double buffering, this is disabled by
   default. */
#define VGA_DOUBLEBUFFER
#endif

/************************************************** Minimum functionality */

/* this is really slow. Calling this function repeatedly should be 
   avoided if possible */
static inline hwrcolor vgaplan4_do_getpixel(hwrbitmap dest, s16 x,s16 y)
{
   hwrcolor ret;
   u8 shift = (7-(x&7));
   volatile u8 *p = PIXELBYTE(x,y);

   SET_RMS(0);
   ret = ((*p) >> shift) & 1;
   SET_RMS(1);
   ret |= (((*p) >> shift) & 1) << 1;
   SET_RMS(2);
   ret |= (((*p) >> shift) & 1) << 2;
   SET_RMS(3);
   ret |= (((*p) >> shift) & 1) << 3;
   return ret;
}

local hwrcolor vgaplan4_getpixel(hwrbitmap dest, s16 x,s16 y)
{
   if (FB_VGAMEM == FB_MEM) 
     return vgaplan4_do_getpixel(dest,x,y);
   else
     return linear4_getpixel(dest,x,y);	   
}

/* reasonable fast pixel setting */
local void vgaplan4_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) { 
   volatile u8 *p;
   
   if (FB_VGAMEM != FB_MEM) {
     linear4_pixel(dest,x,y,c,lgop);
     return;
   }

   switch (lgop) {
      case PG_LGOP_NONE:
	break;
      case PG_LGOP_NULL:
   	return;	      
      case PG_LGOP_INVERT:
	c ^= 0xf;
	break;
      case PG_LGOP_INVERT_OR:
      case PG_LGOP_INVERT_AND:
      case PG_LGOP_INVERT_XOR:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_XOR:
      case PG_LGOP_AND:
      case PG_LGOP_OR:
	SET_FSDR(VGA_LGOPS[lgop]);
	break;
      case PG_LGOP_ADD:
      	c += vgaplan4_do_getpixel(dest,x,y);
	if (c > 0xf) c = 0xf;
	break;
      case PG_LGOP_SUBTRACT:
      	c = vgaplan4_do_getpixel(dest,x,y) - c;
	if (c < 0) c = 0;
	break;
      case PG_LGOP_MULTIPLY:
      	c *= vgaplan4_do_getpixel(dest,x,y);
	if (c > 0xf) c = 0xf;
	break;
      case PG_LGOP_STIPPLE:	
	if (!(x+y)&1) 
	    return;
      default:
	debug_missing("pixel lgop %d\n", lgop);
        def_pixel(dest,x,y,c,lgop);
	return;
   }

   p = PIXELBYTE(x,y);
   SET_SRR(c&0x0f);
   RMW(p,pxlmask[x&7]);

   if (lgop != PG_LGOP_NONE)
	SET_FSDR(0x00);
}

/****************************************************** Basic Drawing */

/* 'Simple' horizontal line */
local void vgaplan4_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop)
{
   int front, back;
   volatile u8 *p;

   /* offscreen bitmap */
   if (FB_VGAMEM != FB_MEM) {
     linear4_slab(dest,x,y,w,c,lgop);
     return;
   }     
 
   switch (lgop) {
      case PG_LGOP_NULL:
   	return;	      
      case PG_LGOP_INVERT:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_NONE:
	break;
#ifdef EXPERIMENTAL_LGOPS
      case PG_LGOP_INVERT_OR:
      case PG_LGOP_INVERT_AND:
      case PG_LGOP_INVERT_XOR:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_XOR:
      case PG_LGOP_AND:
      case PG_LGOP_OR:
	SET_FSDR(VGA_LGOPS[lgop]);
	break;
#endif
      default:
        debug_missing("slab lgop %d\n",lgop);
        def_slab(dest,x,y,w,c,lgop);
	return;
   }


#if 0
   SET_BMR(0xff); /* set this to 0x55/0xaa for stippled slab */
#endif

   SET_SRR(c & 0xf);

   p = PIXELBYTE (x,y);
   front = x & 7;
   back = (x+w) & 7;

   /* slab is contained in one byte, so take a shortcut */
   if (front+w < 8) {
      	u8 mask = slabmask[front];  /* Isolate the necessary pixels */
      	mask  &= islabmask[front+w];
	RMW(p,mask);
	return;
   }

   if (front) {
	RMW(p,slabmask[x&7]);
	p++;
   }

   while (p < PIXELBYTE(x+w,y)) {
	RMW(p,0xff);
	p++;
   }

   if (back) {
	RMW(p,islabmask[back]);
   }
   return;
}

/* fill complete aligned area with the same color, should be near-optimal
   when inlined. Perhaps this should be a macro. */
static inline void 
vgaplan4_fill(volatile u8 *p,int w,int h,hwrcolor c,u8 mask,int stride) {
   for (;h;h--,p+=stride) {
      volatile u8 *q = p;
      int bytes  = w;
      for (; bytes ; bytes--, q++)
	 RMW(q, mask);
   }
}

/* Draw a vertical line using vgaplan4_fill */
local void vgaplan4_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
   /* Compute the masks ahead of time and reuse! */   

   if (FB_VGAMEM != FB_MEM) {
      linear4_bar(dest,x,y,h,c,lgop);
      return;
   } 

   switch (lgop) {
      case PG_LGOP_NULL:
   	return;	      
      case PG_LGOP_INVERT:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_NONE:
	break;
#ifdef EXPERIMENTAL_LGOPS
      case PG_LGOP_INVERT_OR:
      case PG_LGOP_INVERT_AND:
      case PG_LGOP_INVERT_XOR:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_XOR:
      case PG_LGOP_AND:
      case PG_LGOP_OR:
	SET_FSDR(VGA_LGOPS[lgop]);
	break;
#endif
      default:
        debug_missing("bar lgop %x\n",lgop);
        def_bar(dest,x,y,h,c,lgop);
        return;
   }

   SET_SRR(c & 0xf);
   vgaplan4_fill (PIXELBYTE(x,y), 1, h, c, pxlmask[x&7], FB_BPL);

#ifdef EXPERIMENTAL_LGOPS
   SET_FSDR(0x00);
#endif
}

/* draw a filled rectangle using calls to _fill(), this should
   be generic enough to do _bar() and _slab() as well, but the 
   implementation of those functions is as least as good, so I
   left them in. 
   TODO: - maybe an extra _stipple_rect() function setting BMR
  	   that is called by _bar() and _slab() */
local void vgaplan4_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,
		hwrcolor c, s16 lgop)
{
   volatile u8 *p;
   int front,bytes,back;
   if (FB_VGAMEM != FB_MEM) {
     linear4_rect(dest,x,y,w,h,c,lgop);
     return;
   } 

   switch (lgop) {
      case PG_LGOP_NULL:
   	return;	      
      case PG_LGOP_INVERT:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_NONE:
	break;
#ifdef EXPERIMENTAL_LGOPS
      case PG_LGOP_INVERT_OR:
      case PG_LGOP_INVERT_AND:
      case PG_LGOP_INVERT_XOR:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_XOR:
      case PG_LGOP_AND:
      case PG_LGOP_OR:
	SET_FSDR(VGA_LGOPS[lgop]);
	break;
#endif
      default:
	debug_missing("rect lgop %d\n",lgop);
        def_rect(dest,x,y,w,h,c,lgop);
        return;
   }
#if 0
   SET_BMR(0xff); /* set this for stippling */
#endif
   SET_SRR(c & 0xf);

   p = PIXELBYTE (x,y);
   front = x & 7;
   back = (x+w) & 7;

   /* rect is smaller than one byte, so take a shortcut */
   if (front+w < 8) {
 	u8 mask = slabmask[front];  /* Isolate the necessary pixels */
      	mask  &= islabmask[front+w];
        vgaplan4_fill (p, 1, h, c, mask, FB_BPL);
	return;
   }
   
   /* left border */
   if (front) {
        vgaplan4_fill (p, 1, h, c, slabmask[x&7], FB_BPL);
	p++;
   }

   bytes = PIXELBYTE(x+w,y) - p;
   if (bytes) {
        vgaplan4_fill (p, bytes, h, c, 0xff, FB_BPL);
	p+=bytes;
   }

   /* right border */
   if (back) {
   	vgaplan4_fill (p, 1, h, c, islabmask[back], FB_BPL);
   }
#ifdef EXPERIMENTAL_LGOPS
  SET_FSDR(0x00);
#endif
}

/* Raster-optimized version of Bresenham's line algorithm,
   currently untested in 4bpp! */
local void vgaplan4_line(hwrbitmap dest, s16 x1,s16 yy1,s16 x2,s16 yy2,
			hwrcolor c, s16 lgop) {
  volatile u8 *p;
  s16 stepx, stepy;
  s16 dx;
  s16 dy;
  s16 fraction;
  u32 y1 = yy1,y2 = yy2;   /* Convert y coordinates to 32-bits because
			    * they will be converted to framebuffer offsets */
  switch (lgop) {
      case PG_LGOP_NONE:
	break;
      case PG_LGOP_NULL:
   	return;	      
#ifdef EXPERIMENTAL_LGOPS
      case PG_LGOP_INVERT:
	c ^= 0xf;
	break;
      case PG_LGOP_INVERT_OR:
      case PG_LGOP_INVERT_AND:
      case PG_LGOP_INVERT_XOR:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_XOR:
      case PG_LGOP_AND:
      case PG_LGOP_OR:
	SET_FSDR(VGA_LGOPS[lgop]);
	break;
#endif
      default:
	debug_missing("line lgop %d",lgop);
        def_line(dest,x1,y1,x2,y2,c,lgop);
        return;
  }
  
  SET_SRR(c & 0xf);
  SET_BMR(0xff);

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

  p = FB_MEM+(x1>>3)+y1;
  RMW(p,pxlmask[x1&7]);

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
      p = FB_MEM+(x1>>3)+y1;
      RMW(p,pxlmask[x1&7]);
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
       
      p = FB_MEM+(x1>>3)+y1;
      RMW(p,pxlmask[x1&7]);
    }
  }

#ifdef EXPERIMENTAL_LGOPS
  SET_FSDR(0x00);
#endif
}

/****************************************************** Block Transfer */

/* copy offscreen memory to screen, this would be a lot
   faster if the offscreen area had a planar layout instead
   of linear4. 
   Otoh, planar bitmaps are a lot slower when drawing offscreen,
   so there may not necessarily be much to gain. 
   
   TODO: - more lgops if necessary 
 	 - try to make a bit faster (20-30% should be possible) 
 */
static inline void 
vgaplan4_blit_to_screen(hwrbitmap dest, s16 dst_x, s16 dst_y, 
			s16 w, s16 h,
	 		hwrbitmap sbit, s16 src_x, s16 src_y,
			s16 lgop)
{
   volatile u8 *src,*dst;

   switch (lgop) {
      case PG_LGOP_NULL:
   	return;	      
      case PG_LGOP_NONE:
	break;
      case PG_LGOP_XOR:
      case PG_LGOP_AND:
      case PG_LGOP_OR:
	SET_FSDR(VGA_LGOPS[lgop]);
	break;
      default:
        debug_missing("blit to screen lgop %x\n",lgop);
        def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
	return;
   }
   
   SET_MODE(0x02); /* write mode 2 */
   
   /* here, we write columns instead of lines. This makes lookup from 
      linear4_getpixel a bit slower, but saves us from setting color
      _and_ mask for each pixel, because the mask stays the same for
      the whole column. 
      We don't need to care about overwriting the source because we're
      guaranteed to be in a different memory segment */
   for (;w; w--) {
     int h2;
     s16 sy = src_y;
     s16 dy = dst_y;
     int shift = ((src_x %sbit->w)& 1) ? 0 : 4;
     SET_BMR(pxlmask[dst_x&7]);
     dst = PIXELBYTE(dst_x,dy);
     src = (((src_x%sbit->w)>>1))+sbit->bits;
     for (h2=h;h2; h2--) {
	 hwrcolor c;

	 c= (*(src + (sy % sbit->h)*sbit->pitch) >> shift) & 0x0F;
	 
         RMW (dst, c);
	 dst += FB_BPL;
	 sy++;
      }	
      src_x++;
      dst_x++;
   }
   /* reset to standard write mode 3 */
   SET_MODE(0x03);
   SET_BMR(0xff);
   SET_FSDR(0x00);
}

/* copy screen to offscreen memory, see comment fo _blit_to_screen() 
   TODO: avoid calling linear4_pixel() for lgop == PG_LGOP_NONE */
static void 
vgaplan4_blit_from_screen(hwrbitmap dest, s16 dst_x, s16 dst_y, 
			s16 w, s16 h,
	 		hwrbitmap sbit, s16 src_x, s16 src_y,
			s16 lgop)
{
   volatile u8 *p;
   int plane,x,y;
   int sbpl = (w >> 3)+2; /* source bytes per line, leave two 
			     bytes for padding */
   u8 tmp[sbpl][h][4]; /* temporary buffer for all 4 planes, XXX dynamic
			  arrays cannot be inlined! */
   
   /* source is planar, dest is linear, so first 
      copy to a temporary planar buffer */
   for (plane=0; plane<4; plane++)
   {
       SET_RMS (plane);
       for (y=0; y < h; y++) {
           p = (sbit->bits + (src_y+y)*(sbit->pitch) + ((src_x) >> 3)); 
	   				/* p = SPIXELBYTE(src_y+y,src_x); */
           for (x=0; x < sbpl; x++) {
 	       tmp[x][y][plane] = p[x];
 	  }
      }
   }
   /* flipping the bits around:
      src layout:
              /	p0 ---01234567
	plane/	p1 --01234567-
	     \	p2 -01234567--
	      \	p3 01234567---
		    X axis
     dst layout:
		00001111 22223333 44445555 66667777
   */
   for (y=0; y < h; y++) {
       int destbit=0, bit;
       for (x=0; x < sbpl; x++) {
	   int p0=tmp[x][y][0];
	   int p1=tmp[x][y][1]<<1;
	   int p2=tmp[x][y][2]<<2;
	   int p3=tmp[x][y][3]<<3;
#if 0
	   /* this could be used to replace linear4_pixel calls, which
	       currently account for most of the time spent in this function */
	   u8 twopixel;
	   if (dst_x & 1) /* store the other pixel in current byte */
		twopixel = linear4_getpixel (dest,dst_x - 1, dst_y + y) << 4;
#endif
	   for (bit=0;bit<8;bit++) {
	       static const int mask[] = { 0x400, 0x200, 0x100, 
		       0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
	       int realx = (x << 3)+bit + (src_x & ~7);
	       /* upper half */
	       if ((realx >= src_x) && (realx < src_x + w)) {
		    hwrcolor c = ((p0 & mask[bit+3]) 
			        | (p1 & mask[bit+2])
			        | (p2 & mask[bit+1])
			        | (p3 & mask[bit])) >> (7-bit);
		    linear4_pixel(dest,dst_x + destbit,dst_y + y,c,lgop);   
		    destbit++;
	       }
	       realx++; bit++;
	       /* lower half */
	       if ((realx >= src_x) && (realx < src_x + w)) {
		    hwrcolor c = ((p0 & mask[bit+3]) 
			        | (p1 & mask[bit+2])
			        | (p2 & mask[bit+1])
			        | (p3 & mask[bit])) >> (7-bit);
		    linear4_pixel(dest,dst_x + destbit,dst_y + y,c,lgop);   
		    destbit++;
	       }
	   }
       }
   }
}

static inline void
vgaplan4_screen_blitbyte(volatile u8 *dst, volatile u8 *src, u8 mask)
{
   int plane;
   u8 s,d;
   SET_MODE(0x00);
   SET_ESR(0x00);
   SET_SRR(0xf);
   SET_BMR(0xff);
   for (plane=0;plane<4;plane++) {
	SET_RMS(plane);
	SET_MMR(1<<plane);
	d = *dst;
	s = *src;
	*dst = (d & mask) | (s & ~mask);
   }
   SET_MMR(0x0f);
   SET_MODE(0x03);
}

/* aligned blit of a single line, no lgop, dest and sbit are both the vga 
   frame buffer and may overlap.
   This is especially useful for vertical scrolling. */
static inline void 
vgaplan4_screen_blitline(hwrbitmap dest, s16 dst_x, s16 dst_y, s16 w,
	 		 hwrbitmap sbit, s16 src_x, s16 src_y)
{
   volatile u8 *p = PIXELBYTE (dst_x,dst_y);
   volatile u8 *q = SPIXELBYTE (src_x,src_y);
   u8 *end = PIXELBYTE(dst_x+w,dst_y); /* last complete byte */
   
   int front = src_x & 7;
   int back  = (src_x+w) & 7;
   
   /* single byte */
   if (front+w < 8) {
	vgaplan4_screen_blitbyte(p,q,islabmask[front]&slabmask[back]);
	return;
   }
   
   /* front */
   if (front) {
       vgaplan4_screen_blitbyte(p,q,islabmask[front]);
       p++, q++;
   }

   /* center */
   if (p<end) {
       SET_MODE(0x01); /* fast copy mode */
       while (p<end) {
	   *p = *q;    
           p++; q++;
       }
       SET_MODE(0x03);
   }

   /* back */
   if (back) {
       vgaplan4_screen_blitbyte(p,q,slabmask[back]);
   }
}

/* do a 1bpp blit for each plane, this is slower than copying whole
   lines, but it works for the general case
   */
static inline void 
vgaplan4_unaligned_screen_blit(hwrbitmap dest, s16 dst_x, s16 dst_y, 
				s16 w, s16 h,
		 		hwrbitmap sbit, s16 src_x, s16 src_y,
				s16 lgop)
{
   int plane;

   switch (lgop) {
    case PG_LGOP_NONE:
    case PG_LGOP_OR:
    case PG_LGOP_AND:
    case PG_LGOP_XOR:
    case PG_LGOP_INVERT_AND:
      break;
    default:
      def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
      debug_missing("unaligned blit lgop %d", lgop);
      return;
   }

   SET_MODE(0x00);
   SET_ESR(0x00);
   SET_SRR(0xf);
   SET_BMR(0xff);
   SET_FSDR(0x00);
   for (plane=0;plane<4;plane++) {
	SET_RMS(plane);
	SET_MMR(1<<plane);
	linear1_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
   }
   SET_MMR(0x0f);
   SET_MODE(0x03);
}

/* unfinished */
static inline void 
vgaplan4_screen_blit(hwrbitmap dest, s16 dst_x, s16 dst_y, 
			s16 w, s16 h,
	 		hwrbitmap sbit, s16 src_x, s16 src_y,
			s16 lgop)
{
#if 0
   /* no simple put, this should not happen! */
   if (lgop != PG_LGOP_NONE) {
      debug_missing("screen blit lgop %d\n",lgop);
      def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
      return;
   }
#endif
   
   /* can't copy directly, needs to set shift registers */
   if ((dst_x & 7) != (src_x & 7)) {
      vgaplan4_unaligned_screen_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
      return;
   }

   for (;h;h--,dst_y++,src_y++) {
	vgaplan4_screen_blitline (dest,dst_x,dst_y,w,sbit,src_x,src_y);   
   }
}

/* multiplexer for the various special blit calls */
local void vgaplan4_blit(hwrbitmap dest,
		  s16 dst_x, s16 dst_y,s16 w, s16 h,
		  hwrbitmap sbit,s16 src_x,s16 src_y,
		  s16 lgop)
{
   int dest_is_vga = (FB_VGAMEM == FB_MEM);
   int source_is_vga = (FB_VGAMEM == FB_SMEM);

   if (source_is_vga && ! dest_is_vga) {
     vgaplan4_blit_from_screen(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
   } 

   if (dest_is_vga && ! source_is_vga) {
     vgaplan4_blit_to_screen(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
   } 

   if (dest_is_vga && source_is_vga) {
     vgaplan4_screen_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
   }

   if (!dest_is_vga && !source_is_vga) {
     linear4_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
   }
}

/* scrollblit is usually on screen, so we optimize for this case */
local void vgaplan4_scrollblit(hwrbitmap dest,
			 s16 dst_x, s16 dst_y,s16 w, s16 h,
			 hwrbitmap sbit,s16 src_x,s16 src_y,
			 s16 lgop)
{
  /* This version only handles cases where dst_y > src_y */
  if ((FB_VGAMEM != FB_MEM) 
      || (FB_VGAMEM != FB_SMEM)
      || (lgop != PG_LGOP_NONE)
      || (dst_y < src_y)) {
    def_scrollblit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
  }

   /* can't copy directly, needs to shift */
   if ((dst_x & 7) != (src_x & 7)) {
      for (dst_y+=h-1,src_y+=h-1;h;h--,dst_y--,src_y--)
	  vgaplan4_unaligned_screen_blit(dest,dst_x,dst_y,w,1,
			  		sbit,src_x,src_y,lgop);
          return;
   }

   for (dst_y+=h-1,src_y+=h-1;h;h--,dst_y--,src_y--)
      vgaplan4_screen_blitline(dest,dst_x,dst_y,w,sbit,src_x,src_y);
}

/* core of the charblit function
   it might be possible to generalize this enough to use it for
   planar blitting without alignedment contraints as well. */
static inline void 
vgaplan4_do_charblit(hwrbitmap dest,s16 x, s16 y,s16 w,s16 h,u8 *src,int pitch)
{
   volatile u8 *p;
   s16 dx,dy;
   int front,back;
   u8 mask;

   dx = 0;
   front = x&7;
   back  = (x+w)&7;
   
   /* left and right border of the character are in the same byte */
   if (front+w < 8) {
	mask = (slabmask[front] & islabmask[back]);
        p = PIXELBYTE(x+dx,y);
	for (dy=0;dy<h;dy++) {
	     RMW(p,(src[dy*pitch+(dx>>3)] >> front) & mask);
	     p += FB_BPL;
	}
	return;
   }
   
   /* left fragment */
   if (front) {
	mask = (slabmask[front]);
        p = PIXELBYTE(x+dx,y);
	
	for (dy=0;dy<h;dy++) {
  	    RMW(p,(src[dy*pitch+(dx>>3)] >> front) & mask);
	    p += FB_BPL;
	}

	dx += (8-(front));
   } else {
	/* adjust shift value */
	front = 8;
   }

   /* complete bytes in the center */
   for (;x+dx<((x+w)&~7);dx+=8) {
        p = PIXELBYTE(x+dx,y);
        for (dy=0;dy<h;dy++) {
  	    RMW(p,src[dy*pitch+(dx>>3)] << (8-front)
		  | src[dy*pitch+(dx>>3)+1]>>front);
	    p += FB_BPL;
	}
   }
   
   /* right fragment */
   if (back) {
	mask = islabmask[back];
	p = PIXELBYTE(x+dx,y);
	for (dy=0;dy<h;dy++) {
	    RMW(p, (src[dy*pitch+(dx>>3)] << (8-front)
	            | src[dy*pitch+(dx>>3)+1] >> front)
		   & mask);
	    p += FB_BPL;
	}
   }
}

/* blit a single character to screen
   This function is not as simple and fast as it could perhaps be,
   but it is not called that often.
   It may read one byte past the end of chardat[] (is this ok?) and
   some random bugs have showed up in the past
   */
local void 
vgaplan4_charblit(hwrbitmap dest,u8 *chardat,s16 x,s16 y,s16 w,s16 h,
		  s16 lines, s16 angle, hwrcolor c, struct pgquad *clip,
		  s16 lgop, int pitch) {
   /* Pass rotated or skewed blits on somewhere else. Also skip charblits
    * with LGOPs above PG_LGOP_MULTIPLY. If there's clipping involved,
    * don't bother. */
   if (angle || lines || (lgop>PG_LGOP_MULTIPLY) ||
       (clip && (x<clip->x1 || y<clip->y1 || 
		 (x+w)>=clip->x2 || (y+h)>=clip->y2))) {
      def_charblit(dest,chardat,x,y,w,h,lines,angle,c,clip,lgop,pitch);
      return;
   }

   switch (lgop) {
      case PG_LGOP_NULL:
   	return;	      
      case PG_LGOP_INVERT:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_NONE:
	break;
#ifdef EXPERIMENTAL_LGOPS
      case PG_LGOP_INVERT_OR:
      case PG_LGOP_INVERT_AND:
      case PG_LGOP_INVERT_XOR:
	c ^= 0xf;
	/* fallthrough */
      case PG_LGOP_XOR:
      case PG_LGOP_AND:
      case PG_LGOP_OR:
	SET_FSDR(VGA_LGOPS[lgop]);
	break;
#endif
      default:
	debug_missing("charblit lgop %d",lgop);
        def_charblit(dest,chardat,x,y,w,h,lines,angle,c,clip,lgop,pitch);
	return;
   }

   SET_SRR(c&0x0f);

   vgaplan4_do_charblit(dest,x,y,w,h,chardat,pitch);
		   
   if (lgop!=PG_LGOP_NONE)
	SET_FSDR(0x00);
}


/********************************************** Misc stuff */

/* get grey value scaled to 15.99, color space according to NTSC:
   Red 29.9% Green 58.7% Blue 11.4%
   AFAICS, the def_color_pgtohwr function is incorrectly assuming
   33.3% weight for each component.
   */
local hwrcolor vgaplan4_color_pgtohwr(pgcolor c)
{
   return (getred(c)*1230+getgreen(c)*2414+getblue(c)*468)>>16;
}
local pgcolor vgaplan4_color_hwrtopg(hwrcolor c) 
{
  u8 gray = c * 0x11;
  return mkcolor(gray,gray,gray);
}

#ifdef VGA_DOUBLEBUFFER
/* update() uses the fast copy mode to move the changed
   memory from the first half of screen memory to the
   second half (on-screen). */
local void vgaplan4_update(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h)
{
  u8 *src = PIXELBYTE(x,y);
  u8 *dst = src + 32768;
  u8 *s,*d;
  int i;
  w = (w >> 3) + 2;

  SET_MODE(0x01);
  do {
      for (s=src,d=dst,i=w; i; --i)
    	 *(d++) = *(s++);
      src += FB_BPL;
      dst += FB_BPL;
  } while (--h);
  SET_MODE(0x03);
}
#endif


/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_vgaplan4(struct vidlib *vid) {
   setvbl_default(vid);

   if(iopl(3) == -1)
     {
       fprintf(stderr, "oops - must be root!\n");
       exit(1);
     }
#if 0   
   fprintf(stderr, "SRR %x, ESR %x, CCR %x, FSDR %x, RMS %x, MODE %x,"
          " MISC %x, CDC %x, BMR %x",
	  GET_SRR, GET_ESR, GET_CCR, GET_FSDR, GET_RMS, GET_MODE,
	  GET_MODE, GET_CCR, GET_BMR);
#endif

  /*
     Usage of GC registers:
     The functions in this driver assume to find the graphics
     controller in a predefined state, which is defined here.
     If one function needs a different setting (e.g. 
     blit_to_screen uses write mode 2 instead of 3), it has to
     restore the defaults afterwards.
     
     The only registers that never generally have to be preset 
     are SRR and RMS, which contain the color level in write
     mode 3 and the plane to read from.
  */

   SET_SRR  (0xff);
   SET_ESR  (0x0f);
   SET_FSDR (0x00);
   SET_MODE (0x03);
   SET_MISC (0x01);
   SET_BMR  (0xff);
   SET_MMR  (0x0f);

   vid->pixel          = &vgaplan4_pixel;
   vid->getpixel       = &vgaplan4_getpixel;
   vid->slab           = &vgaplan4_slab;
   vid->bar            = &vgaplan4_bar;
   vid->rect           = &vgaplan4_rect;
   vid->blit           = &vgaplan4_blit;
   vid->scrollblit     = &vgaplan4_scrollblit;
   vid->line           = &vgaplan4_line;
   vid->charblit       = &vgaplan4_charblit;
   vid->color_pgtohwr  = &vgaplan4_color_pgtohwr;
   vid->color_hwrtopg  = &vgaplan4_color_hwrtopg;
   
#ifdef VGA_DOUBLEBUFFER
   /* if the screen memory fits at least twice into the frame
      buffer map, enable double buffering. FIXME: Right now, it does
      not get disabled after shutdown, which is bad :-( */
   if (vid->display->h * vid->display->w / 8 < 32768) {
	outw(0x800c,0x3d4); /* set the display start to 0xA8000 */
	vid->update    = &vgaplan4_update;
   }
#endif
}
/* The End */
