/* $Id: hardware.c,v 1.2 2000/06/04 20:30:52 micahjd Exp $
 *
 * hardware.c - Interface to svgalib/vgagl
 * Anything that makes any kind of assumptions about the display hardware
 * should be here or in mode.h.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <video.h>
#include <g_malloc.h>
#include <stdlib.h>

GraphicsContext *virtualscreen,*physicalscreen;

/* Scanline buffer for LGOP blitting */
devcolort slbuf[HWR_WIDTH*HWR_PIXELW];

/* vgagl has nice clipping functions.  This is a bit of glue to load up
   cliprect into vgagl's clipping system
*/
void inline vga_setclip(struct cliprect *c) {
  if (c) {
    gl_enableclipping();
    gl_setclippingwindow(c->x,c->y,c->x2,c->y2);
  } 
  else {
    gl_disableclipping();
  }
}

g_error hwr_init() {
  vga_init();
  vga_setmode(VGA_MODE);
  gl_setcontextvga(VGA_MODE);
  physicalscreen = gl_allocatecontext();
  gl_getcontext(physicalscreen);
  gl_setcontextvgavirtual(VGA_MODE);
  virtualscreen = gl_allocatecontext();
  gl_getcontext(virtualscreen);
  gl_setcontext(virtualscreen);
  gl_setrgbpalette();

  return sucess;
}

void hwr_release() {
  gl_clearscreen(0);
  vga_setmode(0);
}

void hwr_clear() {
  gl_clearscreen(0);
}

void hwr_update() {
  gl_copyscreen(physicalscreen);
}

void hwr_pixel(struct cliprect *clip,int x,int y,devcolort c) {
  vga_setclip(clip);
  gl_setpixel(x,y,c);
}

void hwr_slab(struct cliprect *clip,int x,int y,int l,devcolort c) {
  vga_setclip(clip);
  gl_hline(x,y,x+l,c);
}

void hwr_bar(struct cliprect *clip,int x,int y,int l,devcolort c) {
  vga_setclip(clip);
  gl_fillbox(x,y,1,l,c);
}

void hwr_line(struct cliprect *clip,int x1,int y1,int x2,int y2,devcolort c) {
  vga_setclip(clip);
  gl_line(x1,y1,x2,y2,c);
}

/* Ignore this for now.  Just give a basic rectangle */
void hwr_gradient(struct cliprect *clip,int x,int y,int w,int h,
		  devcolort c1,devcolort c2,int angle,int translucent) {
  vga_setclip(clip);
  gl_fillbox(x,y,w,h,c1);
}

void hwr_rect(struct cliprect *clip,int x,int y,int w,int h,devcolort c) {
  vga_setclip(clip);
  gl_fillbox(x,y,w,h,c);
}

/* Do it the fake stupid way - fill the area with a black checkerboard
   pattern.
*/
void hwr_dim(struct cliprect *clip) {
  int i,j;
  vga_setclip(NULL);
  for (j=clip->y;j<=clip->y2;j++)
    for (i=clip->x+(j&1);i<=clip->x2;i+=2)
      gl_setpixel(i,j,0);
}

/* Nothing special in gl for frames, do it the generic way */
void hwr_frame(struct cliprect *clip,int x,int y,int w,int h,devcolort c) {
  hwr_slab(clip,x,y,w,c);
  hwr_slab(clip,x,y+h-1,w,c);
  hwr_bar(clip,x,y+1,h-2,c);
  hwr_bar(clip,x+w-1,y+1,h-2,c);
}

/* This is still kinda broke.  Copies to bitmaps other than the screen
   don't work yet
*/
void hwr_blit(struct cliprect *clip, int lgop,
	      struct bitmap *src, int src_x, int src_y,
	      struct bitmap *dest, int dest_x, int dest_y,
	      int w, int h) {

  if (lgop==LGOP_NULL) return;
  if (w<=0) return;
  if (h<=0) return;
  
  if (dest) return;   /*** FIX THIS ***/

  vga_setclip(clip);

  if (w>(src->w-src_x) || h>(src->h-src_y)) {
    int i,j;

    /* Do a tiled blit */
    for (i=0;i<w;i+=src->w)
      for (j=0;j<h;j+=src->h)
	hwr_blit(clip,lgop,src,0,0,dest,dest_x+i,dest_y+j,src->w,src->h);

    return;
  }
  
  if (lgop==LGOP_NONE)
    if (src_x==0 && src_y==0)
      gl_putbox(dest_x,dest_y,src->w,src->h,src->bits);
    else
      gl_putboxpart(dest_x,dest_y,src->w,src->h,w,h,src->bits,src_x,src_y);
  {
    devbmpt s,b;
    int iw,lo;

    /* Copy the screen data into the buffer, apply the bitmap with the LGOP
       and paste it back. Not too much worse than the line-by-line way
       the sdl driver blits, but this can't assume the video is mappable
       into system memory. Let vgagl take care of the graphics card's oddities.
    */
    s = src->bits+src_x+src_y*HWR_WIDTH;
    switch (lgop) {
      
    case LGOP_OR:
      lo = src->w-w;
      for (;h;h--,dest_y++,b+=lo) {
	gl_getbox(dest_x,dest_y,w,1,b=slbuf);
	for (iw=w;iw;iw--)
	  *(b++) |= *(s++);
	gl_putbox(dest_x,dest_y,w,1,slbuf);
      }
      break;
      
    case LGOP_AND:
      lo = (src->w-w)*HWR_PIXELW;
      for (;h;h--,dest_y++,b+=lo) {
	gl_getbox(dest_x,dest_y,w,1,b=slbuf);
	for (iw=w;iw;iw--)
	  *(b++) &= *(s++);
	gl_putbox(dest_x,dest_y,w,1,slbuf);
      }
      break;
      
    case LGOP_XOR:
      lo = (src->w-w)*HWR_PIXELW;
      for (;h;h--,dest_y++,b+=lo) {
	gl_getbox(dest_x,dest_y,w,1,b=slbuf);
	for (iw=w;iw;iw--)
	  *(b++) ^= *(s++);
	gl_putbox(dest_x,dest_y,w,1,slbuf);
      }
      break;
      
    case LGOP_INVERT:
      lo = (src->w-w)*HWR_PIXELW;
      for (;h;h--,dest_y++,b+=lo) {
	for (iw=w;iw;iw--)
	  *(b++) = *(s++) ^ HWR_BPPMASK;
	gl_putbox(dest_x,dest_y,w,1,slbuf);
      }
      break;
      
    case LGOP_INVERT_OR:
      lo = (src->w-w)*HWR_PIXELW;
      for (;h;h--,dest_y++,b+=lo) {
	gl_getbox(dest_x,dest_y,w,1,b=slbuf);
	for (iw=w;iw;iw--)
	  *(b++) |= *(s++) ^ HWR_BPPMASK;
	gl_putbox(dest_x,dest_y,w,1,slbuf);
      }
      break;
      
    case LGOP_INVERT_AND:
      lo = (src->w-w)*HWR_PIXELW;
      for (;h;h--,dest_y++,b+=lo) {
	gl_getbox(dest_x,dest_y,w,1,b=slbuf);
	for (iw=w;iw;iw--)
	  *(b++) &= *(s++) ^ HWR_BPPMASK;
	gl_putbox(dest_x,dest_y,w,1,slbuf);
      }
      break;
      
    case LGOP_INVERT_XOR:
      lo = (src->w-w)*HWR_PIXELW;
      for (;h;h--,dest_y++,b+=lo) {
	gl_getbox(dest_x,dest_y,w,1,b=slbuf);
	for (iw=w;iw;iw--)
	  *(b++) ^= *(s++) ^ HWR_BPPMASK;
	gl_putbox(dest_x,dest_y,w,1,slbuf);
      }
      break;
    }
    
  }
}

void hwr_chrblit(struct cliprect *clip, unsigned char *chardat,int dest_x,
		 int dest_y, int w, int h,int lines,devcolort col) {
  int ix,iw,bw = w;
  unsigned char ch;

  vga_setclip(clip);
  
  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  bw &= 0x1F;

  for (;h;h--,dest_y++)
    for (ix=dest_x,iw=bw;iw;iw--) {
      ch = *(chardat++);
      if (ch&0x80) gl_setpixel(ix,dest_y,col); ix++;
      if (ch&0x40) gl_setpixel(ix,dest_y,col); ix++;
      if (ch&0x20) gl_setpixel(ix,dest_y,col); ix++;
      if (ch&0x10) gl_setpixel(ix,dest_y,col); ix++;
      if (ch&0x08) gl_setpixel(ix,dest_y,col); ix++;
      if (ch&0x04) gl_setpixel(ix,dest_y,col); ix++;
      if (ch&0x02) gl_setpixel(ix,dest_y,col); ix++;
      if (ch&0x01) gl_setpixel(ix,dest_y,col); ix++;
    }
}

g_error hwrbit_xbm(struct bitmap **bmp,
		   unsigned char *data, int w, int h,
		   devcolort fg, devcolort bg) {
  int i,bit;
  unsigned char c;
  devbmpt b;
  devbmpt p;
  g_error e;

  e = g_malloc((void **) &b,w*h*HWR_PIXELW);
  if (e.type != ERRT_NONE) return e;

  e = g_malloc((void **) bmp,sizeof(struct bitmap));
  if (e.type != ERRT_NONE) return e;

  p = b;

  (*bmp)->bits = b;
  (*bmp)->freebits = 1;
  (*bmp)->w = w;
  (*bmp)->h = h;

  for (;h>0;h--) {
    bit = 0;
    for (i=0;i<w;i++) {
      if (!bit)
	c = *(data++);
      *(p++) = (c&1) ? fg : bg;
      c = c>>1;
      bit++;
      if (bit==8) bit = 0;
    }
  }
}

void hwrbit_free(struct bitmap *b) {
  if (b->freebits) g_free(b->bits);
  g_free(b);
}

/* Little function to skip to the next value in an ASCII file */
void ascskip(unsigned char **dat,unsigned long *datlen) {
  while (*datlen) {
    if (**dat == ' ' || **dat == '\t' || **dat == '\n' || **dat == '\r') {
      (*dat)++;
      (*datlen)--;
    }
    else if (**dat == '#') {
      while ((*datlen) && (**dat != '\n') && (**dat != '\r')) {
	(*dat)++;
	(*datlen)--;
      }
    }
    else
      return;
  }
}

/* Read a number from ascii data */
int ascread(unsigned char **dat,unsigned long *datlen) {
  char buf[10];
  char *p = buf;
  int buflen=9;
  ascskip(dat,datlen);
  while ((**dat != ' ') && (**dat != '\t') && (**dat != '\n') &&
	 (**dat != '\r') && *datlen && buflen) {
    *(p++) = *((*dat)++);
    (*datlen)--;
    buflen--;
  }
  *p = 0;
  return atoi(buf);
}

/* Convert from any of the pbmplus formats in binary or ascii 
 */
g_error hwrbit_pnm(struct bitmap **bmp,
		   unsigned char *data,unsigned long datalen) {
  char format;
  int bin = 0;
  int bpp;
  int has_maxval=1;
  int w,h,max;
  int i,val,bit,r,g,b;
  devbmpt bits,p;
  g_error e;
  g_error efmt = mkerror(ERRT_BADPARAM,"pnm format error");

  ascskip(&data,&datalen);
  if (!datalen) return efmt;
  if (*(data++) != 'P') return efmt; datalen--;
  format = *(data++); datalen--;
  /* This decides whether the format is ascii or binary, and if it's
     PBM, PGM, or PPM. */
  switch (format) {
  case '4':
    bin=1;
  case '1':
    bpp=1;
    has_maxval=0;
    break;
  case '5':
    bin=1;
  case '2':
    bpp=8;
    break;
  case '6':
    bin=1;
  case '3':
    bpp=24;
    break;
  default:
    return efmt;
  }
  w = ascread(&data,&datalen);
  if (!datalen) return efmt;
  if (!w) return efmt;
  h = ascread(&data,&datalen);
  if (!datalen) return efmt;
  if (!h) return efmt;
  if (has_maxval) {
    max = ascread(&data,&datalen);
    if (!datalen) return efmt;
    if (!max) return efmt;
  }  

  /* One whitespace allowed before bitmap data */
  data++; datalen--;
  if (!datalen) return efmt;

  /* Set up the bitmap */
  e = g_malloc((void **) &bits,w*h*HWR_PIXELW);
  if (e.type != ERRT_NONE) return e;
  p = bits;
  e = g_malloc((void **) bmp,sizeof(struct bitmap));
  if (e.type != ERRT_NONE) return e;
  (*bmp)->bits = bits;
  (*bmp)->freebits = 1;
  (*bmp)->w = w;
  (*bmp)->h = h;

  /* For our color depth, read the right number of values and construct
     the bitmap */
  switch (bpp) {

  case 8:
    for (;h>0;h--)
      for (i=0;i<w;i++) {
	if (!datalen) {
	  hwrbit_free(*bmp);
	  return efmt;
	}
	if (bin) {
	  val = *(data++);
	  datalen--;
	}
	else {
	  val = ascread(&data,&datalen);
	}
	*(p++) = mkgray(val,max);
      }
    break;
    
  case 1:
    for (;h>0;h--) {
      bit = 0;
      for (i=0;i<w;i++) {
	if (!bit) {
	  if (!datalen) {
	    hwrbit_free(*bmp);
	    return efmt;
	  }
	  if (bin) {
	    val = *(data++);
	    datalen--;
	  }
	  else {
	    val = ascread(&data,&datalen);
	  }
	}
	*(p++) = (val&0x80) ? black : white;
	val = val<<1;
	bit++;
	if (bit==8) bit = 0;
      }
    }
    break;

  case 24:
    if (bin) {
      /* Binary with 255 maxval is most common, so optimize for it. */
      if (datalen < (w*h*3)) {
	hwrbit_free(*bmp);
	return efmt;
      }
      if (max==255) {
	for (;h>0;h--)
	  for (i=0;i<w;i++) {
	    r = *(data++);
	    g = *(data++);
	    b = *(data++);
	    datalen-=3;
	    *(p++) = mkcolor(r,g,b);
	  }
      }
      else {
	for (;h>0;h--)
	  for (i=0;i<w;i++) {
	    r = *(data++);
	    g = *(data++);
	    b = *(data++);
	    datalen-=3;
	    *(p++) = mkcolor(r*255/max,g*255/max,b*255/max);
	  }
      }
    }
    else {
      /* Well, ascii will be slow anyway but include for compatibility */
      for (;h>0;h--)
	for (i=0;i<w;i++) {
	  if (!datalen) {
	    hwrbit_free(*bmp);
	    return efmt;
	  }
	  if (bin) {
	    r = *(data++);
	    datalen--;
	  }
	  else {
	    r = ascread(&data,&datalen);
	  }
	  
	  if (!datalen) {
	    hwrbit_free(*bmp);
	    return efmt;
	  }
	  if (bin) {
	    g = *(data++);
	    datalen--;
	  }
	  else {
	    g = ascread(&data,&datalen);
	  }
	  
	  if (!datalen) {
	    hwrbit_free(*bmp);
	    return efmt;
	  }
	  if (bin) {
	    b = *(data++);
	    datalen--;
	  }
	  else {
	    b = ascread(&data,&datalen);
	  }
	  *(p++) = mkcolor(r*255/max,g*255/max,b*255/max);
	}
    }
    break;

  }
  return sucess;
}

devcolort cnvcolor(unsigned long int c) {
#if 0
  /* Grayscale? */
  if (c & 0x80000000) {
    return mkgray(c&0xFF,255);
  }
  else {
#if HWR_BPP == 32
    return c&0xFFFFFF;  /* Same RGB format as the hardware */
#else
    /* Otherwise we convert it */
    return mkcolor((c>>16)&0xFF,(c>>8)&0xFF,c&0xFF);
#endif
  }
#endif
}

/* The End */


