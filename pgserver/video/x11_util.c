/* $Id: x11_util.c,v 1.1 2002/11/04 08:36:25 micahjd Exp $
 *
 * x11_util.c - Utility functions for picogui's driver for the X window system
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
#include <pgserver/x11.h>
#include <pgserver/configfile.h>


/******************************************************** Utilities */

g_error x11_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend) {
  g_error e;
  s16 w,h;

  if (XB(bmp)->rend) {
    *rend = XB(bmp)->rend;
    return success;
  }

  VID(bitmap_getsize)(bmp,&w,&h);

  /* New groprender context for this bitmap */
  e = g_malloc((void **) rend,sizeof(struct groprender));
  errorcheck;
  XB(bmp)->rend = *rend;
  memset(*rend,0,sizeof(struct groprender));
  (*rend)->lgop = PG_LGOP_NONE;
  (*rend)->output = bmp;
  (*rend)->hfont = res[PGRES_DEFAULT_FONT];
  (*rend)->clip.x2 = w - 1;
  (*rend)->clip.y2 = h - 1;
  (*rend)->orig_clip = (*rend)->clip;
  (*rend)->output_rect.w = w;
  (*rend)->output_rect.h = h;

  return success;
}

g_error x11_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h) {
  *w = XB(bmp)->w;
  *h = XB(bmp)->h;
  return success;
}

g_error x11_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp) {
  struct x11bitmap **pxb = (struct x11bitmap **) bmp;
  g_error e;

  /* Allocate an x11bitmap structure for this */
  e = g_malloc((void **) pxb,sizeof(struct x11bitmap));
  errorcheck;
  memset(*pxb,0,sizeof(struct x11bitmap));
  (*pxb)->w = w;
  (*pxb)->h = h;

  /* Allocate a corresponding X pixmap */
  (*pxb)->d  = XCreatePixmap(x11_display,RootWindow(x11_display, 0),w,h,vid->bpp);

  return success;
}

void x11_bitmap_free(hwrbitmap bmp) {
  if (XB(bmp)->rend)
    g_free(XB(bmp)->rend);
  XFreePixmap(x11_display,XB(bmp)->d);
  g_free(bmp);
}

void x11_message(u32 message, u32 param, u32 *ret) {
  switch (message) {

  case PGDM_SOUNDFX:
    /* XFree86 ignores the volume, it seems */
    if (get_param_int("video-x11","sound",1)) 
      XBell(x11_display,50);
    break;
  }
}

int x11_is_rootless(void) {
  return (vid->flags & PG_VID_ROOTLESS)!=0;
}

hwrbitmap x11_window_debug(void) {
  if (!x11_debug_window) {
    /* FIXME: There's nothing we can do with an error here */
    x11_window_new(&x11_debug_window,NULL);
    x11_window_set_title(x11_debug_window,pgstring_tmpwrap("PicoGUI Debug Window"));
  }
  return x11_debug_window;
}

hwrbitmap x11_window_fullscreen(void) {
  /* FIXME: Implement me */
  return x11_window_debug();
}

g_error x11_window_new(hwrbitmap *hbmp, struct divtree *dt) {
  struct x11bitmap *xb;
  XEvent ev;
  int black;
  XRectangle rect;
  g_error e;

  e = g_malloc((void**)&xb, sizeof(struct x11bitmap));
  errorcheck;
  memset(xb,0,sizeof(struct x11bitmap));

  black = BlackPixel(x11_display, DefaultScreen(x11_display));

  /* Create the window */
  xb->d = XCreateSimpleWindow(x11_display, DefaultRootWindow(x11_display),
			      0, 0, vid->xres, vid->yres, 0, black, black);
  
  /* Map the window, waiting for the MapNotify event */
  XSelectInput(x11_display, xb->d, StructureNotifyMask);  
  XMapWindow(x11_display, xb->d);
  do {
    XNextEvent(x11_display, &ev);
  } while (ev.type != MapNotify);
  
  /* Create the display_region */
  xb->display_region = XCreateRegion();
  rect.x = rect.y = 0;
  rect.width = vid->xres;
  rect.height = vid->yres;
  XUnionRectWithRegion(&rect,xb->display_region,xb->display_region);

  /* Set input event mask */
  XSelectInput(x11_display, xb->d,
	       KeyPressMask | KeyReleaseMask | ExposureMask | ButtonMotionMask |
	       ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);
  XAutoRepeatOn(x11_display);

  XFlush(x11_display);

  *hbmp = (hwrbitmap) xb;
  return success;
}

void x11_window_free(hwrbitmap window) {
  XDestroyRegion(XB(window)->display_region);
  if (XB(window)->rend)
    g_free(XB(window)->rend);
}

void x11_window_set_title(hwrbitmap window, const struct pgstring *title) {
  struct pgstring *s;
  if (!iserror(pgstring_convert(&s,PGSTR_ENCODE_UTF8,title))) {
    XStoreName(x11_display, XB(window)->d, s->buffer);
    pgstring_delete(s);
  }
}

void x11_window_set_position(hwrbitmap window, s16 x, s16 y) {
}

void x11_window_set_size(hwrbitmap window, s16 w, s16 h) {
}

void x11_window_get_position(hwrbitmap window, s16 *x, s16 *y) {
}

void x11_window_get_size(hwrbitmap window, s16 *w, s16 *h) {
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

void x11_expose(Region r) {
}

/* The End */
