/* $Id: linear4.c,v 1.14 2001/05/29 20:33:35 micahjd Exp $
 *
 * Video Base Library:
 * linear4.c - For 4-bit grayscale framebuffers
 *
 * A little more complex than linear8, but this makes the
 * 68EZ328 look nice!
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
#include <pgserver/render.h>

/* Macros to easily access the members of vid->display */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((y)*FB_BPL+FB_MEM)
#define PIXELBYTE(x,y) (((x)>>1)+LINE(y))

/* Table of masks used to isolate one pixel within a byte */
unsigned const char notmask4[] = { 0x0F, 0xF0 };

/************************************************** Minimum functionality */

void linear4_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
   char *p;
   if (lgop != PG_LGOP_NONE) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }
   p = PIXELBYTE(x,y);
   *p &= notmask4[x&1];
   *p |= c << ((1-(x&1))<<2);
}
hwrcolor linear4_getpixel(hwrbitmap dest,s16 x,s16 y) {
   return ((*PIXELBYTE(x,y)) >> ((1-(x&1))<<2)) & 0x0F;
}
   
/************************************************** Accelerated (?) primitives */

/* Simple horizontal and vertical straight lines */
void linear4_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
   /* Do the individual pixels at the end seperately if necessary,
    * and do most of it with memset */
   
   char *p;

   if (lgop != PG_LGOP_NONE) {
      def_slab(dest,x,y,w,c,lgop);
      return;
   }
   
   p = PIXELBYTE(x,y);
   if (x&1) {
      *p &= 0xF0;
      *p |= c;
      p++;
      w--;
   }
   __memset(p,c | (c<<4),w>>1);
   if (w&1) {
      p+=(w>>1);
      *p &= 0x0F;
      *p |= c << 4;
   }
}
void linear4_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
   char *p;
   unsigned char mask;

   if (lgop != PG_LGOP_NONE) {
      def_bar(dest,x,y,h,c,lgop);
      return;
   }

   /* Compute the masks ahead of time and reuse! */
   p = PIXELBYTE(x,y);
   mask = notmask4[x&1];
   if (!(x&1)) c <<= 4;
   
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

  p = FB_MEM + y1 + (x1>>1);
  *p &= notmask4[x1&1];
  *p |= c << ((1-(x1&1))<<2);

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
      *p |= c << ((1-(x1&1))<<2);
    }
  }
}

/* Basically draw this as repeated slabs: at the beginning and
 * end of each scanline we have to do partial bytes */
void linear4_rect(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,hwrcolor c,s16 lgop) {
   u8 *l,*p;
   s16 w2;
   
   if (lgop != PG_LGOP_NONE) {
      def_rect(dest,x,y,w,h,c,lgop);
      return;
   }

   p = l = PIXELBYTE(x,y);
   for (;h;h--,p=l+=FB_BPL) {
      w2 = w;
      if (x&1) {
	 *p &= 0xF0;
	 *p |= c;
	 p++;
	 w2--;
      }
      __memset(p,c | (c<<4),w2>>1);
      if (w2&1) {
	 p  += (w2>>1);
	 *p &= 0x0F;
	 *p |= c << 4;
      }
   }
}

#if 0  /* FIXME! charblit is buggy! */
/* Blit from 1bpp packed character to the screen,
 * leaving zeroed pixels transparent */
void linear4_charblit(unsigned char *chardat,int dest_x,
		      int dest_y,int w,int h,int lines,
		      hwrcolor c,struct quad *clip) {
  char *dest,*destline;
  int bw = w;
  int iw;
  int hc;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax,clipping;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || dest_y>clip->y2 || (dest_x+w)<clip->x1 || 
      (dest_y+h)<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  clipping = 0;      /* This is set if we are being clipped,
			otherwise we can use a tight, fast loop */

  destline = dest = PIXELBYTE(dest_x,dest_y);
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y1>dest_y) {
      hc = clip->y1-dest_y; /* Do it this way so skewing doesn't mess up when clipping */
      destline = (dest += hc * FB_BPL);
      chardat += hc*bw;
    }
    if (clip->y2<(dest_y+h))
      h = clip->y2-dest_y+1;
    
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

  /* Decide which loop to use */
  if (olines || clipping) {
    /* Slower loop, taking skewing and clipping into account */
    
    for (;hc<h;hc++,destline=(dest+=FB_BPL)) {
      if (olines && lines==hc) {
	lines += olines;
	if ((--dest_x)&1)
	   dest--;
	flag=1;
      }
      for (iw=bw,xpix=0;iw;iw--)
	for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,xpix++) {
	   if ( (xpix^dest_x)&1 ) {
	      if (ch&0x80 && xpix>=xmin && xpix<xmax) {
		 *destline &= 0xF0;
		 *destline |= c;
	      }
	      destline++;
	   }
	   else {
	      if (ch&0x80 && xpix>=xmin && xpix<xmax) {
		 *destline &= 0x0F;
		 *destline |= c<<4;
	      }
	   }
	}
      if (flag) {
	xmax++;
	flag=0;
      }
    }
  }
  else {
    /* Tight, unrolled loop, works only for normal case.
     * Two different loops, depending on whether the destination is byte-aligned */
    
    if (dest_x&1) 
       for (;hc<h;hc++,destline=(dest+=FB_BPL))
	 for (iw=bw;iw;iw--) {
	    ch = *(chardat++);
	    if (ch&0x80) {*destline &= 0xF0; *destline |= c;    } destline++; 
	    if (ch&0x40) {*destline &= 0x0F; *destline |= c<<4; }
	    if (ch&0x20) {*destline &= 0xF0; *destline |= c;    } destline++; 
	    if (ch&0x10) {*destline &= 0x0F; *destline |= c<<4; }
	    if (ch&0x08) {*destline &= 0xF0; *destline |= c;    } destline++; 
	    if (ch&0x04) {*destline &= 0x0F; *destline |= c<<4; }
	    if (ch&0x02) {*destline &= 0xF0; *destline |= c;    } destline++; 
	    if (ch&0x01) {*destline &= 0x0F; *destline |= c<<4; }
	 }
     else
       for (;hc<h;hc++,destline=(dest+=FB_BPL))
	 for (iw=bw;iw;iw--) {
	    ch = *(chardat++);
	    if (ch&0x80) {*destline &= 0x0F; *destline |= c<<4; }
	    if (ch&0x40) {*destline &= 0xF0; *destline |= c;    } destline++; 
	    if (ch&0x20) {*destline &= 0x0F; *destline |= c<<4; }
	    if (ch&0x10) {*destline &= 0xF0; *destline |= c;    } destline++; 
	    if (ch&0x08) {*destline &= 0x0F; *destline |= c<<4; }
	    if (ch&0x04) {*destline &= 0xF0; *destline |= c;    } destline++; 
	    if (ch&0x02) {*destline &= 0x0F; *destline |= c<<4; }
	    if (ch&0x01) {*destline &= 0xF0; *destline |= c;    } destline++; 
	 }
  }
}
#endif

#if 0
/* Blitter is buggy! FIXME! */
void linear4_blit(hwrbitmap dest,
		  s16 dst_x, s16 dst_y, s16 w, s16 h,
		  hwrbitmap sbit,s16 src_x,s16 src_y,
		  s16 lgop) {
   unsigned char *dst,*dstline,*src,*srcline;
   int i,bw = w>>1;
   struct stdbitmap *srcbit = (struct stdbitmap *) sbit;
   unsigned char flag_l,flag_r;
   
   src  = srcline  = srcbit->bits + (src_x>>1) + src_y*srcbit->pitch;
   dst = dstline = PIXELBYTE(dst_x,dst_y);
   flag_l = (dst_x&1) ^ (src_x&1);
   flag_r = 0; //(dst_x^w) & 1;
   
   for (;h;h--,src=srcline+=srcbit->pitch,dst=dstline+=FB_BPL) {
      /* Check for an extra nibble at the beginning, and shift
       * pixels while blitting */
      if (flag_l) {
	 switch (lgop) {
	    
	  case PG_LGOP_NONE:
	    *dst &= 0xF0;
	    *dst |= (*src) >> 4;
	    dst++;
	    for (i=bw;i;i--,dst++,src++)
	      *dst = ((*src) << 4) | ((*(src+1)) >> 4);
	    break;
	  case PG_LGOP_OR:
	    *dst |= (*src) >> 4;
	    dst++;
	    for (i=bw;i;i--,dst++,src++)
	      *dst |= ((*src) << 4) | ((*(src+1)) >> 4);
	    break;
	  case PG_LGOP_AND:
	    *dst &= ((*src) >> 4) | 0xF0;
	    dst++;
	    for (i=bw;i;i--,dst++,src++)
	      *dst &= ((*src) << 4) | ((*(src+1)) >> 4);
	    break;
	  case PG_LGOP_XOR:
	    *dst ^= (*src) >> 4;
	    dst++;
	    for (i=bw;i;i--,dst++,src++)
	      *dst ^= ((*src) << 4) | ((*(src+1)) >> 4);
	    break;
	  case PG_LGOP_INVERT:
	    *dst &= 0xF0;
	    *dst |= ((*src) >> 4) ^ 0x0F;
	    dst++;
	    for (i=bw;i;i--,dst++,src++)
	      *dst = (((*src) << 4) | ((*(src+1)) >> 4)) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_OR:
	    *dst |= ((*src) >> 4) ^ 0x0F;
	    dst++;
	    for (i=bw;i;i--,dst++,src++)
	      *dst |= (((*src) << 4) | ((*(src+1)) >> 4)) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_AND:
	    *dst &= (((*src) >> 4) | 0xF0) ^ 0x0F;
	    dst++;
	    for (i=bw;i;i--,dst++,src++)
	      *dst &= (((*src) << 4) | ((*(src+1)) >> 4)) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_XOR:
	    *dst ^= ((*src) >> 4) ^ 0x0F;
	    dst++;
	    for (i=bw;i;i--,dst++,src++)
	      *dst ^= (((*src) << 4) | ((*(src+1)) >> 4)) ^ 0xFF;
	    break;
	 }
      }
      else {
	 /* Normal byte copy */
	 switch (lgop) {
	    
	  case PG_LGOP_NONE:
	    __memcpy(dst,src,bw);
	    dst+=bw;
	    src+=bw;
	    break;
	  case PG_LGOP_OR:
	    for (i=bw;i;i--,dst++,src++)
	      *dst |= *src;
	    break;
	  case PG_LGOP_AND:
	    for (i=bw;i;i--,dst++,src++)
	      *dst &= *src;
	    break;
	  case PG_LGOP_XOR:
	    for (i=bw;i;i--,dst++,src++)
	      *dst ^= *src;
	    break;
	  case PG_LGOP_INVERT:
	    for (i=bw;i;i--,dst++,src++)
	      *dst = (*src) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_OR:
	    for (i=bw;i;i--,dst++,src++)
	      *dst |= (*src) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_AND:
	    for (i=bw;i;i--,dst++,src++)
	      *dst &= (*src) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_XOR:
	    for (i=bw;i;i--,dst++,src++)
	      *dst ^= (*src) ^ 0xFF;
	    break;
	 }
      }
      if (flag_r) {
	 /* Extra nibble on the right */
	 switch (lgop) {
	    
	  case PG_LGOP_NONE:
	    *dst &= 0x0F;
	    *dst |= (*src) & 0xF0;
	    break;
	  case PG_LGOP_OR:
	    *dst |= (*src) & 0xF0;
	    break;
	  case PG_LGOP_AND:
	    *dst &= ((*src) & 0xF0) | 0x0F;
	    break;
	  case PG_LGOP_XOR:
	    *dst ^= (*src) & 0xF0;
	    break;
	  case PG_LGOP_INVERT:
	    *dst &= 0x0F;
	    *dst = ((*src) & 0xF0) ^ 0xF0;
	    break;
	  case PG_LGOP_INVERT_OR:
	    *dst |= ((*src) & 0xF0) ^ 0xF0;
	    break;
	  case PG_LGOP_INVERT_AND:
	    *dst &= (((*src) & 0xF0) ^ 0xF0) | 0x0F;
	    break;
	  case PG_LGOP_INVERT_XOR:
	    *dst ^= ((*src) & 0xF0) ^ 0xF0;
	    break;
	 }
      }
   }
}
#endif

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
//  vid->blit           = &linear4_blit;
}

/* The End */

