/* $Id$
 *
 * Video Base Library:
 * linear32.c - For 32bpp linear framebuffers
 *
 * BIG FAT WARNING:
 * This is just a stub that only implements pixel() getpixel() and the color
 * conversions. A fast linear32 is on the way, but this will substitute for now.
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

#include <pgserver/inlstring.h>    /* inline-assembly __memcpy */
#include <pgserver/video.h>

/* Macros to easily access the destination bitmap */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)
#define FB_ISNORMAL(bmp,lgop) (lgop == PG_LGOP_NONE && ((struct stdbitmap*)bmp)->bpp == vid->bpp)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((u32 *)((y)*FB_BPL+FB_MEM))
#define PIXELADDR(x,y) ((x)+LINE(y))
#define PIXEL(x,y)     (*PIXELADDR(x,y))

/* Lookup table for alpha blending */
#ifdef CONFIG_FAST_ALPHA
u8 alpha_table[256*128];
#endif

/************************************************** Minimum functionality */

void linear32_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
   if (!FB_ISNORMAL(dest,lgop)) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }
   PIXEL(x,y) = c;
}

hwrcolor linear32_getpixel(hwrbitmap dest, s16 x,s16 y) {
  if (!FB_ISNORMAL(dest,PG_LGOP_NONE))
    return def_getpixel(dest,x,y);

  return PIXEL(x,y);
}

/*********************************************** Accelerated (?) primitives */

/* A simple slab function speeds things up a lot compared to def_slab */
void linear32_slab(hwrbitmap dest, s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
  u32 *p;

  if (!FB_ISNORMAL(dest,lgop)) {
    def_slab(dest,x,y,w,c,lgop);
    return;
  }

  p = PIXELADDR(x,y);
  while (w--)
    *(p++) = c;
}

void linear32_rect(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,hwrcolor c,s16 lgop) {
  int offset;
  u32 *p;
  int i;

  if (!FB_ISNORMAL(dest,lgop)) {
    def_rect(dest,x,y,w,h,c,lgop);
    return;
  }

  offset = (FB_BPL>>2)-w;
  
  p = PIXELADDR(x,y);
  for (;h;h--,p+=offset)
    for (i=w;i--;)
      *(p++) = c;
}

void linear32_bar(hwrbitmap dest, s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
  u32 *p;

  if (!FB_ISNORMAL(dest,lgop)) {
    def_bar(dest,x,y,h,c,lgop);
    return;
  }

  p = PIXELADDR(x,y);
  while (h--) {
    *p = c;
    (u8*)p += FB_BPL;
  }
}

/* Fun-fun-fun blit functions! */
void linear32_blit(hwrbitmap dest,
		   s16 dst_x, s16 dst_y,s16 w, s16 h,
		   hwrbitmap sbit,s16 src_x,s16 src_y,
		   s16 lgop) {
  u32 *dst;
  struct stdbitmap *srcbit = (struct stdbitmap *) sbit;
  int i,offset_dst;
  int offset_src;
  u32 *src;

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
   
  dst = PIXELADDR(dst_x,dst_y);
  offset_dst = (FB_BPL>>2) - w;
  src = ((u32*)srcbit->bits) + src_x + src_y*(srcbit->pitch>>2);
  offset_src = (srcbit->pitch>>2) - ((w*srcbit->bpp)>>5);

  /* Normal blit loop */
#define BLITLOOP                                                   \
    for (;h;h--,src+=offset_src,dst+=offset_dst) {                 \
      for (i=w;i;i--,src++,dst++)                                  \
	OP(dst,src);                                               \
    }
  
  /* Operator to perform fast alpha blending */
#ifdef CONFIG_FAST_ALPHA
#define ALPHA_OP(d,s)                                                  \
   {                                                                   \
     u32 rgba = *s;                                                    \
     u32 oldpixel = *d;                                                \
     u8 *atab = alpha_table + ((rgba >> 16) & 0x7F00);                 \
     *d = (((rgba>>16) + atab[ oldpixel          >>16] ) << 16) & 0xFF0000 | \
          (((rgba>>8)  + atab[(oldpixel&0x00FF00)>>8]  ) << 8 ) & 0x00FF00 | \
          (( rgba      + atab[(oldpixel&0x0000FF)   ]  )      ) & 0x0000FF;  \
   }                     
#endif

  switch (lgop) {
  case PG_LGOP_NONE: 
    for (;h;h--,src+=(srcbit->pitch>>2),dst+=(FB_BPL>>2))
      __memcpy(dst,src,w<<2);
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

/* 
 * Optimized version of def_gradient()
 *
 */
void linear32_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		  pgcolor c1, pgcolor c2, s16 lgop) {
  /* Lotsa vars! */
  s32 r_vs,g_vs,b_vs,r_sa,g_sa,b_sa,r_ca,g_ca,b_ca,r_ica,g_ica,b_ica;
  s32 r_vsc,g_vsc,b_vsc,r_vss,g_vss,b_vss,sc_d;
  s32 r_v1,g_v1,b_v1,r_v2,g_v2,b_v2;
  int i,s,c;
  int offset;
  u32 *p;
  int r,g,b;

  if (!FB_ISNORMAL(dest,PG_LGOP_NONE)) {
    def_gradient(dest,x,y,w,h,angle,c1,c2,lgop);
    return;
  }

  /* We support a few common LGOPs, but supporting all of them would just
   * waste space. */
  switch (lgop) {
   case PG_LGOP_NONE:
   case PG_LGOP_ADD:
   case PG_LGOP_SUBTRACT:
   case PG_LGOP_MULTIPLY:
     break;
   default:
     def_gradient(dest,x,y,w,h,angle,c1,c2,lgop);
     return;
  }

  /* Look up the sine and cosine */
  angle %= 360;
  if (angle<0) angle += 360;
  if      (angle <= 90 ) s =  trigtab[angle];
  else if (angle <= 180) s =  trigtab[180-angle];
  else if (angle <= 270) s = -trigtab[angle-180];
  else                   s = -trigtab[360-angle];
  angle += 90;
  if (angle>=360) angle -= 360;
  if      (angle <= 90 ) c =  trigtab[angle];
  else if (angle <= 180) c =  trigtab[180-angle];
  else if (angle <= 270) c = -trigtab[angle-180];
  else                   c = -trigtab[360-angle];

  /* Calculate denominator of the scale value */
  sc_d = h*((s<0) ? -((s32)s) : ((s32)s)) +
    w*((c<0) ? -((s32)c) : ((s32)c));

  /* Decode colors */
  r_v1 = getred(c1);
  g_v1 = getgreen(c1);
  b_v1 = getblue(c1);
  r_v2 = getred(c2);
  g_v2 = getgreen(c2);
  b_v2 = getblue(c2);

  /* Calculate the scale values and 
   * scaled sine and cosine (for each channel) */
  r_vs = ((r_v2<<16)-(r_v1<<16)) / sc_d;
  g_vs = ((g_v2<<16)-(g_v1<<16)) / sc_d;
  b_vs = ((b_v2<<16)-(b_v1<<16)) / sc_d;

  /* Zero the accumulators */
  r_sa = g_sa = b_sa = r_ca = g_ca = b_ca = r_ica = g_ica = b_ica = 0;

  /* Calculate the sine and cosine scales */
  r_vsc = (r_vs*((s32)c)) >> 8;
  r_vss = (r_vs*((s32)s)) >> 8;
  g_vsc = (g_vs*((s32)c)) >> 8;
  g_vss = (g_vs*((s32)s)) >> 8;
  b_vsc = (b_vs*((s32)c)) >> 8;
  b_vss = (b_vs*((s32)s)) >> 8;

  /* If the scales are negative, start from the opposite side */
  if (r_vss<0) r_sa  = -r_vss*h;
  if (r_vsc<0) r_ica = -r_vsc*w; 
  if (g_vss<0) g_sa  = -g_vss*h;
  if (g_vsc<0) g_ica = -g_vsc*w; 
  if (b_vss<0) b_sa  = -b_vss*h;
  if (b_vsc<0) b_ica = -b_vsc*w; 

  if (r_v2<r_v1) r_v1 = r_v2;
  if (g_v2<g_v1) g_v1 = g_v2;
  if (b_v2<b_v1) b_v1 = b_v2;

  /* Framebuffer setup */
  offset = (FB_BPL>>2)-w;
  p = PIXELADDR(x,y);

  /* Finally, the loop! */
  switch (lgop) {

  case PG_LGOP_NONE:
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++,p+=offset)
      for (r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc)
	*(p++) = mkcolor(r_v1 + ((r_ca+r_sa) >> 8),
			 g_v1 + ((g_ca+g_sa) >> 8),
			 b_v1 + ((b_ca+b_sa) >> 8));
    break;

  case PG_LGOP_ADD:
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++,p+=offset)
      for (r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc) {
	r = getred(*p);
	g = getgreen(*p);
	b = getblue(*p);
	r += r_v1 + ((r_ca+r_sa) >> 8);
	g += g_v1 + ((g_ca+g_sa) >> 8);
	b += b_v1 + ((b_ca+b_sa) >> 8);
	if (r>255) r=255;
	if (g>255) g=255;
	if (b>255) b=255;
	*(p++) = mkcolor(r,g,b);
      }
    break;

  case PG_LGOP_SUBTRACT:
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++,p+=offset)
      for (r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc) {
	r = getred(*p);
	g = getgreen(*p);
	b = getblue(*p);
	r -= r_v1 + ((r_ca+r_sa) >> 8);
	g -= g_v1 + ((g_ca+g_sa) >> 8);
	b -= b_v1 + ((b_ca+b_sa) >> 8);
	if (r<0) r=0;
	if (g<0) g=0;
	if (b<0) b=0;
	*(p++) = mkcolor(r,g,b);
      }
    break;

  case PG_LGOP_MULTIPLY:
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++,p+=offset)
      for (r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc) {
	r = getred(*p);
	g = getgreen(*p);
	b = getblue(*p);
	r *= r_v1 + ((r_ca+r_sa) >> 8);
	g *= g_v1 + ((g_ca+g_sa) >> 8);
	b *= b_v1 + ((b_ca+b_sa) >> 8);
	*(p++) = mkcolor(r>>8,g>>8,b>>8);
      }
    break;

  }
}

/* 
 * Optimized version of def_line()
 *
 */
void linear32_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c,s16 lgop) {
  int stepx, stepy, pstepy;
  int dx = x2-x1;
  int dy = y2-y1;
  int fraction;
  u32 *py1;
  
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
    stepy = -1;
    pstepy = -(FB_BPL>>2); 
  } else {
    dy = dy << 1;
    stepy = 1;
    pstepy = (FB_BPL>>2);
  }
  py1 = LINE(y1);

  py1[x1] = c;

  /* Major axis is horizontal */
  if (dx > dy) {
    fraction = dy - (dx >> 1);
    while (x1 != x2) {
      if (fraction >= 0) {
	py1 += pstepy;
	fraction -= dx;
      }
      x1 += stepx;
      fraction += dy;
      
      py1[x1] = c;
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
      py1 += pstepy;
      y1 += stepy;
      fraction += dx;
      
      py1[x1] = c;
    }
  }
}


/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear32(struct vidlib *vid) {
#ifdef CONFIG_FAST_ALPHA
  u8 *p;
  int a,c;
#endif

  setvbl_default(vid);
   
  vid->pixel          = &linear32_pixel;
  vid->getpixel       = &linear32_getpixel;
  vid->slab           = &linear32_slab;
  vid->bar            = &linear32_bar;
  vid->blit           = &linear32_blit;
  vid->rect           = &linear32_rect;
  vid->line           = &linear32_line;
  vid->gradient       = &linear32_gradient;

#ifdef CONFIG_FAST_ALPHA
  /* Initialize the alpha blending table */
  p = alpha_table;
  for (a=0;a<128;a++)
    for (c=0;c<256;c++)
      *(p++) = (c * (128-a)) >> 7;
#endif
}

/* The End */

