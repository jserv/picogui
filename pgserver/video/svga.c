/* $Id: svga.c,v 1.1 2000/08/27 08:31:01 micahjd Exp $
 *
 * svga.c - video driver for (S)VGA cards, via vgagl and svgalib
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
#include <vga.h>
#include <vgagl.h>

GraphicsContext *svga_virtual,*svga_physical;

/******************************************** Implementations */

g_error svga_init(int xres,int yres,int bpp,unsigned long flags) {
  unsigned long sdlflags = 0;
  char str[80];
  
  /* 100% hackish here.
     Trying to get something that just runs, then make it 
     PicoGUI compliant...
  */

#define VGA_MODE G320x200x256

  vga_init();
  vga_setmode(VGA_MODE);
  gl_setcontextvga(VGA_MODE);
  svga_physical = gl_allocatecontext();
  gl_getcontext(svga_physical);
  gl_setcontextvgavirtual(VGA_MODE);
  svga_virtual = gl_allocatecontext();
  gl_getcontext(svga_virtual);
  gl_setrgbpalette();


  /* Save the actual video mode (might be different than what
     was requested) */
  vid->xres = WIDTH;
  vid->yres = HEIGHT;
  vid->bpp  = BITSPERPIXEL;

  return sucess;
}

void svga_close(void) {
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
  gl_copyscreen(svga_physical);
}

void svga_clip_set(int x1,int y1,int x2,int y2) {
  gl_setclippingwindow(x1,y1,x2,y2);
  gl_enableclipping();
}

void svga_clip_off(void) {
  gl_disableclipping();
}

void svga_blit(struct stdbitmap *src,int src_x,int src_y,
		 struct stdbitmap *dest,int dest_x,int dest_y,
		 int w,int h,int lgop) {

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

  return sucess;
}

/* The End */
