/* $Id: svgafb.c,v 1.1 2000/12/31 22:13:03 micahjd Exp $
 *
 * svgafb.c - A driver for linear-framebuffer svga devices that uses the linear*
 *          VBLs instead of the default vbl and libvgagl.
 *          Should be faster and overall less crufty but only work with
 *          linear devices.
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

#ifdef DRIVER_SVGAFB

//#define DOUBLEBUFFER

/* This creates no actual output, and writes only to the back buffer.
 * For debugging only, and must be used with DOUBLEBUFFER on */
// #define VIRTUAL

#include <pgserver/video.h>
#include <pgserver/input.h>

#include <vga.h>

#define dist(a,b) (((a)>(b))?((a)-(b)):((b)-(a)))

/* The find-closest-mode helper function originally presented in svga.c */
int svgafb_closest_mode(int xres,int yres,int bpp) {
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

g_error svgafb_init(int xres,int yres,int bpp,unsigned long flags) {
   g_error e;
   int mode,i;
   
#ifndef VIRTUAL

   /* In a GUI environment, we don't want VC switches,
    plus they usually crash on my system anyway,
    and they use extra signals that might confuse 
    select. */
   vga_lockvc();
   
   vga_init();
   
   /* Find a good mode */
   mode = svgafb_closest_mode(xres,yres,bpp);
   
   /* Load a main input driver. Do this before setting
    * video mode, so that the mouse is initialized
    * correctly.
    */
   e = load_inlib(&svgainput_regfunc,&inlib_main);
   errorcheck;

   vga_setmode(mode);
   
   /* Set up a palette for RGB simulation */
   for (i=0;i<256;i++)
     vga_setpalette(i,
		    (i & 0xC0) >> 2,
		    i & 0x38,
		    (i & 0x07) << 3);
   
   /* Save the actual video mode (might be different than what
    was requested) */
   vid->xres = vga_getxdim();
   vid->yres = vga_getydim();
   vid->bpp  = 8;

#else
   
   vid->xres = xres;
   vid->yres = yres;
   vid->bpp  = 8;
   
#endif /* VIRTUAL */
   
   /* Set up the linear framebuffer */
   vid->fb_bpl = vid->xres;
#ifdef DOUBLEBUFFER
   e = g_malloc((void **)&vid->fb_mem,vid->yres * vid->fb_bpl);
   errorcheck;
#else
   vid->fb_mem = vga_getgraphmem();
#endif
   
   return sucess;
}

void svgafb_close(void) {
#ifdef DOUBLEBUFFER
   g_free(vid->fb_mem);
#endif
   
   unload_inlib(inlib_main);    /* Take out input driver */
   vga_setmode(0);
}

#if defined(DOUBLEBUFFER) && !defined(VIRTUAL)
void svgafb_update(int x,int y,int w,int h) {
   unsigned char *src,*dest;
   int offset,i;
   unsigned long fbstart;
   
   /* Blit calculations */
   fbstart = y * vid->fb_bpl + x;
   dest = fbstart + graph_mem;
   src  = fbstart + vid->fb_mem;
   offset = vid->fb_bpl - w;
   
   /* Might prevent tearing? */
   vga_waitretrace();
   
   /* Do most of the blit in 32 bits, pick up the crumbs in 8 bits */
   for (;h;h--,src+=offset,dest+=offset) {
      for (i=w>>2;i;i--,src+=4,dest+=4)
	*((unsigned long *)dest) = *((unsigned long *)src);
      for (i=w&3;i;i--,src++,dest++)
	*dest = *src;
   }
}
#endif

g_error svgafb_regfunc(struct vidlib *v) {
   setvbl_linear8(v);           /* Only 8bpp so far */
   v->init = &svgafb_init;
   v->close = &svgafb_close;
#if defined(DOUBLEBUFFER) && !defined(VIRTUAL)
   v->update = &svgafb_update;
#endif
   
   return sucess;
}

#endif /* DRIVER_SVGAFB */
/* The End */
