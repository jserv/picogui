/* $Id: svga.c,v 1.18 2000/12/16 18:37:47 micahjd Exp $
 *
 * svga.c - video driver for (S)VGA cards, via vgagl and svgalib
 * 
 * This driver supports 8,16,24, and 32 bit color at any resolution.
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

// #define DOUBLEBUFFER

GraphicsContext *svga_virtual,*svga_physical;

/* Scanline buffer for LGOP blitting */
unsigned char *svga_buf;

/******************************************** Implementations */

#define dist(a,b) (((a)>(b))?((a)-(b)):((b)-(a)))

int svga_closest_mode(int xres,int yres,int bpp) {
   int i,x,xbpp;
   int best = -1;
   unsigned int best_xresd = -1;
   unsigned int best_yresd = -1;
   int best_bpp = -1;
   vga_modeinfo *inf;
   
   for (i=1;i<vga_lastmodenumber();i++)
     if (vga_hasmode(i)) {
	inf = vga_getmodeinfo(i);
	xbpp = inf->bytesperpixel << 3;
	if (!xbpp) continue;   /* vgagl doesn't support these modes */

	/** Compare xres **/
	x = dist(inf->width,xres);
	if (x<best_xresd) goto newbestmode;
	if (x>best_xresd) continue;
	
	/** Compare yres **/
	x = dist(inf->height,yres);
	if (x<best_yresd) goto newbestmode;
	if (x>best_yresd) continue;
	
	/** Compare bpp **/
	if (xbpp<best_bpp) continue;
	if (xbpp>bpp) continue;
	
 newbestmode:
	best = i;
	best_bpp = xbpp;
	best_xresd = dist(inf->width,xres);
	best_yresd = dist(inf->height,yres);	
     }
   
   return best;
}

g_error svga_init(int xres,int yres,int bpp,unsigned long flags) {
  g_error e;
  int mode;
   
  /* In a GUI environment, we don't want VC switches,
     plus they usually crash on my system anyway,
     and they use extra signals that might confuse 
     select. */
  vga_lockvc();

  vga_init();

  /* Find a good mode */
  mode = svga_closest_mode(xres,yres,bpp);
   
  /* Load a main input driver. Do this before setting
   * video mode, so that the mouse is initialized
   * correctly.
   */
  e = load_inlib(&svgainput_regfunc,&inlib_main);
  errorcheck;

  vga_setmode(mode);
  gl_setcontextvga(mode);
  svga_physical = gl_allocatecontext();
  gl_getcontext(svga_physical);

#ifdef DOUBLEBUFFER
  gl_setcontextvgavirtual(mode);
  svga_virtual = gl_allocatecontext();
  gl_getcontext(svga_virtual);
#endif

  gl_setrgbpalette();
  gl_setwritemode(WRITEMODE_MASKED | FONT_COMPRESSED);
   
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
        svga_blit(src,src_x,src_y,dest_x+i,dest_y+j,src->w,src->h,lgop);
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
    s = src->bits+src_x+src_y*src->w;
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

  /* If we're skewing, use the slower write. Otherwise use vgagl's built-in
   * font things */
  if (lines) {
   
     /* Find the width of the source data in bytes */
     if (bw & 7) bw += 8;
     bw = bw >> 3;
     bw &= 0x1F;
     
     for (i=0;i<h;i++,dest_y++) {
	/* Skewing */
	if (lines==i) {
	   lines += olines;
	   dest_x--;
	}
	for (x=dest_x,iw=bw;iw;iw--)
	  for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x++)
	    if (ch&0x80)   gl_setpixel(x,dest_y,c);
     }
     
  }
  else {
     
     gl_setfont(w,h,chardat);
     if (!c) c = 1;            /* Eep! Vgagl chokes on a fg color of 0 */
     gl_setfontcolors(0,c);
     ch = 0;
     gl_writen(dest_x,dest_y,1,&ch);
     
  }  
}

/******************************************** Driver registration */

/* This func. is passed to registervid */
g_error svga_regfunc(struct vidlib *v) {
  setvbl_default(v);

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
