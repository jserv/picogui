/* $Id: svgafb.c,v 1.4 2001/01/13 09:49:59 micahjd Exp $
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

/* For debugging: force double-buffer on and don't actually touch the screen */
//#define VIRTUAL

#include <pgserver/inlstring.h>    /* Fast __memcpy inline function */
#include <pgserver/video.h>
#include <pgserver/input.h>

#include <vga.h>

/****************************************************** Definitions */

#define dist(a,b) (((a)>(b))?((a)-(b)):((b)-(a)))

/* Global flags for this driver */

#define SVGAFB_DOUBLEBUFFER   (1<<0)    /* Use doublebuffering */
#define SVGAFB_PAGEDBLITS     (1<<1)    /* Blit to nonlinear memory */

unsigned char svgafb_flags;
unsigned long svgafb_fbsize;

/* Function prototypes */
int svgafb_closest_mode(int xres,int yres,int bpp);
g_error svgafb_init(int xres,int yres,int bpp,unsigned long flags);
void svgafb_close(void);
void svgafb_update_linear(int x,int y,int w,int h);
void svgafb_update_paged(int x,int y,int w,int h);
g_error svgafb_regfunc(struct vidlib *v);

/****************************************************** Implementations */

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
   vga_modeinfo *mi;

#ifndef VIRTUAL
   
   svgafb_flags = flags & PG_VID_DOUBLEBUFFER ? SVGAFB_DOUBLEBUFFER : 0;
   
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
		    (i & 0xC0) * 63 / 0xC0,
		    (i & 0x38) * 63 / 0x38,
		    (i & 0x07) * 63 / 0x07);
   
   /* Save the actual video mode (might be different than what
    was requested) */
   vid->xres = vga_getxdim();
   vid->yres = vga_getydim();
   vid->bpp  = 8;

#else
   vid->xres = xres;
   vid->yres = yres;
   vid->bpp  = 8;
   svgafb_flags = SVGAFB_DOUBLEBUFFER;
#endif
 
   
   /* Calculate the framebuffer's size */
   vid->fb_bpl = vid->xres;
   svgafb_fbsize = vid->yres * vid->fb_bpl;
   
#ifndef VIRTUAL
   
   /* Figure out what type of framebuffer we have now.
    * If we need to and we can, set up linear addressing. Otherwise use
    * the paged double-buffer blits
    */
   mi = vga_getmodeinfo(mode);
   if ( (!(mi->flags & IS_LINEAR)) &&
        (svgafb_fbsize > 0xFFFF) &&
        ((!(mi->flags & CAPABLE_LINEAR)) || 
	(vga_setlinearaddressing() <= 0)) ) {
      /* Nope. Set up paged blits */
      
      svgafb_flags |= SVGAFB_PAGEDBLITS | SVGAFB_DOUBLEBUFFER;
      printf("svgafb: can't get linear addressing, forcing double buffer\n");
   }  

#endif
   
   /* Set up the linear framebuffer */

   if (svgafb_flags & SVGAFB_DOUBLEBUFFER) {
      /* Allocate the back buffer */
      e = g_malloc((void **)&vid->fb_mem,svgafb_fbsize);
      errorcheck;
   }
   else
     vid->fb_mem = vga_getgraphmem();
   
#ifndef VIRTUAL
   
   /* Use our flags to choose an update function. Keep the default if we
    * aren't double-buffering, otherwise choose paged or linear */
   
   if (svgafb_flags & SVGAFB_DOUBLEBUFFER) {
      if (svgafb_flags & SVGAFB_PAGEDBLITS)
	vid->update = &svgafb_update_paged;
      else
	vid->update = &svgafb_update_linear;
   }
   else
     vid->update = &def_update;
   
#endif
   
   return sucess;
}

void svgafb_close(void) {
   if (svgafb_flags & SVGAFB_DOUBLEBUFFER)
     g_free(vid->fb_mem);
   
   unload_inlib(inlib_main);    /* Take out input driver */
   vga_setmode(0);
}

void svgafb_update_linear(int x,int y,int w,int h) {
   unsigned char *src,*dest;
   unsigned long fbstart;
   
   /* Blit calculations */
   fbstart = y * vid->fb_bpl + x;
   dest = fbstart + graph_mem;
   src  = fbstart + vid->fb_mem;
   
   /* Might prevent tearing? */
   vga_waitretrace();
   
   for (;h;h--,src+=vid->fb_bpl,dest+=vid->fb_bpl)
     __memcpy(dest,src,w);
}

/* Like svgafb_update_linear, but set our 64K page using vga_setpage 
 * This must be able to handle a page change anywhere, including within
 * a line or even within a pixel.
 *
 * This code is adapted from the __svgalib_driver8p_putbox() function in
 * vgagl, probably written by Harm Hanemaayer (but if I am wrong please
 * correct me!)
 */
void svgafb_update_paged(int x,int y,int w,int h) {
   unsigned long vp;
   int page;
   char *bp;

   /* Might prevent tearing? */
   vga_waitretrace();

   vp   = x + y * vid->fb_bpl;
   bp   = vid->fb_mem + vp;
   page = vp >> 16;
   vp  &= 0xffff;
   vga_setpage(page);
   for (;h;h--) {
      if (vp + w > 0x10000) {
	 if (vp >= 0x10000) {
	    page++;
	    vga_setpage(page);
	    vp &= 0xffff;
	 } else {		/* page break within line */
	    __memcpy(graph_mem + vp, bp, 0x10000 - vp);
	    page++;
	    vga_setpage(page);
	    __memcpy(graph_mem, bp + 0x10000 - vp,
		     (vp + w) & 0xffff);
	    vp = (vp + vid->fb_bpl) & 0xffff;
	    bp += vid->fb_bpl;
	    continue;
	 }
      };
      __memcpy(graph_mem + vp, bp, w);
      bp += vid->fb_bpl;
      vp += vid->fb_bpl;
   }
}

/****************************************************** Registration */

g_error svgafb_regfunc(struct vidlib *v) {
   setvbl_linear8(v);           /* Only 8bpp so far */
   v->init = &svgafb_init;
   v->close = &svgafb_close;
   return sucess;
}

#endif /* DRIVER_SVGAFB */
/* The End */
