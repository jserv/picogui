/* $Id: hardware.c,v 1.9 2000/04/29 02:40:59 micahjd Exp $
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

/* Trig table used in hwr_gradient (sin*256 for theta from 0 to 90) */
unsigned char trigtab[] = {
  0x00,0x04,0x08,0x0D,0x11,0x16,0x1A,0x1F,0x23,0x28,
  0x2C,0x30,0x35,0x39,0x3D,0x42,0x46,0x4A,0x4F,0x53,
  0x57,0x5B,0x5F,0x64,0x68,0x6C,0x70,0x74,0x78,0x7C,
  0x80,0x83,0x87,0x8B,0x8F,0x92,0x96,0x9A,0x9D,0xA1,
  0xA4,0xA7,0xAB,0xAE,0xB1,0xB5,0xB8,0xBB,0xBE,0xC1,
  0xC4,0xC6,0xC9,0xCC,0xCF,0xD1,0xD4,0xD6,0xD9,0xDB,
  0xDD,0xDF,0xE2,0xE4,0xE6,0xE8,0xE9,0xEB,0xED,0xEE,
  0xF0,0xF2,0xF3,0xF4,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,
  0xFC,0xFC,0xFD,0xFE,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF
};

/* Implementation of this function in a driver is optional - 
   not implementing gradients will only mean that gradient-enhanced
   themes will not look as good.  If you choose not to implement it,
   just make hwr_gradient call hwr_rect and send it the average of the
   two colors.

   The angle is expressed in degrees.

   This implementation is based on the function:
     color = y*sin(angle) + x*sin(angle)
   with scaling added to keep color between the specified values.
   The main improvement (?) is that it has been munged to use
   fixed-point calculations, and only addition and shifting in the
   inner loop.

   Wow, algebra and trigonometry are useful for something!  ;-)
*/
void hwr_gradient(struct cliprect *clip,int x,int y,int w,int h,
		  devcolort c1,devcolort c2,int angle) {
  /* Lotsa vars! */
  long r_vs,g_vs,b_vs,r_sa,g_sa,b_sa,r_ca,g_ca,b_ca,r_ica,g_ica,b_ica;
  long r_vsc,g_vsc,b_vsc,r_vss,g_vss,b_vss,sc_d;
  long r_v1,g_v1,b_v1,r_v2,g_v2,b_v2;
  devbmpt p;
  int r,g,b,i,s,c;
  int line_offset;

  /* Clipping */
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

  /* Init the bitmap */
  SDL_LockSurface(screen);
  p = ((devbmpt) screen->pixels) + x + y*HWR_WIDTH;
  line_offset = HWR_WIDTH-w;

  /* Calculate denominator of the scale value */
  sc_d = h*((s<0) ? -((long)s) : ((long)s)) +
    w*((c<0) ? -((long)c) : ((long)c));

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
  r_ca = g_ca = b_ca = r_ica = g_ica = b_ica = 0;

  /* Calculate the sine and cosine scales */
  r_vsc = (r_vs*((long)c)) >> 8;
  r_vss = (r_vs*((long)s)) >> 8;
  g_vsc = (g_vs*((long)c)) >> 8;
  g_vss = (g_vs*((long)s)) >> 8;
  b_vsc = (b_vs*((long)c)) >> 8;
  b_vss = (b_vs*((long)s)) >> 8;

  /* Initial sine accumulator */
  if (s<=0) { 
    r_sa = r_v2<<8; 
    g_sa = g_v2<<8; 
    b_sa = b_v2<<8; 
  }
  else {
    r_sa = r_v1<<8;
    g_sa = g_v1<<8;
    b_sa = b_v1<<8;
  }

  /* If the scales are negative, start from the opposite side */
  if (r_vss<0) r_sa  = -r_vss*h;
  if (r_vsc<0) r_ica = -r_vsc*w; 
  if (g_vss<0) g_sa  = -g_vss*h;
  if (g_vsc<0) g_ica = -g_vsc*w; 
  if (b_vss<0) b_sa  = -b_vss*h;
  if (b_vsc<0) b_ica = -b_vsc*w; 

  /* Finally, the loop! */

  for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,p+=line_offset)
    for (r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
	 i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc) {
      r = (r_ca+r_sa) >> 8;
      g = (g_ca+g_sa) >> 8;
      b = (b_ca+b_sa) >> 8;
      *(p++) = mkcolor(r,g,b);
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










