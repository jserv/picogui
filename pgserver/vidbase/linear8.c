/* $Id: linear8.c,v 1.2 2000/12/16 20:08:46 micahjd Exp $
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
  memset(vid->fb_mem,0,vid->fb_bpl*vid->yres);
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
  unsigned char *p = LINE(y) + x;
  for (;w;w--,p++) *p = c;
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
  int i,offset = vid->fb_bpl - w;
  unsigned char *p = LINE(y) + x;

  for (;h;h--,p+=offset)
    for (i=w;i;i--,p++)
      *p = c;
}

/* For details on this function's workings see defaultvbl.c
 * This is just a framebuffer-optimized version. 
 * A significant change is that values are converted to hwrcolors,
 * then only one channel is used for the arithmetic.
 *
 * could be a problem on 24 or 32 bit framebuffers, but for 8 and 16 bit
 * it really cuts down on the registers and additions needed.
 */
void linear8_gradient(int x,int y,int w,int h,int angle,
		      pgcolor c1, pgcolor c2,int translucent) {
  long vs,sa,ca,ica;
  long vsc,vss;
  long v1,v2;
  int i,s,c;
  unsigned char *p = LINE(y) + x;

  unsigned char o,d,e;  /* Temp vars for color add/subtract */
  int q;

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

  /* Decode colors (go ahead and squash to our RGB mapping.
   * Not quite kosher, but it saves a lot of per-pixel work) */
  v1 = linear8_color_pgtohwr(c1);
  v2 = linear8_color_pgtohwr(c2);

  /* Calculate the scale values and 
   * scaled sine and cosine */
  vs = ((v2<<16)-(v1<<16)) / 
    (h*((s<0) ? -((long)s) : ((long)s)) +
     w*((c<0) ? -((long)c) : ((long)c)));

  /* Zero the accumulators */
  sa = ca = ica = 0;

  /* Calculate the sine and cosine scales */
  vsc = (vs*((long)c)) >> 8;
  vss = (vs*((long)s)) >> 8;

  /* If the scales are negative, start from the opposite side 
   * (FIXME: This must be calculated for each component) */
  if (vss<0) sa  = -vss*h;
  if (vsc<0) ica = -vsc*w; 
  if (v2<v1) v1 = v2;

  /* Finally, the loop! */

  if (translucent==0) {

    /* Normal opaque gradient */
    for (;h;h--,sa+=vss,p+=vid->fb_bpl)
      for (ca=ica,i=w;i;i--,ca+=vsc,p++)
	*p = v1 + ((ca+sa) >> 8);

  }
  else if (translucent>0) {

    /* Addition */
    for (;h;h--,sa+=vss,p+=vid->fb_bpl)
      for (ca=ica,i=w;i;i--,ca+=vsc,p++) {
	e = v1 + ((ca+sa) >> 8);
	o = *p;    /* Load original pixel color */

	/* Add and truncate each component */
	d = (o & 0x07) + (e & 0x07);                   /* Blue */
	if (d & 0xF8) d = 0xFF;
	d |= (o & 0x38) + (e & 0x38);                  /* Green */
	if (d & 0xC0) d = (d & 0xFF) | 0xFF00;
	d |= (o & 0xC0) + (e & 0xC0);                  /* Red */
	
	/* Store it  */
	*p = d;
      }
    
  }
  else {
    
    /* Subtraction */
    for (;h;h--,sa+=vss,p+=vid->fb_bpl)
      for (ca=ica,i=w;i;i--,ca+=vsc,p++) {
	e = v1 + ((ca+sa) >> 8);
	o = *p;
	d = 0;

	/* Subtract each component, storing if it's nonzero */
	if ( (q = (o & 0xC0) - (e & 0xC0)) > 0x3F ) d  = q; 
	if ( (q = (o & 0x38) - (e & 0x38)) > 0x03 ) d |= q; 
	if ( (q = (o & 0x07) - (e & 0x07)) > 0 )    d |= q; 
	
	/* Store it */
	*p = d;  
      }
  }
}

/* Bit-shift dimming */
void linear8_dim(void) {
  int w = vid->clip_x2 - vid->clip_x1 + 1;
  int h = vid->clip_y2 - vid->clip_y1 + 1;
  unsigned char *p = LINE(vid->clip_y1) + vid->clip_x1;
  int i,offset = vid->fb_bpl - w;

  for (;h;h--,p += offset)
    for (i=w;i;i--,p++)
      *p &= (*p & 0xB6) >> 1;
}

/* Screen-to-screen bottom-up blit */
void linear8_scrollblit(int src_x,int src_y,
		    int dest_x,int dest_y,int w,int h) {
  unsigned char *src = LINE(src_y + h - 1) + src_x;
  unsigned char *dest = LINE(dest_y + h - 1) + dest_x;
  int offset = - vid->fb_bpl - w;
  int i;

  /* Do most of the blit in 32 bits, pick up the crumbs in 8 bits */
  for (;h;h--,src+=offset,dest+=offset) {
    for (i=(w&0xFFFC)>>2;i;i--,src+=4,dest+=4)
      *((unsigned long *)dest) = *((unsigned long *)src);
    for (i=w&3;i;i--,src++,dest++)
      *dest = *src;
  }
}

/* Blit from 1bpp packed character to the screen,
 * leaving zeroed pixels transparent */
void linear8_charblit(unsigned char *chardat,int dest_x,
		  int dest_y,int w,int h,int lines,
		  hwrcolor c) {
  int bw = w;
  int iw,i;
  int olines = lines;
  unsigned char ch;
  unsigned char *dest = LINE(dest_y) + dest_x;
  int offset;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  bw &= 0x1F;

  /* Calculate line offset */
  offset =  vid->fb_bpl - (bw << 3);

  for (i=0;h;h--,i++,dest+=offset) {
    /* Skewing */
    if (olines && lines==i) {
      lines += olines;
      dest--;
    }
    for (iw=bw;iw;iw--) {
      ch = *(chardat++);
      
      if (ch&0x80) *dest = c; dest++;
      if (ch&0x40) *dest = c; dest++;
      if (ch&0x20) *dest = c; dest++;
      if (ch&0x10) *dest = c; dest++;
      if (ch&0x08) *dest = c; dest++;
      if (ch&0x04) *dest = c; dest++;
      if (ch&0x02) *dest = c; dest++;
      if (ch&0x01) *dest = c; dest++;
    }
  }
}   

/* Like charblit, but rotate 90 degrees anticlockwise whilst displaying */
void linear8_charblit_v(unsigned char *chardat,int dest_x,
		  int dest_y,int w,int h,int lines,
		  hwrcolor c) {
  int bw = w;
  int iw,i;
  int olines = lines;
  unsigned char ch;
  unsigned char *dest = LINE(dest_y) + dest_x;
  int offset;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  bw &= 0x1F;

  /* Calculate line offset */
  offset =  1 + vid->fb_bpl * (bw << 3);

  for (i=0;h;h--,i++,dest+=offset) {
    /* Skewing */
    if (olines && lines==i) {
      lines += olines;
      dest += vid->fb_bpl;
    }
    for (iw=bw;iw;iw--) {
      ch = *(chardat++);
      
      if (ch&0x80) *dest = c; dest -= vid->fb_bpl;
      if (ch&0x40) *dest = c; dest -= vid->fb_bpl;
      if (ch&0x20) *dest = c; dest -= vid->fb_bpl;
      if (ch&0x10) *dest = c; dest -= vid->fb_bpl;
      if (ch&0x08) *dest = c; dest -= vid->fb_bpl;
      if (ch&0x04) *dest = c; dest -= vid->fb_bpl;
      if (ch&0x02) *dest = c; dest -= vid->fb_bpl;
      if (ch&0x01) *dest = c; dest -= vid->fb_bpl;
    }
  }
}

void linear8_tileblit(struct stdbitmap *srcbit,
		  int src_x,int src_y,int src_w,int src_h,
		  int dest_x,int dest_y,int dest_w,int dest_h) {
  unsigned char *src_top = srcbit->bits + src_y*srcbit->w + src_x;
  unsigned char *dest = LINE(dest_y) + dest_x;
  int offset = vid->fb_bpl - dest_w;
  int dw;
  /* Restart values for the tile */
  unsigned char *src;
  unsigned char *src_line;
  int sh,sw;

  /* This horrifying loop scans across the destination rectangle one
   * line at at time, copying the source bitmap's scanline and letting
   * it wrap around as many times as needed. (all the while, doing as much
   * as possible 4 bytes at a time)
   * These scanlines are repeated for the height of the destination rectangle,
   * wrapping to the top of the source bitmap when necessary.
   */
  while (dest_h) {
    for (src_line=src_top,sh=src_h;sh && dest_h;
	 sh--,dest_h--,src_line+=src_w,dest+=offset) { 
      for (dw=dest_w;dw;) { 
	for (src=src_line,sw=(( (src_w < dw) ? src_w : dw ) & 
			       0xFFFC)>>2;sw;sw--,dw-=4,src+=4,dest+=4)
	  *((unsigned long *)dest) = *((unsigned long *)src);
	for (sw=(( (src_w < dw) ? src_w : dw ) & 3);sw;sw--,src++,dest++,dw--)
	  *dest = *src;
      }
    }
  }
}

/* Ugh. Evil but necessary... I suppose... */
void linear8_pixel(int x,int y,hwrcolor c) {
  PIXEL(x,y) = c;
}
hwrcolor linear8_getpixel(int x,int y) {
  return PIXEL(x,y);
}

/* Fun-fun-fun blit functions! */
void linear8_blit(hwrbitmap src,int src_x,int src_y,
		  int dest_x, int dest_y,
		  int w, int h, int lgop) {
}

/* Simpler than blit, thank goodness. */
void linear8_unblit(int src_x,int src_y,
		    hwrbitmap dest,int dest_x,int dest_y,
		    int w,int h) {
}

/* Load our driver functions into a vidlib */
void setvbl_linear8(struct vidlib *vid) {
  /* We take the default for these */
  vid->setmode        = &def_setmode;
  vid->close          = &emulate_dos;
  vid->clip_set       = &def_clip_set;
  vid->clip_off       = &def_clip_off;
  vid->update         = &emulate_dos;
  vid->frame          = &def_frame;
  vid->bitmap_loadxbm = &def_bitmap_loadxbm;
  vid->bitmap_loadpnm = &def_bitmap_loadpnm;
  vid->bitmap_new     = &def_bitmap_new;
  vid->bitmap_free    = &def_bitmap_free;
  vid->bitmap_getsize = &def_bitmap_getsize;
  vid->sprite_show    = &def_sprite_show;
  vid->sprite_hide    = &def_sprite_hide;
  vid->sprite_update  = &def_sprite_update;
  vid->sprite_showall = &def_sprite_showall;
  vid->sprite_hideall = &def_sprite_hideall;

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

