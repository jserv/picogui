/* $Id: hardware.c,v 1.8 2000/04/27 03:27:36 micahjd Exp $
 *
 * hardware.c - SDL "hardware" layer
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

SDL_Surface *screen;
struct bitmap screenb = {NULL, HWR_WIDTH, HWR_HEIGHT};
struct cliprect screenclip = {0,0,HWR_WIDTH-1,HWR_HEIGHT-1};

g_error hwr_init() {
  int i;
#if SDL_BPP <= 8
  SDL_Color palette[256];
#endif  

  if (SDL_Init(SDL_INIT_VIDEO))
    return mkerror(ERRT_IO,SDL_GetError());

  /* Set the video mode */
  screen = SDL_SetVideoMode(HWR_WIDTH, HWR_HEIGHT, sizeof(devcolort)*8, 
			    SDLFLAG);
  if ( screen == NULL )
    return mkerror(ERRT_IO,SDL_GetError());

#if HWR_BPP == 4
  /* Set up a 16-gray palette, Out-of-range values set to red. */
  for ( i=0; i<16; ++i ) {
    palette[i].r = i<<4 | i;
    palette[i].g = i<<4 | i;
    palette[i].b = i<<4 | i;
  }
  for (; i<256; ++i ) {
    palette[i].r = 255;
    palette[i].g = 0;
    palette[i].b = 0;
  }
#endif /* HWR_BPP == 4 */

#if SDL_BPP <= 8
  SDL_SetColors(screen, palette, 0, 256);
#endif

  SDL_WM_SetCaption(TITLE,NULL);

  return sucess;
}

void hwr_release() {
  SDL_Quit();
}

void hwr_clear() {
  SDL_FillRect(screen,NULL,0);
}

void hwr_update() {
  SDL_UpdateRect(screen,0,0,0,0);  /* Real hardware will probably 
				      double-buffer */
}

void hwr_pixel(struct cliprect *clip,int x,int y,devcolort c) {
  if (clip) {
    if (x<clip->x || y<clip->y || x>clip->x2 || y>clip->y2) return;
  }

  SDL_LockSurface(screen);
  ((devbmpt)screen->pixels)[HWR_WIDTH*y+x] = c;
  SDL_UnlockSurface(screen);
}

void hwr_slab(struct cliprect *clip,int x,int y,int l,devcolort c) {
  devbmpt p;

  if (l<=0) return;
  if (clip) {
    if (y<clip->y || x>clip->x2 || y>clip->y2) return;
    if ((x+l-1)>clip->x2)
      l = clip->x2 - x + 1;
    if (x<clip->x) {
      l -= clip->x - x;
      x = clip->x;
    }
  }

  /*printf("slab(%d,%d,%d,0x%08X)\n\r",x,y,l,c);*/

  SDL_LockSurface(screen);
  p = ((devbmpt)screen->pixels)+x+y*HWR_WIDTH;
  for (;l;l--) *(p++) = c;
  SDL_UnlockSurface(screen);
}

void hwr_bar(struct cliprect *clip,int x,int y,int l,devcolort c) {
  devbmpt p;

  if (l<=0) return;
  if (clip) {
    if (x<clip->x || x>clip->x2 || y>clip->y2) return;
    if ((y+l-1)>clip->y2)
      l = clip->y2 - y + 1;
    if (y<clip->y) {
      l -= clip->y - y;
      y = clip->y;
    }
  }

  SDL_LockSurface(screen);
  p = ((devbmpt) screen->pixels)+x+y*HWR_WIDTH;
  for (;l>0;l--) {
    *p = c;
    p += HWR_WIDTH;
  }
  SDL_UnlockSurface(screen);
}

void hwr_line(struct cliprect *clip,int x1,int y1,int x2,int y2,devcolort c) {
  int stepx, stepy;
  int cx,cy,cstepy;
  int dx = x2-x1;
  int dy = y2-y1;
  int fraction;

  /* Implementation of Bresenham's algorithm */
  
  SDL_LockSurface(screen);

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
    stepy = -HWR_WIDTH; 
    cstepy = -1;
  } else {
    dy = dy << 1;
    stepy = HWR_WIDTH;
    cstepy = 1;
  }

  y1 *= HWR_WIDTH;
  y2 *= HWR_WIDTH;

  ((devbmpt)screen->pixels)[x1+y1] = c;

  /* Clipping? */
  if (clip && ((clip->x > x1) || (clip->y > y1) || 
	       (clip->x2 < x2) || (clip->y2 < y2))) {
    /* Yes! */
    cx = x1;   /* Keep track of our actual position in absolute */
    cy = y1;   /* coordinates for clipping purposes */

    /* Major axis is horizontal */
    if (dx > dy) {
      fraction = dy - (dx >> 1);
      while (x1 != x2) {
	if (fraction >= 0) {
	  y1 += stepy;
	  cy += cstepy;
	  fraction -= dx;
	}
	x1 += stepx;
	cx += stepx;
	fraction += dy;

	if ((clip->x <= cx) && (clip->y <= cy) && 
	    (clip->x2 >= cx) && (clip->y2 >= cy))
	  ((devbmpt)screen->pixels)[x1+y1] = c;
      }
    } 
    
    /* Major axis is vertical */
    else {
      fraction = dx - (dy >> 1);
      while (y1 != y2) {
	if (fraction >= 0) {
	  x1 += stepx;
	  cx += stepx;
	  fraction -= dy;
	}
	y1 += stepy;
	cy += cstepy;
	fraction += dx;
	
	if ((clip->x <= cx) && (clip->y <= cy) && 
	    (clip->x2 >= cx) && (clip->y2 >= cy))
	  ((devbmpt)screen->pixels)[x1+y1] = c;
      }
    }

  }
  else { /* Non-clipped version */

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

	((devbmpt)screen->pixels)[x1+y1] = c;
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
	
	((devbmpt)screen->pixels)[x1+y1] = c;
      }
    }
  }

  SDL_UnlockSurface(screen);
}

/* Implementation of this function in a driver is optional - 
   not implementing gradients will only mean that gradient-enhanced
   themes will not look as good.  If you choose not to implement it,
   just make hwr_gradient call hwr_rect and send it the average of the
   two colors.

   This is a vertical gradient, which is sheared by 'sy' pixels vertically
   every 'sx' horizontal pixels.
*/
void hwr_vgradient(struct cliprect *clip,int x,int y,int w,int h,
		  devcolort c1,devcolort c2,int sx,int sy) {
  devbmpt p,line;
  int i,isx,j;
  int r,g,b;    /* Current rgb values */
  int or,og,ob;    /* Original rgb values */
  int dr,dg,db; /* total rgb deltas */
  int lr,lg,lb; /* rgb at the beginning of a line */
  
  if (w<=0) return;
  if (h<=0) return;
  if (clip) {
    if ((x+w-1)>clip->x2) w = clip->x2-x+1;
    if ((y+h-1)>clip->y2) h = clip->y2-y+1;
    if (x<clip->x) {
      w -= clip->x - x;
      x = clip->x;
    }
    if (y<clip->y) {
      h -= clip->y - y;
      y = clip->y;
    }
    if (w<=0 || h<=0) return;
  }

  SDL_LockSurface(screen);
  p = ((devbmpt) screen->pixels)+x+y*HWR_WIDTH;

  /* This method of color interpolation (RGB) is portable, but
     pallete-based or grayscale drivers could perform much better if they
     worked directly with device-specific colors here instead of using
     the conversion macros.
  */
  or = r = getred(c1);
  og = g = getgreen(c1);
  ob = b = getblue(c1);
  dr = getred(c2) - r;
  dg = getgreen(c2) - g;
  db = getblue(c2) - b;

  for (line=p,j=h,lr=lg=lb=0;j;j--,line=(p+=HWR_WIDTH)) {
    for (i=0,isx=0;i<w;i++,isx++,line++) {
      /* Draw a pixel */
      *line = mkcolor(r,g,b);
    }

    /* Set the RGB values up for the next scan line. (Yuk, division!) */
    r = or+(lr += dr)/h;
    g = og+(lg += dg)/h;
    b = ob+(lb += db)/h;
  }

  SDL_UnlockSurface(screen);
}

void hwr_rect(struct cliprect *clip,int x,int y,int w,int h,devcolort c) {
  SDL_Rect r;

  if (w<=0) return;
  if (h<=0) return;
  if (clip) {
    if ((x+w-1)>clip->x2) w = clip->x2-x+1;
    if ((y+h-1)>clip->y2) h = clip->y2-y+1;
    if (x<clip->x) {
      w -= clip->x - x;
      x = clip->x;
    }
    if (y<clip->y) {
      h -= clip->y - y;
      y = clip->y;
    }
    if (w<=0 || h<=0) return;
  }

  r.x = x;
  r.y = y;
  r.w = w;
  r.h = h;
  SDL_FillRect(screen,&r,c);
}

void hwr_dim(struct cliprect *clip) {
  devbmpt p,pl;
  int i,j;
  SDL_LockSurface(screen);

  if (!clip) clip = &screenclip;

  p = ((devbmpt) screen->pixels)+clip->x+clip->y*HWR_WIDTH;
  for (j=clip->y;j<=clip->y2;j++) {
    pl = p;
    for (i=clip->x;i<=clip->x2;i++)
      *(pl++) = DIM_ALGO(*pl);
    p += HWR_WIDTH;
  }

  SDL_UnlockSurface(screen);
}

void hwr_frame(struct cliprect *clip,int x,int y,int w,int h,devcolort c) {
  /*  printf("frame(%d,%d,%d,%d,0x%08X)\n\r",x,y,w,h,c); */
  hwr_slab(clip,x,y,w,c);
  hwr_slab(clip,x,y+h-1,w,c);
  hwr_bar(clip,x,y+1,h-2,c);
  hwr_bar(clip,x+w-1,y+1,h-2,c);
}

/* Blits between two bitmaps, with optional clipping rectangles and lgop
 */
void hwr_blit(struct cliprect *clip, int lgop,
	      struct bitmap *src, int src_x, int src_y,
	      struct bitmap *dest, int dest_x, int dest_y,
	      int w, int h) {
  devbmpt s,d,sl,dl;
  int i,scrbuf=0;
  int s_of,d_of; /* Pixel offset between lines */

  if (lgop==LGOP_NULL) return;
  if (w<=0) return;
  if (h<=0) return;

  if (w>(src->w-src_x) || h>(src->h-src_y)) {
    int i,j;

    /* Do a tiled blit */
    for (i=0;i<w;i+=src->w)
      for (j=0;j<h;j+=src->h)
	hwr_blit(clip,lgop,src,0,0,dest,dest_x+i,dest_y+j,src->w,src->h);

    return;
  }
  
  if (!src) {
    src = &screenb;
    scrbuf = 1;
  }
  if (!dest) {
    dest = &screenb;
    scrbuf = 1;
  }

  if (clip) {
    if ((dest_x+w-1)>clip->x2) w = clip->x2-dest_x+1;
    if ((dest_y+h-1)>clip->y2) h = clip->y2-dest_y+1;
    if (dest_x<clip->x) {
      w -= clip->x - dest_x;
      src_x += clip->x - dest_x;
      dest_x = clip->x;
    }
    if (dest_y<clip->y) {
      h -= clip->y - dest_y;
      src_y += clip->y - dest_y;
      dest_y = clip->y;
    }
    if (w<=0 || h<=0) return;
  }

  if (scrbuf) {
    SDL_LockSurface(screen);
    screenb.bits = screen->pixels;
  }

  /* Now that the coords are clipped, set up pointers */
  s_of = src->w;
  d_of = dest->w;
  s = src->bits + src_x + src_y*s_of;
  d = dest->bits + dest_x + dest_y*d_of;

  /* Now the actual blitter code depends on the LGOP */
  switch (lgop) {

  case LGOP_NONE:
    for (;h;h--,s+=s_of,d+=d_of)
      memcpy(d,s,w*HWR_PIXELW);
    break;

  case LGOP_OR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
	*d |= *s;
    break;

  case LGOP_AND:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
	*d &= *s;
    break;

  case LGOP_XOR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
	*d ^= *s;
    break;

  case LGOP_INVERT:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
	*d = (*s) ^ HWR_BPPMASK;
    break;

  case LGOP_INVERT_OR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
	*d |= (*s) ^ HWR_BPPMASK;
    break;

  case LGOP_INVERT_AND:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
	*d &= (*s) ^ HWR_BPPMASK;
    break;

  case LGOP_INVERT_XOR:
    for (sl=s,dl=d;h;h--,s=(sl+=s_of),d=(dl+=d_of))
      for (i=w;i;i--,d++,s++)
	*d ^= (*s) ^ HWR_BPPMASK;
    break;

  }  

  if (scrbuf) SDL_UnlockSurface(screen);
}

/* A special blit function completely rewritten for font glyph
   blits.
   Much room for optimizational improvement, but remember that this
   is the 'portable' driver so no assembler!  :)
*/
void hwr_chrblit(struct cliprect *clip, unsigned char *chardat,int dest_x,
		 int dest_y, int w, int h,int lines,devcolort col) {
  devbmpt dest;
  devbmpt destline;
  int bw = w;
  int iw;
  int hc;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax,clipping;
  unsigned char ch;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  bw &= 0x1F;
  xmin = 0;
  xmax = w;
  clipping = 0;   /* This is set if we are being clipped,
		     otherwise we can use a tight, fast loop */

  SDL_LockSurface(screen);
  dest = screen->pixels;

  destline = dest =(devbmpt)screen->pixels+dest_x+dest_y*HWR_WIDTH;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y>dest_y) {
      hc = clip->y-dest_y; /* Do it this way so skewing doesn't mess up when
			     clipping */
      destline = (dest += hc * HWR_WIDTH);
    }
    if (clip->y2<(dest_y+h))
      h = clip->y2-dest_y+1;

    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->x>dest_x) {
      xmin = clip->x-dest_x;
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

    for (;hc<h;hc++,destline=(dest+=HWR_WIDTH)) {
      if (olines && lines==hc) {
	lines += olines;
	dest--;
	flag=1;
      }
      for (iw=bw,xpix=0;iw;iw--)
	for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,destline++,xpix++)
	  if (ch&0x80 && xpix>=xmin && xpix<xmax) *destline = col; 
      if (flag) {
	xmax++;
	flag=0;
      }
    }
  }
  else {
    /* Tight, unrolled loop, works only for normal case */

    for (;hc<h;hc++,destline=(dest+=HWR_WIDTH))
      for (iw=bw;iw;iw--) {
	ch = *(chardat++);
	if (ch&0x80) *destline = col; destline++; 
	if (ch&0x40) *destline = col; destline++; 
	if (ch&0x20) *destline = col; destline++; 
	if (ch&0x10) *destline = col; destline++; 
	if (ch&0x08) *destline = col; destline++; 
	if (ch&0x04) *destline = col; destline++; 
	if (ch&0x02) *destline = col; destline++; 
	if (ch&0x01) *destline = col; destline++; 
      }
  }

  SDL_UnlockSurface(screen);
}

/* The following functions allocate a device-specific bitmap
 * and convert the supplied data. They are named based on the
 * format they accept.
 */

/* This loads 1bpp XBM-formatted data */
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

  return sucess;
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
}

/* The End */










