/* $Id: x11_init.c,v 1.6 2002/11/04 13:14:05 micahjd Exp $
 *
 * x11_init.c - Initialization for picogui'x driver for the X window system
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/common.h>
#include <pgserver/configfile.h>
#include <pgserver/input.h>
#include <pgserver/x11.h>
#include <stdio.h>
#include <string.h>

/* Display and file descriptor for the X server */
Display *x11_display;
int x11_fd;

/* A table of graphics contexts for each LGOP mode */
GC x11_gctab[PG_LGOPMAX+1];

/* Stipple bitmap */
const u8 x11_stipple_bits[] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};

/* The window used by x11_window_debug */
hwrbitmap x11_debug_window;

/* A list of all allocated windows */
struct x11bitmap *x11_window_list;


/******************************************************** Initialization */

g_error x11_init(void) {

  /* Connect to the default X server */
  x11_display = XOpenDisplay(NULL);

  if (!x11_display)
    return mkerror(PG_ERRT_IO,46);   /* Error initializing video */

  x11_fd = ConnectionNumber(x11_display);
  vid->bpp  = DefaultDepth(x11_display,0);
  vid->display = NULL;
  x11_gc_setup(RootWindow(x11_display, 0));
  
  /* Load the matching input driver */
  return load_inlib(&x11input_regfunc,&inlib_main);
}

g_error x11_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  int x,y,rootw,rooth,border,depth;
  Window root;
  XRectangle rect;

  /* Get the display size */
  XGetGeometry(x11_display, RootWindow(x11_display, 0), &root,
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

  x11_monolithic_window_update();
  return success;
}

void x11_close(void) {
  /* Clean up after the monolithic window and the debug window */
  if (vid->display)
    x11_bitmap_free(vid->display);
  if (x11_debug_window)
    x11_bitmap_free(x11_debug_window);

  XDestroyRegion(x11_display_region);  

  unload_inlib(inlib_main);   /* Take out our input driver */
  XCloseDisplay(x11_display);
  x11_display = NULL;
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

  return success;
}

/* The End */