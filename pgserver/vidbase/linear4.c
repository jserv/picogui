/* $Id: linear4.c,v 1.8 2001/02/23 04:44:47 micahjd Exp $
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
#define PIXELBYTE(x,y) (((x)>>1)+LINE(y))

/* Table of masks used to isolate one pixel within a byte */
unsigned const char notmask4[] = { 0x0F, 0xF0 };

/************************************************** Minimum functionality */

/* By default assume 16 grays */
hwrcolor linear4_color_pgtohwr(pgcolor c) {
   return (getred(c)+getgreen(c)+getblue(c))/51;
}
pgcolor linear4_color_hwrtopg(hwrcolor c) {
   /* If this was called more often we could use a lookup table,
    * but it's not even worth the space here. */
   unsigned char gray = c * 17;
   return mkcolor(gray,gray,gray);
}

/* Ugh. Evil but necessary... I suppose... */

void linear4_pixel(int x,int y,hwrcolor c) {
   char *p = PIXELBYTE(x,y);
   *p &= notmask4[x&1];
   *p |= c << ((1-(x&1))<<2);
}
hwrcolor linear4_getpixel(int x,int y) {
   return ((*PIXELBYTE(x,y)) >> ((1-(x&1))<<2)) & 0x0F;
}
   
/************************************************** Accelerated (?) primitives */

/* Simple horizontal and vertical straight lines */
void linear4_slab(int x,int y,int w,hwrcolor c) {
   /* Do the individual pixels at the end seperately if necessary,
    * and do most of it with memset */
   
   char *p = PIXELBYTE(x,y);

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
void linear4_bar(int x,int y,int h,hwrcolor c) {
   /* Compute the masks ahead of time and reuse! */
   
   char *p = PIXELBYTE(x,y);
   unsigned char mask = notmask4[x&1];
   if (!(x&1)) c <<= 4;
   
   for (;h;h--,p+=vid->fb_bpl) {
      *p &= mask;
      *p |= c;
   }
}

/* Raster-optimized version of Bresenham's line algorithm */
void linear4_line(int x1,int y1,int x2,int y2,hwrcolor c) {
  int stepx, stepy;
  int dx = x2-x1;
  int dy = y2-y1;
  int fraction;
  char *p;
   
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
    stepy = -vid->fb_bpl; 
  } else {
    dy = dy << 1;
    stepy = vid->fb_bpl;
  }

  y1 *= vid->fb_bpl;
  y2 *= vid->fb_bpl;

  p = vid->fb_mem + y1 + (x1>>1);
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
      
      p = vid->fb_mem + y1 + (x1>>1);
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
       
      p = vid->fb_mem + y1 + (x1>>1);
      *p &= notmask4[x1&1];
      *p |= c << ((1-(x1&1))<<2);
    }
  }
}

/* Basically draw this as repeated slabs: at the beginning and
 * end of each scanline we have to do partial bytes */
void linear4_rect(int x,int y,int w,int h,hwrcolor c) {
   char *l = PIXELBYTE(x,y);
   char *p = l;
   int w2;
   
   for (;h;h--,p=l+=vid->fb_bpl) {
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

void linear4_gradient(int x,int y,int w,int h,int angle,
		      pgcolor c1, pgcolor c2,int translucent) {
   /* FIXME! */
}

void linear4_dim(int x,int y,int w,int h) {
   /* FIXME! */
}

/* Screen-to-screen bottom-up blit */
void linear4_scrollblit(int src_x,int src_y,
		    int dest_x,int dest_y,int w,int h) {
   /* FIXME! */
}

/* Blit from 1bpp packed character to the screen,
 * leaving zeroed pixels transparent */
void linear4_charblit(unsigned char *chardat,int dest_x,
		      int dest_y,int w,int h,int lines,
		      hwrcolor c,struct cliprect *clip) {
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
      destline = (dest += hc * vid->fb_bpl);
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
    
    for (;hc<h;hc++,destline=(dest+=vid->fb_bpl)) {
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
       for (;hc<h;hc++,destline=(dest+=vid->fb_bpl))
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
       for (;hc<h;hc++,destline=(dest+=vid->fb_bpl))
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

/* Like charblit, but rotate 90 degrees anticlockwise whilst displaying */
void linear4_charblit_v(unsigned char *chardat,int dest_x,
		  int dest_y,int w,int h,int lines,
		  hwrcolor c,struct cliprect *clip) {
   /* FIXME! */
}

void linear4_tileblit(struct stdbitmap *srcbit,
		  int src_x,int src_y,int src_w,int src_h,
		  int dest_x,int dest_y,int dest_w,int dest_h) {
   /* FIXME! */
}
   
void linear4_blit(struct stdbitmap *srcbit,int src_x,int src_y,
		  int dest_x, int dest_y,
		  int w, int h, int lgop) {
   struct stdbitmap screen;
   unsigned char *dest,*destline,*src,*srcline;
   int i,bw = w>>1;
   unsigned char flag_l,flag_r;
   
   /* Screen-to-screen blit */
   if (!srcbit) {
      srcbit = &screen;
      screen.bits = vid->fb_mem;
      screen.w = vid->fb_bpl;
      screen.h = vid->yres;
   }
   
   src  = srcline  = srcbit->bits + (src_x>>1) + src_y*srcbit->pitch;
   dest = destline = PIXELBYTE(dest_x,dest_y);
   flag_l = dest_x&1;
   flag_r = (dest_x^w) & 1;
   
   for (;h;h--,src=srcline+=srcbit->pitch,dest=destline+=vid->fb_bpl) {
      /* Check for an extra nibble at the beginning, and shift
       * pixels while blitting */
      if (flag_l) {
	 switch (lgop) {
	    
	  case PG_LGOP_NONE:
	    *dest &= 0xF0;
	    *dest |= (*src) >> 4;
	    dest++;
	    for (i=bw;i;i--,dest++,src++)
	      *dest = ((*src) << 4) | ((*(src+1)) >> 4);
	    break;
	  case PG_LGOP_OR:
	    *dest |= (*src) >> 4;
	    dest++;
	    for (i=bw;i;i--,dest++,src++)
	      *dest |= ((*src) << 4) | ((*(src+1)) >> 4);
	    break;
	  case PG_LGOP_AND:
	    *dest &= ((*src) >> 4) | 0xF0;
	    dest++;
	    for (i=bw;i;i--,dest++,src++)
	      *dest &= ((*src) << 4) | ((*(src+1)) >> 4);
	    break;
	  case PG_LGOP_XOR:
	    *dest ^= (*src) >> 4;
	    dest++;
	    for (i=bw;i;i--,dest++,src++)
	      *dest ^= ((*src) << 4) | ((*(src+1)) >> 4);
	    break;
	  case PG_LGOP_INVERT:
	    *dest &= 0xF0;
	    *dest |= ((*src) >> 4) ^ 0x0F;
	    dest++;
	    for (i=bw;i;i--,dest++,src++)
	      *dest = (((*src) << 4) | ((*(src+1)) >> 4)) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_OR:
	    *dest |= ((*src) >> 4) ^ 0x0F;
	    dest++;
	    for (i=bw;i;i--,dest++,src++)
	      *dest |= (((*src) << 4) | ((*(src+1)) >> 4)) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_AND:
	    *dest &= (((*src) >> 4) | 0xF0) ^ 0x0F;
	    dest++;
	    for (i=bw;i;i--,dest++,src++)
	      *dest &= (((*src) << 4) | ((*(src+1)) >> 4)) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_XOR:
	    *dest ^= ((*src) >> 4) ^ 0x0F;
	    dest++;
	    for (i=bw;i;i--,dest++,src++)
	      *dest ^= (((*src) << 4) | ((*(src+1)) >> 4)) ^ 0xFF;
	    break;
	 }
      }
      else {
	 /* Normal byte copy */
	 switch (lgop) {
	    
	  case PG_LGOP_NONE:
	    __memcpy(dest,src,bw);
	    dest+=bw;
	    src+=bw;
	    break;
	  case PG_LGOP_OR:
	    for (i=bw;i;i--,dest++,src++)
	      *dest |= *src;
	    break;
	  case PG_LGOP_AND:
	    for (i=bw;i;i--,dest++,src++)
	      *dest &= *src;
	    break;
	  case PG_LGOP_XOR:
	    for (i=bw;i;i--,dest++,src++)
	      *dest ^= *src;
	    break;
	  case PG_LGOP_INVERT:
	    for (i=bw;i;i--,dest++,src++)
	      *dest = (*src) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_OR:
	    for (i=bw;i;i--,dest++,src++)
	      *dest |= (*src) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_AND:
	    for (i=bw;i;i--,dest++,src++)
	      *dest &= (*src) ^ 0xFF;
	    break;
	  case PG_LGOP_INVERT_XOR:
	    for (i=bw;i;i--,dest++,src++)
	      *dest ^= (*src) ^ 0xFF;
	    break;
	 }
      }
      if (flag_r) {
	 /* Extra nibble on the right */
	 switch (lgop) {
	    
	  case PG_LGOP_NONE:
	    *dest &= 0x0F;
	    *dest |= (*src) & 0xF0;
	    break;
	  case PG_LGOP_OR:
	    *dest |= (*src) & 0xF0;
	    break;
	  case PG_LGOP_AND:
	    *dest &= ((*src) & 0xF0) | 0x0F;
	    break;
	  case PG_LGOP_XOR:
	    *dest ^= (*src) & 0xF0;
	    break;
	  case PG_LGOP_INVERT:
	    *dest &= 0x0F;
	    *dest = ((*src) & 0xF0) ^ 0xF0;
	    break;
	  case PG_LGOP_INVERT_OR:
	    *dest |= ((*src) & 0xF0) ^ 0xF0;
	    break;
	  case PG_LGOP_INVERT_AND:
	    *dest &= (((*src) & 0xF0) ^ 0xF0) | 0x0F;
	    break;
	  case PG_LGOP_INVERT_XOR:
	    *dest ^= ((*src) & 0xF0) ^ 0xF0;
	    break;
	 }
      }
   }
}

/* Nice simple screen-to-bitmap blit */
void linear4_unblit(int src_x,int src_y,
		    struct stdbitmap *destbit,int dest_x,int dest_y,
		    int w,int h) {
   unsigned char *dest,*destline,*src,*srcline;
   int i,bw;
   unsigned char flag_l;
   
   bw = /*(w>>1) -1; */ destbit->pitch;  /* FIXME: for w!=destbit->w this doesn't work! */
   dest = destline = destbit->bits + (dest_x>>1) + dest_y*destbit->pitch;
   src  = srcline  = PIXELBYTE(src_x,src_y);
   flag_l = src_x&1;
   
   for (;h;h--,src=srcline+=vid->fb_bpl,dest=destline+=destbit->pitch) {
      if (flag_l) {
	 /* Shifted copy */
	 for (i=bw;i;i--,dest++,src++)
	   *dest = ((*src) << 4) | ((*(src+1)) >> 4);
      }
      else {
	 /* Normal byte copy */
	 __memcpy(dest,src,bw);
      }
   }
}
   
/************************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear4(struct vidlib *vid) {
  /* Start with the defaults */
  setvbl_default(vid);

  /* Minimum functionality */
  vid->pixel          = &linear4_pixel;
  vid->getpixel       = &linear4_getpixel;
  vid->color_pgtohwr  = &linear4_color_pgtohwr;
  vid->color_hwrtopg  = &linear4_color_hwrtopg;
   
  /* Accelerated functions */
  vid->slab           = &linear4_slab;
  vid->bar            = &linear4_bar;
  vid->line           = &linear4_line;
  vid->rect           = &linear4_rect;
//  vid->gradient       = &linear4_gradient;
//  vid->dim            = &linear4_dim;
//  vid->scrollblit     = &linear4_scrollblit;
  vid->charblit       = &linear4_charblit;
//  vid->charblit_v     = &linear4_charblit_v;
//  vid->tileblit       = &linear4_tileblit;
  vid->blit           = &linear4_blit;
  vid->unblit         = &linear4_unblit;
}

/* The End */

