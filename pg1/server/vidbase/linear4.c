/* $Id$
 *
 * Video Base Library:
 * linear4.c - For 4-bit grayscale framebuffers
 *
 * A little more complex than linear8, but this makes the
 * 68EZ328 look nice!
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
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch>
 *         2001-03-23  180 degree rotation
 *
 *   Daniel Potts <danielp@cse.unsw.edu.au>
 *         2002-07-08  Rewrote charblit, improved text speed 3x
 * 
 */

#include <pgserver/common.h>

#include <pgserver/inlstring.h>    /* inline-assembly __memcpy if possible*/
#include <pgserver/video.h>
#include <pgserver/render.h>

/* Macros to easily access the destination bitmap */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)
#define FB_ISNORMAL(bmp,lgop) (lgop == PG_LGOP_NONE && ((struct stdbitmap*)bmp)->bpp == vid->bpp)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((y)*FB_BPL+FB_MEM)
#define PIXELBYTE(x,y) ((unsigned char *)(((x)>>1)+LINE(y)))

/* The Psion uses a framebuffer with the high and low nibble reversed */
#ifdef CONFIG_FB_PSION
#define SWAP_NYBBLES
#endif

/* Table of masks used to isolate one pixel within a byte */
#ifdef SWAP_NYBBLES
unsigned const char notmask4[]  = { 0xF0, 0x0F };
unsigned const char slabmask4[] = { 0xFF, 0xF0, 0x00 };
#else
unsigned const char notmask4[]  = { 0x0F, 0xF0 };
unsigned const char slabmask4[] = { 0xFF, 0x0F, 0x00 };
#endif


/************************************************** Minimum functionality */

void linear4_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  unsigned char *p;
  if (!FB_ISNORMAL(dest,lgop)) {
    def_pixel(dest,x,y,c,lgop);
    return;
  }
  p = PIXELBYTE(x,y);

#ifdef SWAP_NYBBLES
  if (x & 1) {
    *p = (*p&0x0F)  | ((c&0x0F) << 4);
  } else {
    *p = (*p&0xF0) | (c&0x0F);
  }
#else 
  if (x&1) {
    *p = (*p&0xF0) | (c&0x0F);
  } else {
    *p = (*p&0x0F)  | ((c&0x0F) << 4);
  }
#endif
}


hwrcolor linear4_getpixel(hwrbitmap dest,s16 x,s16 y) {
  if (!FB_ISNORMAL(dest,PG_LGOP_NONE))
    return def_getpixel(dest,x,y);
  
#ifdef SWAP_NYBBLES
  return ((*PIXELBYTE(x,y)) >> ((x&1)<<2)) & 0x0F;
#else
  return ((*PIXELBYTE(x,y)) >> ((1-(x&1))<<2)) & 0x0F;
#endif
}
   
/************************************************** Accelerated (?) primitives */

/* 'Simple' horizontal line with stippling */
void linear4_slab_stipple(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c) {
   u8 *p;
   u8 mask, remainder, stipple;
   s16 bw;
   
   p = PIXELBYTE(x,y);
   remainder = x&1;
   stipple = y&1 ? 0xF0 : 0x0F;
   c = c | (c<<4);                         /* Expand color to 8 bits */
      
   /* If the slab is completely contained within one byte,
    * use a different method */
   if ( (remainder + w) <= 2 ) {
      mask  = slabmask4[remainder];        /* Isolate the necessary pixels */
      mask &= ~slabmask4[remainder+w];
      mask &= stipple;
      *p &= ~mask;
      *p |= c & mask;
      return;
   }
   
   if (remainder) {                        /* Draw the partial byte before */
      mask = slabmask4[remainder];
      mask &= stipple;
      *p &= ~mask;
      *p |= c & mask;
      p++;
      w-=2-remainder;
   }
   if (w<1)                                /* That it? */
     return;

   /* Full bytes */
   c &= stipple;
   for (bw = w >> 1;bw;bw--,p++) {
      *p &= ~stipple;
      *p |= c;
   }

   if ((remainder = (w&1))) {              /* Partial byte afterwards */
      mask = slabmask4[remainder];
      mask = (~mask) & stipple;
      *p &= ~mask;
      *p |= c & mask;
   }
}

/* 'Simple' horizontal line */
void linear4_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
   u8 *p;
   u8 mask, remainder;
   s16 bw;
   
   if (lgop == PG_LGOP_STIPPLE) {
      linear4_slab_stipple(dest,x,y,w,c);
      return;
   }
   else if (!FB_ISNORMAL(dest,lgop)) {
      def_slab(dest,x,y,w,c,lgop);
      return;
   }
  
   c = c | (c<<4);
   
   p = PIXELBYTE(x,y);
   remainder = x&1;
   
   /* If the slab is completely contained within one byte,
    * use a different method */
   if ( (remainder + w) < 2 ) {
      mask  = slabmask4[remainder];        /* Isolate the necessary pixels */
      mask &= ~slabmask4[remainder+w];
      *p &= ~mask;
      *p |= c & mask;
      return;
   }
   
   if (remainder) {                        /* Draw the partial byte before */
      mask = slabmask4[remainder];
      *p &= ~mask;
      *p |= c & mask;
      p++;
      w-=2-remainder;
   }
   if (w<1)                                /* That it? */
     return;
   bw = w>>1;
   __memset(p,c,bw);                       /* Full bytes */
   if ((remainder = (w&1))) {              /* Partial byte afterwards */
      p+=bw;
      mask = slabmask4[remainder];
      *p &= mask;
      *p |= c & ~mask;
   }
}

void linear4_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
   char *p;
   unsigned char mask;

   if (!FB_ISNORMAL(dest,lgop)) {
      def_bar(dest,x,y,h,c,lgop);
      return;
   }

   /* Compute the masks ahead of time and reuse! */
   p = PIXELBYTE(x,y);
   mask = notmask4[x&1];
#ifdef SWAP_NYBBLES
   if (x&1) c <<= 4;
#else
   if (!(x&1)) c <<= 4;
#endif

   for (;h;h--,p+=FB_BPL) {
      *p &= mask;
      *p |= c;
   }
}

/* Raster-optimized version of Bresenham's line algorithm */
void linear4_line(hwrbitmap dest,s16 x1,s16 yy1,s16 x2,s16 yy2,
		  hwrcolor c,s16 lgop) {
  s16 stepx, stepy;
  s16 dx,dy;
  s16 fraction;
  u32 y1 = yy1,y2 = yy2;   /* Convert y coordinates to 32-bits because
			    * they will be converted to framebuffer offsets */
   
  char *p;
   
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

  p = FB_MEM + y1 + (x1>>1);
  *p &= notmask4[x1&1];
#ifdef SWAP_NYBBLES
  *p |= c << ((x1&1)<<2);
#else
  *p |= c << ((1-(x1&1))<<2);
#endif

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
      
      p = FB_MEM + y1 + (x1>>1);
      *p &= notmask4[x1&1];
      *p |= c << ((1-(x1&1))<<2);
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
       
      p = FB_MEM + y1 + (x1>>1);
      *p &= notmask4[x1&1];
#ifdef SWAP_NYBBLES
      *p |= c << ((x1&1)<<2);
#else
      *p |= c << ((1-(x1&1))<<2);
#endif
    }
  }
}

/* Basically draw this as repeated slabs: at the beginning and
 * end of each scanline we have to do partial bytes */
void linear4_rect(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,hwrcolor c,s16 lgop) {
   u8 *l,*p;
   s16 w2;
   
   if (!FB_ISNORMAL(dest,lgop)) {
     def_rect(dest,x,y,w,h,c,lgop);
     return;
   }

   p = l = PIXELBYTE(x,y);
   for (;h;h--,p=l+=FB_BPL) {
      w2 = w;
      if (x&1) {
#ifdef SWAP_NYBBLES
	 *p &= 0x0F;
	 *p |= c<<4;
#else
	 *p &= 0xF0;
	 *p |= c;
#endif
	 p++;
	 w2--;
      }
      __memset(p,c | (c<<4),w2>>1);
      if (w2&1) {
	 p  += (w2>>1);
#ifdef SWAP_NYBBLES
	 *p &= 0xF0;
	 *p |= c;
#else
	 *p &= 0x0F;
	 *p |= c << 4;
#endif
      }
   }
}

void linear4_charblit(hwrbitmap dest, u8 *chardat,s16 dest_x,s16 dest_y,s16 w,s16 h,
		  s16 lines, s16 angle, hwrcolor c, struct pgquad *clip,
		  s16 lgop, int char_pitch) {
  int iw,hc,x;
  int olines = lines;
  int bit;
  int flag=0, clipping = 0;
  int xpix,xmin,xmax;
  unsigned char ch;
  char *destline;
  int line_skip;

  if (!FB_ISNORMAL(dest,lgop) || angle) {
	  def_charblit(dest,chardat,dest_x,dest_y,w,h,lines,angle,c,clip,lgop,char_pitch);
	  return;
  }


  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || dest_y>clip->y2 || (dest_x+w)<clip->x1 || 
      (dest_y+h)<clip->y1)) return;


  xmin = 0;
  xmax = w;
  hc = 0;
  line_skip = FB_BPL - (char_pitch<<2); /* 4 bytes out for each byte in */

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y2<(dest_y+h))
      h = clip->y2-dest_y+1;
    if (clip->y1>dest_y) {
      hc = clip->y1-dest_y; /* Do it this way so skewing doesn't mess up when clipping */
      while (lines < hc && olines) {
	lines += olines;
	dest_x--;
      }
      dest_y += hc;
      chardat += hc*char_pitch;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->x1>dest_x) {
      xmin = clip->x1-dest_x;
      clipping = 1;
    }
    if (clip->x2<(dest_x+w)) {
      xmax = clip->x2-dest_x+1;
      clipping = 1;
    }
  }

  destline = PIXELBYTE(dest_x, dest_y);

  /* Decide which loop to use */
  if (olines || clipping) {
    /* Slower loop, taking skewing and clipping into account */
    
    for (;hc<h;hc++,destline += line_skip) {
      if (olines && lines==hc) {
	lines += olines;
	if ((--dest_x)&1)
	   dest--;
	flag=1;
      }
      for (iw=char_pitch,xpix=0;iw;iw--)
	for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,xpix++) {
	   if ( (xpix^dest_x)&1 ) {
	      if (ch&0x80 && xpix>=xmin && xpix<xmax) {
#ifdef SWAP_NYBBLES
		 *destline &= 0x0F;
		 *destline |= c<<4;
#else
		 *destline &= 0xF0;
		 *destline |= c;
#endif
	      }
	      destline++;
	   }
	   else {
	      if (ch&0x80 && xpix>=xmin && xpix<xmax) {
#ifdef SWAP_NYBBLES
		 *destline &= 0xF0;
		 *destline |= c;
#else
		 *destline &= 0x0F;
		 *destline |= c<<4;
#endif
	      }
	   }
	}
      if (flag) {
	xmax++;
	flag=0;
      }
    }
    return;
  }

  if(dest_x&1) {
	  for (;hc<h;hc++, destline += line_skip) {
		  for (iw=char_pitch;iw;iw--) {
			  ch = *(chardat++);
#ifdef SWAP_NYBBLES
			  if (ch&0x80) {*destline &= 0x0F; *destline |= c<<4; } destline++; 
			  if (ch&0x40) {*destline &= 0xF0; *destline |= c;    }
			  if (ch&0x20) {*destline &= 0x0F; *destline |= c<<4; } destline++; 
			  if (ch&0x10) {*destline &= 0xF0; *destline |= c;    }
			  if (ch&0x08) {*destline &= 0x0F; *destline |= c<<4; } destline++; 
			  if (ch&0x04) {*destline &= 0xF0; *destline |= c;    }
			  if (ch&0x02) {*destline &= 0x0F; *destline |= c<<4; } destline++; 
			  if (ch&0x01) {*destline &= 0xF0; *destline |= c;    }
#else
			  if (ch&0x80) {*destline &= 0xF0; *destline |= c;    } destline++; 
			  if (ch&0x40) {*destline &= 0x0F; *destline |= c<<4; }
			  if (ch&0x20) {*destline &= 0xF0; *destline |= c;    } destline++; 
			  if (ch&0x10) {*destline &= 0x0F; *destline |= c<<4; }
			  if (ch&0x08) {*destline &= 0xF0; *destline |= c;    } destline++; 
			  if (ch&0x04) {*destline &= 0x0F; *destline |= c<<4; }
			  if (ch&0x02) {*destline &= 0xF0; *destline |= c;    } destline++; 
			  if (ch&0x01) {*destline &= 0x0F; *destline |= c<<4; }
#endif
		  }
	  }
  } else {
	  for (;hc<h;hc++, destline += line_skip) {
		  for (iw=char_pitch;iw;iw--) {
			  ch = *(chardat++);
#ifdef SWAP_NYBBLES
			  if (ch&0x80) {*destline &= 0xF0; *destline |= c;    }
			  if (ch&0x40) {*destline &= 0x0F; *destline |= c<<4; } destline++; 
			  if (ch&0x20) {*destline &= 0xF0; *destline |= c;    }
			  if (ch&0x10) {*destline &= 0x0F; *destline |= c<<4; } destline++; 
			  if (ch&0x08) {*destline &= 0xF0; *destline |= c;    }
			  if (ch&0x04) {*destline &= 0x0F; *destline |= c<<4; } destline++; 
			  if (ch&0x02) {*destline &= 0xF0; *destline |= c;    }
			  if (ch&0x01) {*destline &= 0x0F; *destline |= c<<4; } destline++; 
#else
			  if (ch&0x80) {*destline &= 0x0F; *destline |= c<<4; }
			  if (ch&0x40) {*destline &= 0xF0; *destline |= c;    } destline++; 
			  if (ch&0x20) {*destline &= 0x0F; *destline |= c<<4; }
			  if (ch&0x10) {*destline &= 0xF0; *destline |= c;    } destline++; 
			  if (ch&0x08) {*destline &= 0x0F; *destline |= c<<4; }
			  if (ch&0x04) {*destline &= 0xF0; *destline |= c;    } destline++; 
			  if (ch&0x02) {*destline &= 0x0F; *destline |= c<<4; }
			  if (ch&0x01) {*destline &= 0xF0; *destline |= c;    } destline++; 
#endif
		  }
	  }
  }
}

/*
 * This is a relatively complicated 4bpp packed-pixel blit that does
 * handle LGOPs but currently doesn't handle tiling by itself.
 * Note that it can read (but not modify) one byte past the boundary of the
 * bitmap, but this is alright.
 */
void linear4_blit(hwrbitmap dest,
		  s16 dst_x, s16 dst_y,s16 w, s16 h,
		  hwrbitmap sbit,s16 src_x,s16 src_y,
		  s16 lgop) {
   u8 *src, *srcline, *dst, *dstline, mask;
   struct stdbitmap *srcbit = (struct stdbitmap *) sbit;
   int bw,xb,s,rs,tp,lp;
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
   src = srcline = srcbit->bits + (src_x>>1) + src_y*srcbit->pitch;
   dst = dstline = PIXELBYTE(dst_x,dst_y);
   xb = dst_x & 1;
   s = (src_x & 1) - xb;
   if (s<0) {
      s += 2;
      src=--srcline;
   }
   s <<= 2;
   rs = 8-s;

   /* The blitter core is a macro so various LGOPs can be used */

#ifdef SWAP_NYBBLES

   /* Nybble-swapped blitter core */   
#define BLITCORE                                                          \
   /* Special case when it fits entirely within one byte */               \
   if ((xb+w)<=2) {                                                       \
      mask = slabmask4[xb] & ~slabmask4[xb+w];                            \
      for (;h;h--,src+=srcbit->pitch,dst+=FB_BPL)                         \
	BLITCOPY(((src[0] << s) | (src[1] >> rs)),mask);                  \
   }                                                                      \
   else {                                                                 \
      tp = (dst_x+w)&1;        /* Trailing pixels */                      \
      lp = (2-xb)&1;           /* Leading pixels */                       \
      bw = (w-tp-lp)>>1;       /* Width in whole bytes */                 \
                                                                          \
      /* Bit-banging blitter loop */                                      \
      for (;h;h--,src=srcline+=srcbit->pitch,dst=dstline+=FB_BPL) {       \
	 if (lp) {                                                        \
	    BLITCOPY(((src[0] >> s) | (src[1] << rs)),0xF0);              \
	    src++,dst++;                                                  \
	 }                                                                \
	 for (i=bw;i>0;i--,src++,dst++)                                   \
	   BLITMAINCOPY(((src[0] >> s) | (src[1] << rs)));                \
	 if (tp)                                                          \
	    BLITCOPY(((src[0] >> s) | (src[1] << rs)),0x0F);              \
      }                                                                   \
   }

#else

   /* Normal blitter core */   
#define BLITCORE                                                          \
   /* Special case when it fits entirely within one byte */               \
   if ((xb+w)<=2) {                                                       \
      mask = slabmask4[xb] & ~slabmask4[xb+w];                            \
      for (;h;h--,src+=srcbit->pitch,dst+=FB_BPL)                         \
	BLITCOPY(((src[0] << s) | (src[1] >> rs)),mask);                  \
   }                                                                      \
   else {                                                                 \
      tp = (dst_x+w)&1;        /* Trailing pixels */                      \
      lp = (2-xb)&1;           /* Leading pixels */                       \
      bw = (w-tp-lp)>>1;       /* Width in whole bytes */                 \
                                                                          \
      /* Bit-banging blitter loop */                                      \
      for (;h;h--,src=srcline+=srcbit->pitch,dst=dstline+=FB_BPL) {       \
	 if (lp) {                                                        \
	    BLITCOPY(((src[0] << s) | (src[1] >> rs)),0x0F);              \
	    src++,dst++;                                                  \
	 }                                                                \
	 for (i=bw;i>0;i--,src++,dst++)                                   \
	   BLITMAINCOPY(((src[0] << s) | (src[1] >> rs)));                \
	 if (tp)                                                          \
	    BLITCOPY(((src[0] << s) | (src[1] >> rs)),0xF0);              \
      }                                                                   \
   }
#endif

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
   
/************************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear4(struct vidlib *vid) {
  setvbl_default(vid);


  vid->pixel          = &linear4_pixel;
  vid->getpixel       = &linear4_getpixel;
  vid->slab           = &linear4_slab;
  vid->bar            = &linear4_bar;
  vid->line           = &linear4_line;
  vid->rect           = &linear4_rect;
  vid->blit           = &linear4_blit;
  vid->charblit	      = &linear4_charblit;
}

/* The End */

