/* $Id: linear16.c,v 1.15 2002/03/31 17:16:04 micahjd Exp $
 *
 * Video Base Library:
 * linear16.c - For 16bpp linear framebuffers
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/inlstring.h>    /* inline-assembly __memcpy */
#include <pgserver/video.h>
#include <pgserver/autoconf.h>

#include <stdlib.h> /* For alloca */

/* Macros to easily access the members of vid->display */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)
#define FB_ISNORMAL(bmp,lgop) (lgop == PG_LGOP_NONE && ((struct stdbitmap*)bmp)->bpp == vid->bpp)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((u16 *)((y)*FB_BPL+FB_MEM))
#define PIXELADDR(x,y) ((x)+LINE(y))
#define PIXEL(x,y)     (*PIXELADDR(x,y))

#undef DEBUG

/* Lookup table for alpha blending */
#ifdef CONFIG_FAST_ALPHA
u8 alpha_table[256*128];
#endif

/************************************************** Minimum functionality */

void linear16_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
#ifdef DRIVER_S1D13806
# ifdef DEBUG
  unsigned short * addr = PIXELADDR (x,y);
# endif /* DEBUG */
#endif

  if (!FB_ISNORMAL(dest,lgop)) {
    def_pixel(dest,x,y,c,lgop);
    return;
  }

#ifdef DRIVER_S1D13806
# ifdef DEBUG
   printf ("set %dx%d @ %p\n", x, y, addr);
# endif /* DEBUG */
#endif

   PIXEL(x,y) = c;
}
hwrcolor linear16_getpixel(hwrbitmap dest, s16 x,s16 y) {
#ifdef DRIVER_S1D13806
  unsigned short * addr;
  hwrcolor c;
#endif

  if (!FB_ISNORMAL(dest,PG_LGOP_NONE))
    return def_getpixel(dest,x,y);
  
#ifdef DRIVER_S1D13806
  addr = PIXELADDR (x,y);
  c = * addr;

  return c << 8 | c >> 8;
#else
  return PIXEL(x,y);
#endif
}

/*********************************************** Accelerated (?) primitives */

/* A simple slab function speeds things up a lot compared to def_slab */
void linear16_slab(hwrbitmap dest, s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
  u16 *p;

  if (!FB_ISNORMAL(dest,lgop)) {
    def_slab(dest,x,y,w,c,lgop);
    return;
  }

  p = PIXELADDR(x,y);
  while (w--)
    *(p++) = c;
}

/* Fun-fun-fun blit functions! */
void linear16_blit(hwrbitmap dest,
		   s16 dst_x, s16 dst_y,s16 w, s16 h,
		   hwrbitmap sbit,s16 src_x,s16 src_y,
		   s16 lgop) {
  u16 *dst;
  struct stdbitmap *srcbit = (struct stdbitmap *) sbit;
  s16 i,offset_dst;
  
  if (!FB_ISNORMAL(dest,PG_LGOP_NONE)) {
     def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
  }

  /* We support a few common LGOPs, but supporting all of them would just
   * waste space. */
  switch (lgop) {
   case PG_LGOP_NONE:
   case PG_LGOP_OR:
   case PG_LGOP_AND:
   case PG_LGOP_XOR:
#ifdef CONFIG_FAST_ALPHA
   case PG_LGOP_ALPHA:
#endif
     break;
   default:
     def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
  }
   
  /* Calculations needed by both normal and tiled blits */
  dst = PIXELADDR(dst_x,dst_y);
  offset_dst = (FB_BPL>>1) - w;

  /* The following little macro mess is to repeat the
     loop using different logical operations.
     (Putting the switch inside the loop would be
     easier to read, but much slower)

     You still have to admit that this blitter is _much_ better
     written than the one on the old SDL driver...

     This loop uses __memcpy for the normal blits, and for lgop blits
     it uses loops, performing as much as possible 4 bytes at a time
  */

  /* Normal blit loop */
#define BLITLOOP                                                   \
    for (;h;h--,src+=offset_src,dst+=offset_dst) {                 \
      for (i=w;i;i--,src++,dst++)                                  \
	OP(dst,src);                                               \
    }
  
  /* Tiled blit loop - similar to tileblit() but always restarts the bitmap
   * on a tile boundary, instead of tiling a bitmap section */
#define TILEBLITLOOP                                                      \
   while (h) {                                                            \
      for (;sh && h;sh--,h--,src_line+=(srcbit->pitch>>1),dst+=offset_dst) { \
	 src = src_line + src_x;                                          \
	 swm = (swp < w) ? swp : w;                                       \
	 for (dw=w;dw;) {                                                 \
	    for (sw=swm;sw;sw--,src++,dst++,dw--)                         \
	      OP(dst,src);                                                \
	    src = src_line;                                               \
	    swm = (srcbit->w < dw) ? srcbit->w : dw;                      \
	 }                                                                \
      }                                                                   \
      sh = srcbit->h;                                                     \
      src_line = (u16*) srcbit->bits;                                     \
   }

  /* Operator to perform fast alpha blending */
#ifdef CONFIG_FAST_ALPHA
#if defined(CONFIG_FAST_ALPHA_565)
#define ALPHA_OP(d,s)                                                  \
   {                                                                   \
     u32 rgba = *((u32*)(s++));                                        \
     u16 oldpixel = *d;                                                \
     u8 *atab = alpha_table + ((rgba >> 16) & 0x7F00);                 \
     *d = (((rgba>>16) + atab[(oldpixel&0xF800)>>8] ) << 8) & 0xF800 | \
          (((rgba>>8)  + atab[(oldpixel&0x07E0)>>3] ) << 3) & 0x07E0 | \
          (( rgba      + atab[(oldpixel&0x001F)<<3] ) >> 3) & 0x001F;  \
   }                     
#elif defined(CONFIG_FAST_ALPHA_555)
#define ALPHA_OP(d,s)                                                  \
   {                                                                   \
     u32 rgba = *((u32*)(s++));                                        \
     u16 oldpixel = *d;                                                \
     u8 *atab = alpha_table + ((rgba >> 16) & 0x7F00);                 \
     *d = (((rgba>>16) + atab[(oldpixel&0x7C00)>>7] ) << 7) & 0x7C00 | \
          (((rgba>>8)  + atab[(oldpixel&0x03E0)>>2] ) << 2) & 0x03E0 | \
          (( rgba      + atab[(oldpixel&0x001F)<<3] ) >> 3) & 0x001F;  \
   }                     
#elif defined(CONFIG_FAST_ALPHA_444)
#define ALPHA_OP(d,s)                                                  \
   {                                                                   \
     u32 rgba = *((u32*)(s++));                                        \
     u16 oldpixel = *d;                                                \
     u8 *atab = alpha_table + ((rgba >> 16) & 0x7F00);                 \
     *d = (((rgba>>16) + atab[(oldpixel&0x0F00)>>4] ) << 4) & 0x0F00 | \
           ((rgba>>8)  + atab[oldpixel&0x00F0     ] )       & 0x00F0 | \
          (( rgba      + atab[(oldpixel&0x000F)<<4] ) >> 4) & 0x000F;  \
   }                     
#else
#error Unsupported color mode for fast alpha blending
#endif
#endif /* CONFIG_FAST_ALPHA */

  /* Is this a normal or tiled blit? */
  if (w>(srcbit->w-src_x) || h>(srcbit->h-src_y)) {   /* Tiled */
    u16 *src,*src_line;
    s16 dw,sh,swm,sw,swp;
     
    /* A few calculations for tiled blits */
    src_x %= srcbit->w;
    src_y %= srcbit->h;
    src_line = ((u16*)srcbit->bits) + src_y*(srcbit->pitch>>1); 
    sh = srcbit->h - src_y;
    swp = srcbit->w - src_x;

    switch (lgop) {

    case PG_LGOP_NONE: 
#ifdef CONFIG_NO_VRAM_MEMCPY
#define OP(d,s) (*d=*s)
      TILEBLITLOOP;
#undef OP
#else
       while (h) {
	  for (;sh && h;sh--,h--,src_line+=srcbit->pitch>>1,dst+=offset_dst) {
	     src = src_line + src_x;
	     swm = (swp < w) ? swp : w;
	     for (dw=w;dw;) {
		__memcpy(dst,src,swm<<1);
		dst += swm;
		src = src_line;
		dw -= swm;
		swm = (srcbit->w < dw) ? srcbit->w : dw;
	     }
	  }
	  sh = srcbit->h;
	  src_line = (u16*) srcbit->bits;
       }
#endif
       break;

    case PG_LGOP_OR:
#define OP(d,s) (*d|=*s)
      TILEBLITLOOP;
#undef OP
      break;

    case PG_LGOP_AND:
#define OP(d,s) (*d&=*s)
      TILEBLITLOOP;
#undef OP      
      break;

    case PG_LGOP_XOR:
#define OP(d,s) (*d^=*s)
      TILEBLITLOOP;
#undef OP
      break;

#ifdef CONFIG_FAST_ALPHA
    case PG_LGOP_ALPHA:
#define OP(d,s) ALPHA_OP(d,s)
      TILEBLITLOOP;
#undef OP
      break;
#endif
    }
  }
  else {                                        /* Normal */
    s16 offset_src;
    u16 *src;

    /* Only needed for normal blits */
    src = ((u16*)srcbit->bits) + src_x + src_y*(srcbit->pitch>>1);
    offset_src = (srcbit->pitch>>1) - ((w*srcbit->bpp)>>4);

    switch (lgop) {
    case PG_LGOP_NONE: 
#ifdef CONFIG_NO_VRAM_MEMCPY
      for (;h;h--,src+=offset_src,dst+=offset_dst) {
	for (i=w;i;i--,src++,dst++)
	  *dst = *src;
      }
#else
      for (;h;h--,src+=(srcbit->pitch>>1),dst+=(FB_BPL>>1))
	__memcpy(dst,src,w<<1);
#endif
      break;

    case PG_LGOP_OR:
#define OP(d,s) (*d|=*s)
      BLITLOOP;
#undef OP
      break;

    case PG_LGOP_AND:
#define OP(d,s) (*d&=*s)
      BLITLOOP;
#undef OP      
      break;
      
    case PG_LGOP_XOR:
#define OP(d,s) (*d^=*s)
      BLITLOOP;
#undef OP
      break;
      
#ifdef CONFIG_FAST_ALPHA
    case PG_LGOP_ALPHA:
#define OP(d,s) ALPHA_OP(d,s)
      BLITLOOP;
#undef OP
      break;
#endif
    }
  }
}

#ifdef CONFIG_FAST_BLUR
/* This is the standard blurring algorithm, but with
 * optimizations for 16-bit 5-6-5 framebuffers
 */

void linear16_blur(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, s16 radius) {
  int i, skip, fallback, stride;
  register int p0,p1,p2;   /* 3-pixel buffer for blurring */
  register u16 *p, *stop;

  stride = FB_BPL>>1;
  skip = stride - w - 2;
  fallback = stride * h - 1;

  while (radius--) {

    /* Horizontal blur */
    i = h;
    p = PIXELADDR(x,y);
    p1 = *p;
    p++;
    p2 = *p;
    while (i--) {
      stop = p+w-2;
      while (p++ < stop) {

	/* Shift the buffer */
	p0 = p1;
	p1 = p2;

	/* Shift in the new pixel, expanding it to 32 bits */
	p2 = p[1];
	p2 = (p2&0x1F) | ((p2<<3)&0x3F00) | ((p2<<5)&0x1F0000);

	/* Now we have room to add them all at once, without masking off
	 * the individual color components
	 */
	p0 += p1 + p2;

	/* Now squish it back into a 16-bit color, rolling the divide and shift
	 * operations into one divide.
	 */
	*p = ((((p0 & 0xFF0000) / 96) & 0xF800) |
	      (((p0 & 0x00FF00) / 24) & 0x07E0) |
	      (((p0 & 0x0000FF) / 3 ) & 0x001F));

      }
      p += skip;
    }

    /* Vertical blur */
    i = w;
    p = PIXELADDR(x,y);
    p1 = *p;
    p += stride;
    p2 = *p;
    while (i--) {
      stop = p+fallback;
      while (p < stop) {
	p += stride;

	/* Shift the buffer */
	p0 = p1;
	p1 = p2;

	/* Shift in the new pixel, expanding it to 32 bits */
	p2 = p[stride];
	p2 = (p2&0x1F) | ((p2<<3)&0x3F00) | ((p2<<5)&0x1F0000);

	/* Now we have room to add them all at once, without masking off
	 * the individual color components
	 */
	p0 += p1 + p2;

	/* Now squish it back into a 16-bit color, rolling the divide and shift
	 * operations into one divide.
	 */
	*p = ((((p0 & 0xFF0000) / 96) & 0xF800) |
	      (((p0 & 0x00FF00) / 24) & 0x07E0) |
	      (((p0 & 0x0000FF) / 3 ) & 0x001F));

      }
      p -= fallback;
    }
  }
}
#endif /* CONFIG_FAST_BLUR */


#ifdef CONFIG_FASTER_BLUR
/*
 * This is a different algorigthm with longer buffers and no division.
 * It forces the blur radius to be a power of two. It uses no buffer, just
 * a set of accumulators. This way each pixel requires just a handful of
 * bitshifts and ands, no division. Since the blur is linear rather than
 * the ideal gaussian curve, there's some interlacing to make the effect
 * a little closer.
 */
void linear16_blur(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, s16 radius) {
  int log_diameter, diameter, i, j;
  int shiftsize;
  int r,g,b;        /* Current color sums */
  int skip, stride, fallback, bpl, strideradius;
  u16 *p, *stop;
  u16 color;

  diameter = radius << 1;

  /* This algorithm doesn't work with a radius of 1 */
  if (diameter <= 2)
    diameter = 4;

  /* Find out the next highest power of two from radius, and the log of that */
  for (i=0,j=diameter;j!=1;i++)
    j >>= 1;

  /* If that's the only bit set, the input was already
   * a power of two and we can leave it alone
   */
  if (diameter == 1<<i)
    log_diameter = i;
  else {
    /* Otherwise get the next power of two */
    log_diameter = 1+i;
    diameter = 1<<log_diameter;
  }
  radius = diameter>>1;

  bpl = FB_BPL;
  stride = bpl>>1;
  skip = stride - w + diameter;
  strideradius = stride * radius;
  fallback = stride*(h-diameter);

  /* Horizontal blur, even fields */

  p = PIXELADDR(x+radius,y);
  j = h;
  r = g = b = 0;
  while (j--) {
    stop = p+w-diameter;
    for (;p<stop;p+=2) {

      /* Add the new pixel into the buffer */
      color = p[radius];
      r += color & 0xF800;
      g += color & 0x07E0;
      b += color & 0x001F;

      /* Take the old one away */
      color = p[-radius];
      r -= color & 0xF800;
      g -= color & 0x07E0;
      b -= color & 0x001F;

      /* Shift it into place */
      *p = (((r >> log_diameter) & 0xF800) |
	    ((g >> log_diameter) & 0x07E0) |
	    ((b >> log_diameter) & 0x001F) );
    }
    p = stop + skip;
  }

  /* Horizontal blur, odd fields */

  p = PIXELADDR(x+radius,y);
  j = h;
  r = g = b = 0;
  while (j--) {
    stop = p+w-diameter;
    p++;
    for (;p<stop;p+=2) {

      /* Add the new pixel into the buffer */
      color = p[radius];
      r += color & 0xF800;
      g += color & 0x07E0;
      b += color & 0x001F;

      /* Take the old one away */
      color = p[-radius];
      r -= color & 0xF800;
      g -= color & 0x07E0;
      b -= color & 0x001F;

      /* Shift it into place */
      *p = (((r >> log_diameter) & 0xF800) |
	    ((g >> log_diameter) & 0x07E0) |
	    ((b >> log_diameter) & 0x001F) );
    }
    p = stop + skip;
  }

  /* Vertical blur, even fields */

  p = PIXELADDR(x,y+radius);
  j = w;
  r = g = b = 0;
  while (j--) {
    stop = p+fallback;
    for (;p<stop;p+=bpl) {

      /* Add the new pixel into the buffer */
      color = p[strideradius];
      r += color & 0xF800;
      g += color & 0x07E0;
      b += color & 0x001F;

      /* Take the old one away */
      color = p[-strideradius];
      r -= color & 0xF800;
      g -= color & 0x07E0;
      b -= color & 0x001F;

      /* Shift it into place */
      *p = (((r >> log_diameter) & 0xF800) |
	    ((g >> log_diameter) & 0x07E0) |
	    ((b >> log_diameter) & 0x001F) );
    }
    p = stop - fallback + 1;
  }

  /* Vertical blur, odd fields */

  p = PIXELADDR(x,y+radius);
  j = w;
  r = g = b = 0;
  while (j--) {
    stop = p+fallback;
    p += stride;
    for (;p<stop;p+=bpl) {

      /* Add the new pixel into the buffer */
      color = p[strideradius];
      r += color & 0xF800;
      g += color & 0x07E0;
      b += color & 0x001F;

      /* Take the old one away */
      color = p[-strideradius];
      r -= color & 0xF800;
      g -= color & 0x07E0;
      b -= color & 0x001F;

      /* Shift it into place */
      *p = (((r >> log_diameter) & 0xF800) |
	    ((g >> log_diameter) & 0x07E0) |
	    ((b >> log_diameter) & 0x001F) );
    }
    p = stop - fallback + 1;
  }



}
#endif /* CONFIG_FASTER_BLUR */

/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear16(struct vidlib *vid) {

  setvbl_default(vid);
   
  vid->pixel          = &linear16_pixel;
  vid->getpixel       = &linear16_getpixel;
  vid->slab           = &linear16_slab;
  vid->blit           = &linear16_blit;

#if defined(CONFIG_FAST_BLUR) || defined(CONFIG_FASTER_BLUR)
  vid->blur = &linear16_blur;
#endif

#ifdef CONFIG_FAST_ALPHA
  /* Initialize the alpha blending table */
  {
    u8 *p = alpha_table;
    int a,c;
    
    for (a=0;a<128;a++)
      for (c=0;c<256;c++)
	*(p++) = (c * (128-a)) >> 7;
  }
#endif
}

/* The End */

