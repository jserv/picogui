/* $Id: linear8.c,v 1.10 2001/02/08 04:56:03 micahjd Exp $
 *
 * Video Base Library:
 * linear8.c - For 8bpp linear framebuffers (2-3-3 RGB mapping)
 *
 * These framebuffer libraries are fun to write! (but maybe not as
 * fun for others to read? I have that thing about for loops...)
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/inlstring.h>    /* inline-assembly __memcpy */
#include <pgserver/video.h>

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)    ((y)*vid->fb_bpl+vid->fb_mem)
#define PIXEL(x,y) (*((x)+LINE(y)))

/* 2-3-3 RGB color quantization */
hwrcolor linear8_color_pgtohwr(pgcolor c) {
  return ((getred(c) & 0xC0) |
	  ((getgreen(c) >> 2) & 0x38) |
	  (getblue(c) >> 5));
}
pgcolor linear8_color_hwrtopg(hwrcolor c) {
  return mkcolor( (c&0xC0),
		  (c&0x38) << 2,
		  c << 5 );
}

/* Simple enough... */
void linear8_clear(void) {
  __memset(vid->fb_mem,0,vid->fb_bpl*vid->yres);
}

/* Add colors, truncating if we overflow */
void linear8_addpixel(int x,int y,pgcolor c) {
  unsigned char o = PIXEL(x,y); /* Load original pixel color */
  unsigned char d;              /* Working variable for building result */

  /* Add and truncate each component */
  d = (o & 0x07) + (getblue(c) >> 5);            /* Blue */
  if (d & 0xF8) d = 0xFF;
  d |= (o & 0x38) + ((getgreen(c) >> 2) & 0x38); /* Green */
  if (d & 0xC0) d = (d & 0xFF) | 0xFF00;
  d |= (o & 0xC0) + (getred(c) & 0xC0);          /* Red */

  /* Store it  */
  PIXEL(x,y) = d;
}

/* Subtract, truncating to 0 if a component is larger than the original */
void linear8_subpixel(int x,int y,pgcolor c) {
  unsigned char o = PIXEL(x,y); /* Load original pixel color */
  unsigned char d = 0;          /* Working variable for building result */
  int q;                        /* Store components */

  /* Subtract each component, storing if it's nonzero */
  if ( (q = (o & 0xC0) - (getred(c) & 0xC0)) > 0x3F )          d  = q; 
  if ( (q = (o & 0x38) - ((getgreen(c) >> 2) & 0x38)) > 0x03 ) d |= q; 
  if ( (q = (o & 0x07) - (getblue(c) >> 5)) > 0 )              d |= q; 

  /* Store it */
  PIXEL(x,y) = d;  
}

/* Simple horizontal and vertical straight lines */
void linear8_slab(int x,int y,int w,hwrcolor c) {
   __memset(LINE(y) + x,c,w);
}
void linear8_bar(int x,int y,int h,hwrcolor c) {
  unsigned char *p = LINE(y) + x;
  for (;h;h--,p+=vid->fb_bpl) *p = c;
}

/* Raster-optimized version of Bresenham's line algorithm */
void linear8_line(int x1,int y1,int x2,int y2,hwrcolor c) {
  int stepx, stepy;
  int dx = x2-x1;
  int dy = y2-y1;
  int fraction;
  
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

  vid->fb_mem[x1+y1] = c;

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
      vid->fb_mem[x1+y1] = c;
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
	vid->fb_mem[x1+y1] = c;
    }
  }
}

/* Just a good 'ol rectangle */
void linear8_rect(int x,int y,int w,int h,hwrcolor c) {
  unsigned char *p = LINE(y) + x;
  for (;h;h--,p+=vid->fb_bpl)
     __memset(p,c,w);
}

/* Gradients look really cheesy in 8bpp anyway, so just
 * fake it with a rectangle
 */
void linear8_gradient(int x,int y,int w,int h,int angle,
		      pgcolor c1, pgcolor c2,int translucent) {
  hwrcolor c;
  int i,offset = vid->fb_bpl - w;
  unsigned char *p = LINE(y) + x;
  unsigned char o,d;
  int q;

  /* Find a reasonable in-between color to use */
  c = linear8_color_pgtohwr(mkcolor((getred(c2)+getred(c1))>>1,
				    (getgreen(c2)+getgreen(c1))>>1,
				    (getblue(c2)+getblue(c1))>>1));

  if (translucent==0)           /* Normal opaque rectangle */
    linear8_rect(x,y,w,h,c);

  else if (translucent>0)       /* Additive translucency */
    for (;h;h--,p+=offset)
      for (i=w;i;i--,p++) {
	o = *p;    /* Load original pixel color */
	
	/* Add and truncate each component */
	d = (o & 0x07) + (c & 0x07);                   /* Blue */
	if (d & 0xF8) d = 0xFF;
	d |= (o & 0x38) + (c & 0x38);                  /* Green */
	if (d & 0xC0) d = (d & 0xFF) | 0xFF00;
	d |= (o & 0xC0) + (c & 0xC0);                  /* Red */
	
	/* Store it  */
	*p = d;
      }

  else                          /* Subtractive translucency */
    for (;h;h--,p+=offset)
      for (i=w;i;i--,p++) {
	o = *p;
	d = 0;

	/* Subtract each component, storing if it's nonzero */
	if ( (q = (o & 0xC0) - (c & 0xC0)) > 0x3F ) d  = q; 
	if ( (q = (o & 0x38) - (c & 0x38)) > 0x03 ) d |= q; 
	if ( (q = (o & 0x07) - (c & 0x07)) > 0 )    d |= q; 
	
	/* Store it */
	*p = d;  
      }
}

/* Bit-shift dimming */
void linear8_dim(int x,int y,int w,int h) {
  unsigned char *p = LINE(y) + x;
  int i,offset = vid->fb_bpl - w;

  for (;h;h--,p += offset)
    for (i=w;i;i--,p++)
      *p = (*p & 0xB6) >> 1;
}

/* Screen-to-screen bottom-up blit */
void linear8_scrollblit(int src_x,int src_y,
		    int dest_x,int dest_y,int w,int h) {
  unsigned char *src = LINE(src_y + h - 1) + src_x;
  unsigned char *dest = LINE(dest_y + h - 1) + dest_x;

  for (;h;h--,src-=vid->fb_bpl,dest-=vid->fb_bpl)
     __memcpy(dest,src,w);
}

/* Blit from 1bpp packed character to the screen,
 * leaving zeroed pixels transparent */
void linear8_charblit(unsigned char *chardat,int dest_x,
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

  destline = dest = LINE(dest_y) + dest_x;
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
	dest--;
	flag=1;
      }
      for (iw=bw,xpix=0;iw;iw--)
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
    
    for (;hc<h;hc++,destline=(dest+=vid->fb_bpl))
      for (iw=bw;iw;iw--) {
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

/* Like charblit, but rotate 90 degrees anticlockwise whilst displaying
 *
 * FIXME: There is still a subtle bug when clipping against the left side
 * of a box in which the text gets 'smudged' by one pixel, as if it was OR'ed
 * with the first non-visible column. I give up, for now...
 */
void linear8_charblit_v(unsigned char *chardat,int dest_x,
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
  if (clip && (dest_x>clip->x2 || (dest_y-w)>clip->y2 || (dest_x+h)<clip->x1 || 
      dest_y<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  clipping = 0;      /* This is set if we are being clipped,
			otherwise we can use a tight, fast loop */

  destline = dest = LINE(dest_y) + dest_x;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->x1>dest_x) {
      hc = clip->x1-dest_x; /* Do it this way so skewing doesn't mess up when clipping */
      destline = (dest += hc);
      chardat += hc*bw;
    }
    if (clip->x2<(dest_x+h-1))
      h = clip->x2-dest_x+2;
    
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

    for (;hc<h;hc++,destline=(dest++)) {
      if (olines && lines==hc) {
	lines += olines;
	dest += vid->fb_bpl;
	flag=1;
      }
      for (iw=bw,xpix=0;iw;iw--)
	for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,
	     destline-=vid->fb_bpl,xpix++)
	  if (ch&0x80 && xpix>=xmin && xpix<xmax) *destline = c; 
      if (flag) {
	xmax++;
	flag=0;
      }
    }
  }
  else {
    /* Tight, unrolled loop, works only for normal case */
    
    for (;hc<h;hc++,destline=(dest++))
      for (iw=bw;iw;iw--) {
	ch = *(chardat++);
	if (ch&0x80) *destline = c; destline-=vid->fb_bpl; 
	if (ch&0x40) *destline = c; destline-=vid->fb_bpl; 
	if (ch&0x20) *destline = c; destline-=vid->fb_bpl; 
	if (ch&0x10) *destline = c; destline-=vid->fb_bpl; 
	if (ch&0x08) *destline = c; destline-=vid->fb_bpl; 
	if (ch&0x04) *destline = c; destline-=vid->fb_bpl; 
	if (ch&0x02) *destline = c; destline-=vid->fb_bpl; 
	if (ch&0x01) *destline = c; destline-=vid->fb_bpl; 
      }
  }
}

void linear8_tileblit(struct stdbitmap *srcbit,
		  int src_x,int src_y,int src_w,int src_h,
		  int dest_x,int dest_y,int dest_w,int dest_h) {
  unsigned char *src_top = srcbit->bits + src_y*srcbit->w + src_x;
  unsigned char *dest = LINE(dest_y) + dest_x;
  int dw;
  /* Restart values for the tile */
  unsigned char *src_line;
  int sh;

  /* This horrifying loop scans across the destination rectangle one
   * line at at time, copying the source bitmap's scanline and letting
   * it wrap around as many times as needed.
   * These scanlines are repeated for the height of the destination rectangle,
   * wrapping to the top of the source bitmap when necessary.
   */
  while (dest_h)
     for (src_line=src_top,sh=src_h;sh && dest_h;
	  sh--,dest_h--,src_line+=srcbit->w,dest+=vid->fb_bpl)
       for (dw=dest_w;dw;)
	 __memcpy(dest,src_line, src_w < dw ? src_w : dw);
}

/* Ugh. Evil but necessary... I suppose... */
void linear8_pixel(int x,int y,hwrcolor c) {
  PIXEL(x,y) = c;
}
hwrcolor linear8_getpixel(int x,int y) {
  return PIXEL(x,y);
}

/* Fun-fun-fun blit functions! */
void linear8_blit(struct stdbitmap *srcbit,int src_x,int src_y,
		  int dest_x, int dest_y,
		  int w, int h, int lgop) {
  struct stdbitmap screen;
  unsigned char *dest;
  int i,offset_dest;
  
#ifdef DEBUG_VIDEO
   printf("linear8_blit(0x%08X,%d,%d,%d,%d,%d,%d,%d)\n",
	  srcbit,src_x,src_y,dest_x,dest_y,w,h,lgop);
#endif
   
  /* Screen-to-screen blit */
  if (!srcbit) {
    srcbit = &screen;
    screen.bits = vid->fb_mem;
    screen.w = vid->fb_bpl;
    screen.h = vid->yres;
  }

  /* Calculations needed by both normal and tiled blits */
  dest = LINE(dest_y) + dest_x;
  offset_dest = vid->fb_bpl - w;

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
#ifdef UCLINUX   /* The 32-bit stuff isn't aligned, crashes uclinux */
#define BLITLOOP(op,xtra32,xtra8)                                   \
    for (;h;h--,src+=offset_src,dest+=offset_dest) {                \
      for (i=w;i;i--,src++,dest++)                                  \
	*dest op *src xtra8;                                        \
    }
#else
#define BLITLOOP(op,xtra32,xtra8)                                   \
    for (;h;h--,src+=offset_src,dest+=offset_dest) {                \
      for (i=w>>2;i;i--,src+=4,dest+=4)                             \
	*((unsigned long *)dest) op *((unsigned long *)src) xtra32; \
      for (i=w&3;i;i--,src++,dest++)                                \
	*dest op *src xtra8;                                        \
    }
#endif
   
  /* Tiled blit loop - similar to tileblit() but always restarts the bitmap
   * on a tile boundary, instead of tiling a bitmap section */
#ifdef UCLINUX   /* The 32-bit stuff isn't aligned, crashes uclinux */
#define TILEBLITLOOP(op,xtra32,xtra8)                                     \
   while (h) {                                                            \
      for (;sh && h;sh--,h--,src_line+=srcbit->w,dest+=offset_dest) {     \
	 src = src_line + src_x;                                          \
	 swm = (swp < w) ? swp : w;                                       \
	 for (dw=w;dw;) {                                                 \
	    for (sw=swm;sw;sw--,src++,dest++,dw--)                        \
	      *dest op *src xtra8;                                        \
	    src = src_line;                                               \
	    swm = (srcbit->w < dw) ? srcbit->w : dw;                      \
	 }                                                                \
      }                                                                   \
      sh = srcbit->h;                                                     \
      src_line = srcbit->bits;                                            \
   }
#else
#define TILEBLITLOOP(op,xtra32,xtra8)                                     \
   while (h) {                                                            \
      for (;sh && h;sh--,h--,src_line+=srcbit->w,dest+=offset_dest) {     \
	 src = src_line + src_x;                                          \
	 swm = (swp < w) ? swp : w;                                       \
	 for (dw=w;dw;) {                                                 \
	    for (sw=swm>>2;sw;sw--,dw-=4,src+=4,dest+=4)                  \
	      *((unsigned long *)dest) op *((unsigned long *)src) xtra32; \
	    for (sw=swm&3;sw;sw--,src++,dest++,dw--)                      \
	      *dest op *src xtra8;                                        \
	    src = src_line;                                               \
	    swm = (srcbit->w < dw) ? srcbit->w : dw;                      \
	 }                                                                \
      }                                                                   \
      sh = srcbit->h;                                                     \
      src_line = srcbit->bits;                                            \
   }
#endif
   
  /* Is this a normal or tiled blit? */
  if (w>(srcbit->w-src_x) || h>(srcbit->h-src_y)) {   /* Tiled */
    unsigned char *src,*src_line;
    int dw,sh,swm,sw,swp;
     
    /* A few calculations for tiled blits */
    src_x %= srcbit->w;
    src_y %= srcbit->h;
    src_line = srcbit->bits + src_y * srcbit->w; 
    sh = srcbit->h - src_y;
    swp = srcbit->w - src_x;

    switch (lgop) {
    case PG_LGOP_NONE:  
       while (h) {
	  for (;sh && h;sh--,h--,src_line+=srcbit->w,dest+=vid->fb_bpl) {
	     src = src_line + src_x;
	     swm = (swp < w) ? swp : w;
	     for (dw=w;dw;) {
		__memcpy(dest,src,swm);
		src = src_line;
		swm = (srcbit->w < dw) ? srcbit->w : dw;
	     }
	  }
	  sh = srcbit->h;
	  src_line = srcbit->bits;
       }
       break;
    case PG_LGOP_OR:         TILEBLITLOOP(|=,,);                   break;
    case PG_LGOP_AND:        TILEBLITLOOP(&=,,);                   break;
    case PG_LGOP_XOR:        TILEBLITLOOP(^=,,);                   break;
    case PG_LGOP_INVERT:     TILEBLITLOOP(= ,^ 0xFFFFFFFF,^ 0xFF); break;
    case PG_LGOP_INVERT_OR:  TILEBLITLOOP(|=,^ 0xFFFFFFFF,^ 0xFF); break;
    case PG_LGOP_INVERT_AND: TILEBLITLOOP(&=,^ 0xFFFFFFFF,^ 0xFF); break;
    case PG_LGOP_INVERT_XOR: TILEBLITLOOP(^=,^ 0xFFFFFFFF,^ 0xFF); break;
    }
  }
  else {                                        /* Normal */
    int offset_src;
    unsigned char *src;

    /* Only needed for normal blits */
    src = srcbit->bits + src_x + src_y*srcbit->w;
    offset_src = srcbit->w - w;

    switch (lgop) {
    case PG_LGOP_NONE: 
       for (;h;h--,src+=srcbit->w,dest+=vid->fb_bpl)
	 __memcpy(dest,src,w);
       break;
    case PG_LGOP_OR:         BLITLOOP(|=,,);                   break;
    case PG_LGOP_AND:        BLITLOOP(&=,,);                   break;
    case PG_LGOP_XOR:        BLITLOOP(^=,,);                   break;
    case PG_LGOP_INVERT:     BLITLOOP(= ,^ 0xFFFFFFFF,^ 0xFF); break;
    case PG_LGOP_INVERT_OR:  BLITLOOP(|=,^ 0xFFFFFFFF,^ 0xFF); break;
    case PG_LGOP_INVERT_AND: BLITLOOP(&=,^ 0xFFFFFFFF,^ 0xFF); break;
    case PG_LGOP_INVERT_XOR: BLITLOOP(^=,^ 0xFFFFFFFF,^ 0xFF); break;
    }
  }
}

/* Nice simple screen-to-bitmap blit */
void linear8_unblit(int src_x,int src_y,
		    struct stdbitmap *destbit,int dest_x,int dest_y,
		    int w,int h) {
  unsigned char *src,*dest;

#ifdef DEBUG_VIDEO
   printf("linear8_unblit(%d,%d,0x%08X,%d,%d,%d,%d)\n",
	  src_x,src_y,destbit,dest_x,dest_y,w,h);
#endif
   
  /* A few calculations */
  src  = LINE(src_y) + src_x;
  dest = destbit->bits + dest_x + dest_y*destbit->w;

  for (;h;h--,src+=vid->fb_bpl,dest+=destbit->w)
     __memcpy(dest,src,w);
}

/* Load our driver functions into a vidlib */
void setvbl_linear8(struct vidlib *vid) {
  /* Start with the defaults */
  setvbl_default(vid);
   
  /* In this linear8 driver, replacing the default version */
  vid->color_pgtohwr  = &linear8_color_pgtohwr;
  vid->color_hwrtopg  = &linear8_color_hwrtopg;
  vid->addpixel       = &linear8_addpixel;
  vid->subpixel       = &linear8_subpixel;
  vid->clear          = &linear8_clear;
  vid->slab           = &linear8_slab;
  vid->bar            = &linear8_bar;
  vid->line           = &linear8_line;
  vid->rect           = &linear8_rect;
  vid->gradient       = &linear8_gradient;
  vid->dim            = &linear8_dim;
  vid->scrollblit     = &linear8_scrollblit;
  vid->charblit       = &linear8_charblit;
  vid->charblit_v     = &linear8_charblit_v;
  vid->tileblit       = &linear8_tileblit;
	
  /* In linear8, but no equivalent exists by default (Listed as "Required"
   * in the video.h index) */
  vid->pixel          = &linear8_pixel;
  vid->getpixel       = &linear8_getpixel;
  vid->blit           = &linear8_blit;
  vid->unblit         = &linear8_unblit;
}

/* The End */

