/* $Id: svga.c,v 1.10 2000/10/10 00:33:37 micahjd Exp $
 *
 * svga.c - video driver for (S)VGA cards, via vgagl and svgalib
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

#ifdef DRIVER_SVGA

#include <pgserver/video.h>
#include <pgserver/input.h>

#include <vga.h>
#include <vgagl.h>

//#define DOUBLEBUFFER

GraphicsContext *svga_virtual,*svga_physical;

/* Scanline buffer for LGOP blitting */
unsigned char *svga_buf;

/******************************************** Implementations */

g_error svga_init(int xres,int yres,int bpp,unsigned long flags) {
  g_error e;
  
  /* 100% hackish here.
     Trying to get something that just runs, then make it 
     PicoGUI compliant...
  */

#define VGA_MODE G640x480x16M

  /* In a GUI environment, we don't want VC switches,
     plus they usually crash on my system anyway,
     and they use extra signals that might confuse 
     select. */
  vga_lockvc();

  vga_init();

  /* Load a main input driver. Do this before setting
   * video mode, so that the mouse is initialized
   * correctly.
   **/
  e = load_inlib(&svgainput_regfunc,&inlib_main);
  errorcheck;

  vga_setmode(VGA_MODE);
  gl_setcontextvga(VGA_MODE);
  svga_physical = gl_allocatecontext();
  gl_getcontext(svga_physical);

#ifdef DOUBLEBUFFER
  gl_setcontextvgavirtual(VGA_MODE);
  svga_virtual = gl_allocatecontext();
  gl_getcontext(svga_virtual);
#endif

  gl_setrgbpalette();


  /* Save the actual video mode (might be different than what
     was requested) */
  vid->xres = WIDTH;
  vid->yres = HEIGHT;
  vid->bpp  = BITSPERPIXEL;
  
  /* Allocate scanline buffer */
  e = g_malloc((void**)&svga_buf,WIDTH*BYTESPERPIXEL);
  errorcheck;

  return sucess;
}

void svga_close(void) {
  /* Take out input driver */
  unload_inlib(inlib_main);

  g_free(svga_buf);
  gl_clearscreen(0);
  vga_setmode(0);
}

void svga_pixel(int x,int y,hwrcolor c) {
  gl_setpixel(x,y,c);
}

hwrcolor svga_getpixel(int x,int y) {
  return gl_getpixel(x,y);
}

void svga_update(void) {
#ifdef DOUBLEBUFFER
  gl_copyscreen(svga_physical);
#endif
}

void svga_clip_set(int x1,int y1,int x2,int y2) {
  gl_setclippingwindow(x1,y1,x2,y2);
  gl_enableclipping();
  vid->clip_x1 = x1;
  vid->clip_y1 = y1;
  vid->clip_x2 = x2;
  vid->clip_y2 = y2;
}

void svga_clip_off(void) {
  gl_disableclipping();
}

void svga_blit(struct stdbitmap *src,int src_x,int src_y,
		 struct stdbitmap *dest,int dest_x,int dest_y,
		 int w,int h,int lgop) {
  if (lgop==PG_LGOP_NULL) return;
  if (w<=0) return;
  if (h<=0) return;
  
  if (dest) return;   /*** FIX THIS ***/

  if (w>(src->w-src_x) || h>(src->h-src_y)) {
    int i,j;

    /* Do a tiled blit */
    for (i=0;i<w;i+=src->w)
      for (j=0;j<h;j+=src->h)
        svga_blit(src,0,0,dest,dest_x+i,dest_y+j,src->w,src->h,lgop);

    return;
  }
  
  if (lgop==PG_LGOP_NONE)
    if (src_x==0 && src_y==0)
      gl_putbox(dest_x,dest_y,src->w,src->h,src->bits);
    else
      gl_putboxpart(dest_x,dest_y,src->w,src->h,w,h,src->bits,src_x,src_y);
  {
    unsigned char *s,*b;
    int iw,lo,bytew;

    /* Copy the screen data into the buffer, apply the bitmap with the LGOP
       and paste it back. Not too much worse than the line-by-line way
       the sdl driver blits, but this can't assume the video is mappable
       into system memory. Let vgagl take care of the graphics card's oddities.
    */
    s = src->bits+src_x+src_y*WIDTH;
    bytew = w*BYTESPERPIXEL;
    switch (lgop) {
      
    case PG_LGOP_OR:
      lo = src->w-w;
      for (;h;h--,dest_y++,b+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) |= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_AND:
      lo = (src->w-w)*BYTESPERPIXEL;
      for (;h;h--,dest_y++,b+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) &= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_XOR:
      lo = (src->w-w)*BYTESPERPIXEL;
      for (;h;h--,dest_y++,b+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) ^= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_INVERT:
      lo = (src->w-w)*BYTESPERPIXEL;
      for (;h;h--,dest_y++,b+=lo) {
        for (iw=bytew;iw;iw--)
          *(b++) = *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_INVERT_OR:
      lo = (src->w-w)*BYTESPERPIXEL;
      for (;h;h--,dest_y++,b+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) |= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_INVERT_AND:
      lo = (src->w-w)*BYTESPERPIXEL;
      for (;h;h--,dest_y++,b+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) &= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_INVERT_XOR:
      lo = (src->w-w)*BYTESPERPIXEL;
      for (;h;h--,dest_y++,b+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) ^= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
    }
    
  }
}

void svga_rect(int x,int y,int w,int h,hwrcolor c) {
  gl_fillbox(x,y,w,h,c);
}

hwrcolor svga_color_pgtohwr(pgcolor c) {
  return gl_rgbcolor(getred(c),getgreen(c),getblue(c));
}

/******************************************** Driver registration */

/* This func. is passed to registervid */
g_error svga_regfunc(struct vidlib *v) {
  v->init = &svga_init;
  v->close = &svga_close;
  v->pixel = &svga_pixel;
  v->getpixel = &svga_getpixel;
  v->update = &svga_update;
  v->blit = &svga_blit;
  v->clip_set = &svga_clip_set;
  v->clip_off = &svga_clip_off;
  v->rect = &svga_rect;
  v->color_pgtohwr = &svga_color_pgtohwr;

  return sucess;
}

#endif /* DRIVER_SVGA */
/* The End */
