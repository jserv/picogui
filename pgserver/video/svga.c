/* $Id: svga.c,v 1.15 2000/10/29 09:06:39 micahjd Exp $
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

#define DOUBLEBUFFER

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

#define VGA_MODE G640x480x64K

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
#ifdef DOUBLEBUFFER
  add_updarea(x,y,1,1);
#endif
  gl_setpixel(x,y,c);
}

hwrcolor svga_getpixel(int x,int y) {
  return gl_getpixel(x,y);
}

void svga_update(void) {
#ifdef DOUBLEBUFFER
  gl_setcontext(svga_physical);
  gl_disableclipping();
  gl_copyboxfromcontext(svga_virtual,upd_x,upd_y,upd_w,upd_h,upd_x,upd_y);
  gl_setcontext(svga_virtual);
  upd_x = upd_y = upd_w = upd_h = 0;
#endif
}

void svga_clip_set(int x1,int y1,int x2,int y2) {
  gl_setclippingwindow(x1,y1,x2,y2);
  vid->clip_x1 = x1;
  vid->clip_y1 = y1;
  vid->clip_x2 = x2;
  vid->clip_y2 = y2;
}

void svga_blit(struct stdbitmap *src,int src_x,int src_y,
		 int dest_x,int dest_y,
		 int w,int h,int lgop) {

  if (lgop==PG_LGOP_NULL) return;
  if (w<=0) return;
  if (h<=0) return;
  
  if (!src) {
    /* Screen-to-screen copy */
    for (;h;h--,dest_y++,src_y++) {
      gl_getbox(src_x,src_y,w,1,svga_buf);
      gl_putbox(dest_x,dest_y,w,1,svga_buf);
    }
    return;
  }

  if (w>(src->w-src_x) || h>(src->h-src_y)) {
    int i,j;
    /* Do a tiled blit */
    for (i=0;i<w;i+=src->w)
      for (j=0;j<h;j+=src->h)
        svga_blit(src,0,0,dest_x+i,dest_y+j,src->w,src->h,lgop);
    return;
  }

  if ((dest_x+w-1)>vid->clip_x2) w = vid->clip_x2-dest_x+1;
  if ((dest_y+h-1)>vid->clip_y2) h = vid->clip_y2-dest_y+1;
  if (dest_x<vid->clip_x1) {
    w -= vid->clip_x1 - dest_x;
    src_x += vid->clip_x1 - dest_x;
    dest_x = vid->clip_x1;
  }
  if (dest_y<vid->clip_y1) {
    h -= vid->clip_y1 - dest_y;
    src_y += vid->clip_y1 - dest_y;
    dest_y = vid->clip_y1;
  }
  if (w<=0 || h<=0) return;

#ifdef DOUBLEBUFFER
  add_updarea(dest_x,dest_y,w,h);
#endif
  
  if (lgop==PG_LGOP_NONE) {
    gl_putboxpart(dest_x,dest_y,w,h,src->w,src->h,src->bits,src_x,src_y);
  }
  else {
    unsigned char *s,*b;
    int iw,lo,bytew;

    /* Copy the screen data into the buffer, apply the bitmap with the LGOP
       and paste it back. Not too much worse than the line-by-line way
       the sdl driver blits, but this can't assume the video is mappable
       into system memory. Let vgagl take care of the graphics card's oddities.
    */
    s = src->bits+src_x+src_y*WIDTH;
    bytew = w*BYTESPERPIXEL;
    lo = (src->w-w)*BYTESPERPIXEL;
    switch (lgop) {
      
    case PG_LGOP_OR:
      for (;h;h--,dest_y++,s+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) |= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_AND:
      for (;h;h--,dest_y++,s+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) &= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_XOR:
      for (;h;h--,dest_y++,s+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) ^= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_INVERT:
      for (;h;h--,dest_y++,s+=lo) {
        for (iw=bytew;iw;iw--)
          *(b++) = *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_INVERT_OR:
      for (;h;h--,dest_y++,s+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) |= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_INVERT_AND:
      for (;h;h--,dest_y++,s+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) &= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
      
    case PG_LGOP_INVERT_XOR:
      for (;h;h--,dest_y++,s+=lo) {
        gl_getbox(dest_x,dest_y,w,1,b=svga_buf);
        for (iw=bytew;iw;iw--)
          *(b++) ^= *(s++);
        gl_putbox(dest_x,dest_y,w,1,svga_buf);
      }
      break;
    }
    
  }
}

void svga_unblit(int src_x,int src_y,
		 struct stdbitmap *dest,int dest_x,int dest_y,
		 int w,int h) {

  if (w<=0) return;
  if (h<=0) return;
  
  if (dest_x==0 && dest_y==0)
    gl_getbox(src_x,src_y,dest->w,dest->h,dest->bits);
}

void svga_rect(int x,int y,int w,int h,hwrcolor c) {
#ifdef DOUBLEBUFFER
  add_updarea(x,y,w,h);
#endif
  gl_fillbox(x,y,w,h,c);
}

hwrcolor svga_color_pgtohwr(pgcolor c) {
  return gl_rgbcolor(getred(c),getgreen(c),getblue(c));
}

void svga_charblit(unsigned char *chardat,int dest_x,
		   int dest_y,int w,int h,int lines,
		   hwrcolor c) {
  int bw = w;
  int iw,bit,x,i;
  int olines = lines;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (dest_x>vid->clip_x2 || dest_y>vid->clip_y2 || 
      (dest_x+w)<vid->clip_x1 || (dest_y+h)<vid->clip_y1) return;

#ifdef DOUBLEBUFFER
  add_updarea(dest_x,dest_y,w,h);
#endif

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  bw &= 0x1F;

  for (i=0;i<h;i++,dest_y++) {
    /* Skewing */
    if (olines && lines==i) {
      lines += olines;
      dest_x--;
    }
    for (x=dest_x,iw=bw;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x++)
	/* FIXME: This still needs lots of optimization. I'm sure
	   we're involving at least a multiplication and some bitshifting
	   per-pixel that we don't need, not to mention the function
	   call...
	*/
	if (ch&0x80)   gl_setpixel(x,dest_y,c);
  }
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
  v->unblit = &svga_unblit;
  v->clip_set = &svga_clip_set;
  v->rect = &svga_rect;
  v->color_pgtohwr = &svga_color_pgtohwr;
  v->charblit = &svga_charblit;

  return sucess;
}

#endif /* DRIVER_SVGA */
/* The End */
