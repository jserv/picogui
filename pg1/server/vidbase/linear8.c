/* $Id$
 *
 * Video Base Library:
 * linear8.c - For 8bpp linear framebuffers (2-3-3 RGB mapping)
 *
 * These framebuffer libraries are fun to write! (but maybe not as
 * fun for others to read? I have that thing about for loops...)
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
#include <pgserver/render.h>

/* Macros to easily access the destination bitmap */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)
#define FB_ISNORMAL(bmp,lgop) (lgop == PG_LGOP_NONE && ((struct stdbitmap*)bmp)->bpp == vid->bpp)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)    ((y)*FB_BPL+FB_MEM)
#define PIXEL(x,y) (*((x)+LINE(y)))

/* Simple horizontal and vertical straight lines */
void linear8_slab(hwrbitmap dest, s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
  if (!FB_ISNORMAL(dest,lgop))
    def_slab(dest,x,y,w,c,lgop);
  else
    __memset(LINE(y) + x,c,w);
}
void linear8_bar(hwrbitmap dest, s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
  unsigned char *p;
  if (!FB_ISNORMAL(dest,lgop)) {
     def_bar(dest,x,y,h,c,lgop);
     return;
  }
  p = LINE(y) + x;
  for (;h;h--,p+=FB_BPL) *p = c;
}

/* Raster-optimized version of Bresenham's line algorithm */
void linear8_line(hwrbitmap dest, s16 x1,s16 yy1,s16 x2,s16 yy2,hwrcolor c,
		  s16 lgop) {
  s16 stepx, stepy;
  s16 dx;
  s16 dy;
  s16 fraction;
  u32 y1 = yy1,y2 = yy2;   /* Convert y coordinates to 32-bits because
			    * they will be converted to framebuffer offsets */
  
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

  FB_MEM[x1+y1] = c;

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
      FB_MEM[x1+y1] = c;
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
	FB_MEM[x1+y1] = c;
    }
  }
}

/* Just a good 'ol rectangle */
void linear8_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,
		  hwrcolor c,s16 lgop) {
  unsigned char *p;
 
  if (!FB_ISNORMAL(dest,lgop)) {
     def_rect(dest,x,y,w,h,c,lgop);
     return;
  }
   
  p = LINE(y) + x;
  for (;h;h--,p+=FB_BPL)
     __memset(p,c,w);
}

/* Gradients look really cheesy in 8bpp anyway, so just
 * fake it with a rectangle. Unless of course we have dithering...
 */
#ifndef CONFIG_DITHER_GRADIENTS
void linear8_gradient(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,s16 angle,
		      pgcolor c1, pgcolor c2,s16 lgop) {
  hwrcolor c;

  /* Find a reasonable in-between color to use */
  c = VID(color_pgtohwr) (mkcolor((getred(c2)+getred(c1))>>1,
				    (getgreen(c2)+getgreen(c1))>>1,
				    (getblue(c2)+getblue(c1))>>1));

  linear8_rect(dest,x,y,w,h,c,lgop);
}
#endif

/* Like charblit, but rotate 90 degrees anticlockwise whilst displaying
 *
 * FIXME: There is still a subtle bug when clipping against the left side
 * of a box in which the text gets 'smudged' by one pixel, as if it was OR'ed
 * with the first non-visible column. I give up, for now...
 */
void linear8_charblit_v(hwrbitmap dest,u8 *chardat,s16 dest_x,
			s16 dest_y,s16 w,s16 h,s16 lines,
			hwrcolor c,struct pgquad *clip, int char_pitch) {
  u8 *dst,*destline;
  s16 iw;
  s16 hc;
  s16 olines = lines;
  s16 bit;
  s16 flag=0;
  s16 xpix,xmin,xmax,clipping;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || (dest_y-w)>clip->y2 || (dest_x+h)<clip->x1 || 
      dest_y<clip->y1)) return;

  xmin = 0;
  xmax = w;
  clipping = 0;      /* This is set if we are being clipped,
			otherwise we can use a tight, fast loop */

  destline = dst = LINE(dest_y) + dest_x;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->x2<(dest_x+h-1))
      h = clip->x2-dest_x+2;
    if (clip->x1>dest_x) {
      hc = clip->x1-dest_x; /* Do it this way so skewing doesn't mess up when clipping */
      destline = (dst += hc);
      chardat += hc*char_pitch;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->y1>dest_y-w+1) {
      xmax = 1+dest_y-clip->y1;
      clipping = 1;
    }
    if (clip->y2<(dest_y)) {
      xmin = dest_y-clip->y2;
      clipping = 1;
    }
  }

  /* Decide which loop to use */
  if (olines || clipping) {
    /* Slower loop, taking skewing and clipping into account */

    for (;hc<h;hc++,destline=(dst++)) {
      if (olines && lines==hc) {
	lines += olines;
	dst += FB_BPL;
	flag=1;
      }
      for (iw=char_pitch,xpix=0;iw;iw--)
	for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,
	     destline-=FB_BPL,xpix++)
	  if (ch&0x80 && xpix>=xmin && xpix<xmax) *destline = c; 
      if (flag) {
	xmax++;
	flag=0;
      }
    }
  }
  else {
    /* Tight, unrolled loop, works only for normal case */
    
    for (;hc<h;hc++,destline=(dst++))
       for (iw=char_pitch;iw;iw--) {
	  ch = *(chardat++);
	  if (ch&0x80) *destline = c; destline-=FB_BPL; 
	  if (ch&0x40) *destline = c; destline-=FB_BPL; 
	  if (ch&0x20) *destline = c; destline-=FB_BPL; 
	  if (ch&0x10) *destline = c; destline-=FB_BPL; 
	  if (ch&0x08) *destline = c; destline-=FB_BPL; 
	  if (ch&0x04) *destline = c; destline-=FB_BPL; 
	  if (ch&0x02) *destline = c; destline-=FB_BPL; 
	  if (ch&0x01) *destline = c; destline-=FB_BPL; 
       }
  }
}

/* Blit from 1bpp packed character to the screen, with optional rotation,
 * logical operations, and background color. Luckily, almost all text is
 * at 0 degrees, completely inside the clip rectangle, and drawn with a
 * transparent background and PG_LGOP_NONE
 */
void linear8_charblit(hwrbitmap dest, u8 *chardat,s16 dest_x,
		      s16 dest_y,s16 w,s16 h,s16 lines,s16 angle,
		      hwrcolor c,struct pgquad *clip,
		      s16 lgop, int char_pitch) {
  u8 *dst,*destline;
  s16 iw,hc,olines,bit,flag,xpix,xmin,xmax,clipping;
  u8 ch;

  if (!FB_ISNORMAL(dest,lgop) || (angle && (angle != 90))) {
    def_charblit(dest,chardat,dest_x,dest_y,w,h,lines,angle,c,clip,lgop,char_pitch);
    return;
  }
  
  if (angle==90) {
    linear8_charblit_v(dest,chardat,dest_x,dest_y,w,h,lines,c,clip,char_pitch);
    return;
  }
   
  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || dest_y>clip->y2 || (dest_x+w)<clip->x1 || 
      (dest_y+h)<clip->y1)) return;

  olines = lines;
  flag = 0;
   
  xmin = 0;
  xmax = w;
  clipping = 0;      /* This is set if we are being clipped,
			otherwise we can use a tight, fast loop */

  destline = dst = LINE(dest_y) + dest_x;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y2<(dest_y+h))
      h = clip->y2-dest_y+1;
    if (clip->y1>dest_y) {
      hc = clip->y1-dest_y; /* Do it this way so skewing doesn't mess up when clipping */
      destline = (dst += hc * FB_BPL);
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

  /* Decide which loop to use */
  if (olines || clipping) {
    /* Slower loop, taking skewing and clipping into account */
    
    for (;hc<h;hc++,destline=(dst+=FB_BPL)) {
      if (olines && lines==hc) {
	lines += olines;
	dst--;
	flag=1;
      }
      for (iw=char_pitch,xpix=0;iw;iw--)
	for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,destline++,xpix++)
	  if (ch&0x80 && xpix>=xmin && xpix<xmax) *destline = c; 
      if (flag) {
	xmax++;
	flag=0;
      }
    }
  }
  else {
    /* Tight, unrolled loop, works only for normal case */
    
    for (;hc<h;hc++,destline=(dst+=FB_BPL))
      for (iw=char_pitch;iw;iw--) {
	ch = *(chardat++);
	if (ch&0x80) *destline = c; destline++; 
	if (ch&0x40) *destline = c; destline++; 
	if (ch&0x20) *destline = c; destline++; 
	if (ch&0x10) *destline = c; destline++; 
	if (ch&0x08) *destline = c; destline++; 
	if (ch&0x04) *destline = c; destline++; 
	if (ch&0x02) *destline = c; destline++; 
	if (ch&0x01) *destline = c; destline++; 
      }
  }
}

/* Ugh. Evil but necessary... I suppose... */
void linear8_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
  if (!FB_ISNORMAL(dest,lgop)) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }
   PIXEL(x,y) = c;
}
hwrcolor linear8_getpixel(hwrbitmap dest, s16 x,s16 y) {
  if (!FB_ISNORMAL(dest,PG_LGOP_NONE))
    return def_getpixel(dest,x,y);

  return PIXEL(x,y);
}

/* Fun-fun-fun blit functions! */
void linear8_blit(hwrbitmap dest,
		  s16 dst_x, s16 dst_y,s16 w, s16 h,
		  hwrbitmap sbit,s16 src_x,s16 src_y,
		  s16 lgop) {
  unsigned char *dst;
  struct stdbitmap *srcbit = (struct stdbitmap *) sbit;
  s16 i,offset_dst;
  s16 offset_src;
  unsigned char *src;
  
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
     break;
   default:
     def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
  }
   
  dst = LINE(dst_y) + dst_x;
  offset_dst = FB_BPL - w;
  src = srcbit->bits + src_x + src_y*srcbit->pitch;
  offset_src = srcbit->pitch - w;

#define BLITLOOP(op)                                               \
    for (;h;h--,src+=offset_src,dst+=offset_dst) {                 \
      for (i=w;i;i--,src++,dst++)                                  \
	*dst op *src;                                              \
    }
  
  switch (lgop) {
  case PG_LGOP_NONE: 
    for (;h;h--,src+=srcbit->pitch,dst+=FB_BPL)
      __memcpy(dst,src,w);
    break;
  case PG_LGOP_OR:         BLITLOOP(|=);                   break;
  case PG_LGOP_AND:        BLITLOOP(&=);                   break;
  case PG_LGOP_XOR:        BLITLOOP(^=);                   break;
  }
}

/* Load our driver functions into a vidlib */
void setvbl_linear8(struct vidlib *vid) {
  /* Start with the defaults */
  setvbl_default(vid);
   
  vid->slab           = &linear8_slab;
  vid->bar            = &linear8_bar;
  vid->line           = &linear8_line;
  vid->rect           = &linear8_rect;
#ifndef CONFIG_DITHER_GRADIENTS
  vid->gradient       = &linear8_gradient;
#endif
  vid->charblit       = &linear8_charblit;
  vid->pixel          = &linear8_pixel;
  vid->getpixel       = &linear8_getpixel;
  vid->blit           = &linear8_blit;
}

/* The End */

