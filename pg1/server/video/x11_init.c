/* $Id$
 *
 * x11_init.c - Initialization for picogui'x driver for the X window system
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/configfile.h>
#include <pgserver/input.h>
#include <pgserver/x11.h>
#include <stdio.h>
#include <string.h>

/* Display and file descriptor for the X server */
Display *x11_display;
int x11_fd, x11_screen;

/* A table of graphics contexts for each LGOP mode */
GC x11_gctab[PG_LGOPMAX+1];

/* Stipple bitmap */
const u8 x11_stipple_bits[] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};

/* The window used by x11_window_debug */
hwrbitmap x11_debug_window;

/* A list of all allocated windows */
struct x11bitmap *x11_window_list;

/* A region specifying the entire display */
Region x11_display_region;

/* A region specifying the area we're currently rendering to */
Region x11_current_region;

/* We're using SHM if nonzero */
int x11_using_shm;

/* VBLs for each color depth, or NULL if not available */
#ifdef CONFIG_VBL_LINEAR16
struct vidlib x11_vbl_linear16;
#endif
#ifdef CONFIG_VBL_LINEAR32
struct vidlib x11_vbl_linear32;
#endif


/******************************************************** Error handler */

/* Handle fatal X errors.
 * Just shut down pgserver somewhat gracefully
 */
int x11_ioerrhandler(Display *d) {
  pgserver_shutdown();
  exit(1);
}

/* Handle X protocol errors.
 * These shouldn't normally happen of course, but
 * there's no reason they should be fatal.
 */
int x11_errhandler(Display *d, XErrorEvent *e) {
  char buffer[256];
  XGetErrorText(d, e->error_code, buffer, sizeof(buffer));
  printf("*** PicoGUI - X protocol error: %s\n",buffer);
}


/******************************************************** Initialization */

g_error x11_init(void) {
  int major,minor;
  Bool pixmaps;

  /* Connect to the default X server */
  x11_display = XOpenDisplay(NULL);
  if (!x11_display)
    return mkerror(PG_ERRT_IO,46);   /* Error initializing video */
  x11_screen = DefaultScreen(x11_display);

  XSetIOErrorHandler(&x11_ioerrhandler);
  XSetErrorHandler(&x11_errhandler);

  x11_fd = ConnectionNumber(x11_display);
  vid->display = NULL;
  x11_gc_setup(RootWindow(x11_display, x11_screen));

  /* X counts only the color itself in the depth, while
   * picogui counts the space allocated for the color.
   */  
  vid->bpp  = DefaultDepth(x11_display, x11_screen);
  if (vid->bpp <= 1)
    vid->bpp = 1;
  else if (vid->bpp <= 2)
    vid->bpp = 2;
  else if (vid->bpp <= 4)
    vid->bpp = 4;
  else if (vid->bpp <= 8)
    vid->bpp = 8;
  else if (vid->bpp <= 16)
    vid->bpp = 16;
  else if (vid->bpp <= 32)
    vid->bpp = 32;

  /* Are SHM pixmaps supported? */
  x11_using_shm = XShmQueryVersion(x11_display, &major, &minor, &pixmaps) && pixmaps;
  x11_using_shm = get_param_int("video-x11","shm",x11_using_shm);
  
  /* Load up all the linear VBLs for use on SHM bitmaps later */
#ifdef CONFIG_VBL_LINEAR16
  setvbl_linear16(&x11_vbl_linear16);
#endif
#ifdef CONFIG_VBL_LINEAR32
  setvbl_linear32(&x11_vbl_linear32);
#endif

  /* Load the matching input driver */
  return load_inlib(&x11input_regfunc,&inlib_main);
}

g_error x11_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  int x,y,rootw,rooth,border,depth;
  Window root;
  XRectangle rect;

  /* Get the display size */
  XGetGeometry(x11_display, RootWindow(x11_display, x11_screen), &root,
	       &x, &y, &rootw, &rooth, &border, &depth);

  if (flags & PG_VID_ROOTLESS) {
    vid->xres = rootw;
    vid->yres = rooth;
  }
  else {
    /* Default to 640x480 */
    if (!vid->xres) xres = 640;
    if (!vid->yres) yres = 480;
    if (xres) vid->xres = xres;
    if (yres) vid->yres = yres;
  }

  /* Create the display_region */
  x11_display_region = XCreateRegion();
  rect.x = rect.y = 0;
  rect.width = rootw;
  rect.height = rooth;
  XUnionRectWithRegion(&rect,x11_display_region,x11_display_region);
  x11_current_region = x11_display_region;

  x11_monolithic_window_update();
  return success;
}

void x11_close(void) {
  /* Delete the monolithic window and the debug window,
   * both the front and back buffers if they exist.
   */
  if (vid->display) {
    if (XB(vid->display)->frontbuffer)
      x11_bitmap_free((hwrbitmap) XB(vid->display)->frontbuffer);
    x11_bitmap_free(vid->display);
  }
  if (x11_debug_window) {
    if (XB(x11_debug_window)->frontbuffer)
      x11_bitmap_free((hwrbitmap) XB(x11_debug_window)->frontbuffer);
    x11_bitmap_free(x11_debug_window);
  }

  XDestroyRegion(x11_display_region);  

  unload_inlib(inlib_main);   /* Take out our input driver */
  XCloseDisplay(x11_display);
  x11_display = NULL;
}

void x11_gc_setup(Drawable d) {
  /* Set up our GCs for each supported LGOP */
  /* Set up graphics contexts for each LGOP that X can support directly 
   */
  x11_gctab[PG_LGOP_NONE]       = XCreateGC(x11_display,d,0,NULL);

  x11_gctab[PG_LGOP_OR]         = XCreateGC(x11_display,d,0,NULL);
  x11_gctab[PG_LGOP_AND]        = XCreateGC(x11_display,d,0,NULL);
  x11_gctab[PG_LGOP_XOR]        = XCreateGC(x11_display,d,0,NULL);
  x11_gctab[PG_LGOP_INVERT]     = XCreateGC(x11_display,d,0,NULL);
  x11_gctab[PG_LGOP_INVERT_OR]  = XCreateGC(x11_display,d,0,NULL);
  x11_gctab[PG_LGOP_INVERT_AND] = XCreateGC(x11_display,d,0,NULL);
  x11_gctab[PG_LGOP_INVERT_XOR] = XCreateGC(x11_display,d,0,NULL);
  x11_gctab[PG_LGOP_STIPPLE]    = XCreateGC(x11_display,d,0,NULL);

  XSetFunction(x11_display,x11_gctab[PG_LGOP_OR],        GXor);
  XSetFunction(x11_display,x11_gctab[PG_LGOP_AND],       GXand);
  XSetFunction(x11_display,x11_gctab[PG_LGOP_XOR],       GXxor);
  XSetFunction(x11_display,x11_gctab[PG_LGOP_INVERT],    GXcopyInverted);
  XSetFunction(x11_display,x11_gctab[PG_LGOP_INVERT_OR], GXorInverted);
  XSetFunction(x11_display,x11_gctab[PG_LGOP_INVERT_AND],GXandInverted);
  XSetFunction(x11_display,x11_gctab[PG_LGOP_INVERT_XOR],GXequiv);

  /* Set up a stipple bitmap for PG_LGOP_STIPPLE */
  XSetLineAttributes(x11_display,x11_gctab[PG_LGOP_STIPPLE],
		     1,LineOnOffDash,CapRound,JoinRound);
  XSetFillStyle(x11_display,x11_gctab[PG_LGOP_STIPPLE],FillStippled);
  XSetStipple(x11_display,x11_gctab[PG_LGOP_STIPPLE],
	      XCreateBitmapFromData(x11_display,d,x11_stipple_bits,
				    x11_stipple_width,x11_stipple_height));
}


/******************************************************** Miscellaneous */

void x11_message(u32 message, u32 param, u32 *ret) {
  switch (message) {

  case PGDM_SOUNDFX:
    /* XFree86 ignores the volume, it seems */
    if (get_param_int("video-x11","sound",0)) 
      XBell(x11_display,50);
    break;
  }
}

int x11_is_rootless(void) {
  return (vid->flags & PG_VID_ROOTLESS)!=0;
}


/******************************************************** Driver registration */

g_error x11_regfunc(struct vidlib *v) {
  setvbl_default(v);
  
  v->pixel                 = &x11_pixel;
  v->getpixel              = &x11_getpixel;
  v->rect                  = &x11_rect;
  v->line                  = &x11_line;
  v->slab                  = &x11_slab;
  v->bar                   = &x11_bar;
  v->ellipse               = &x11_ellipse;
  v->fellipse              = &x11_fellipse;
  v->update                = &x11_update;
  v->bitmap_get_groprender = &x11_bitmap_get_groprender;
  v->bitmap_getsize        = &x11_bitmap_getsize;
  v->bitmap_new            = &x11_bitmap_new;
  v->bitmap_free           = &x11_bitmap_free;
  v->blit                  = &x11_blit;
  v->message               = &x11_message;
  v->is_rootless           = &x11_is_rootless;
  v->init                  = &x11_init;
  v->setmode               = &x11_setmode;
  v->close                 = &x11_close;
  v->window_debug          = &x11_window_debug;
  v->window_fullscreen     = &x11_window_fullscreen;
  v->window_new            = &x11_window_new;
  v->window_free           = &x11_window_free;
  v->window_set_title      = &x11_window_set_title;
  v->window_set_position   = &x11_window_set_position;
  v->window_set_size       = &x11_window_set_size;
  v->window_get_position   = &x11_window_get_position;
  v->window_get_size       = &x11_window_get_size;
  v->multiblit             = &x11_multiblit;
  v->bitmap_getshm         = &x11_bitmap_getshm;
  v->charblit              = &x11_charblit;
  v->blur                  = &x11_blur;
  v->fpolygon              = &x11_fpolygon;
  v->rotateblit            = &x11_rotateblit;
  v->gradient              = &x11_gradient;
  v->scrollblit            = &x11_scrollblit;
  v->window_set_flags      = &x11_window_set_flags;
#ifdef CONFIG_FONTENGINE_FREETYPE
  v->alpha_charblit        = &x11_alpha_charblit;
#endif

  return success;
}

/* The End */
