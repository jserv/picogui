/* $Id: x11_util.c,v 1.4 2002/11/04 12:11:32 micahjd Exp $
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

  /* X doesn't like 0 dimensions */
  if (!w) w = 1;
  if (!h) h = 1;

  /* Allocate a corresponding X pixmap */
  (*pxb)->d  = XCreatePixmap(x11_display,RootWindow(x11_display, 0),w,h,vid->bpp);

  return success;
}

void x11_bitmap_free(hwrbitmap bmp) {
  if (XB(bmp)->frontbuffer)
    x11_bitmap_free((hwrbitmap) XB(bmp)->frontbuffer);

  if (XB(bmp)->rend)
    g_free(XB(bmp)->rend);

  if (XB(bmp)->is_window)
    XDestroyWindow(x11_display,XB(bmp)->d);
  else
    XFreePixmap(x11_display,XB(bmp)->d);
  
  g_free(bmp);
}

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

hwrbitmap x11_window_debug(void) {
  if (VID(is_rootless)()) {
    if (!x11_debug_window) {
      /* FIXME: There's nothing we can do with an error here */
      x11_window_new(&x11_debug_window,NULL);
      x11_window_set_title(x11_debug_window,pgstring_tmpwrap("PicoGUI Debug Window"));
      x11_window_set_size(x11_debug_window,640,480);
    }
    return x11_debug_window;
  }
  return x11_monolithic_window();
}
  

hwrbitmap x11_window_fullscreen(void) {
  if (VID(is_rootless)()) {
    /* FIXME: Implement me */
    return x11_window_debug();
  }
  return x11_monolithic_window();
}

g_error x11_window_new(hwrbitmap *hbmp, struct divtree *dt) {
  g_error e;

  /* In rootless mode, create a special window */
  if (VID(is_rootless)()) {
    e = x11_create_window(hbmp);
    errorcheck;
  }

  /* In monolithic mode, everybody uses the same window */
  else
    *hbmp = x11_monolithic_window();

  XB(*hbmp)->dt = dt;
  return success;
}

g_error x11_create_window(hwrbitmap *hbmp) {
  struct x11bitmap *xb;
  int black;
  g_error e;

  e = g_malloc((void**)&xb, sizeof(struct x11bitmap));
  errorcheck;
  memset(xb,0,sizeof(struct x11bitmap));
  xb->is_window = 1;

  black = BlackPixel(x11_display, DefaultScreen(x11_display));

  /* Create the window.
   * The size and everything else will be configured in x11_window_set_size()
   */
  xb->d = XCreateSimpleWindow(x11_display, DefaultRootWindow(x11_display),
			      0, 0, 1, 1, 0, black, black);
  xb->w = xb->h = 0;

  /* Optionally double-buffer this window */
  if (get_param_int("video-x11","doublebuffer",1)) {
    e = x11_new_backbuffer(&xb, xb);
    errorcheck;
  }

  xb->next_window = x11_window_list;
  x11_window_list = xb;

  *hbmp = (hwrbitmap) xb;
  return success;
}

void x11_window_free(hwrbitmap window) {
  struct x11bitmap **b;

  if (VID(is_rootless)()) {
    /* Remove it from the window list */
    for (b=&x11_window_list;*b;b=&(*b)->next_window)
      if (*b == XB(window)) {
	*b = (*b)->next_window;
	break;
      }

    x11_bitmap_free(window);
  }      
}

void x11_window_set_title(hwrbitmap window, const struct pgstring *title) {
  struct x11bitmap *xb = XB(window)->frontbuffer ? XB(window)->frontbuffer : XB(window);
  XStoreName(x11_display, xb->d, title->buffer);
}

void x11_window_set_position(hwrbitmap window, s16 x, s16 y) {
  struct x11bitmap *xb = XB(window)->frontbuffer ? XB(window)->frontbuffer : XB(window);
  XWindowChanges wc;

  wc.x = x;
  wc.y = y;
  XConfigureWindow(x11_display, xb->d, CWX | CWY, &wc);
}

void x11_window_set_size(hwrbitmap window, s16 w, s16 h) {
  XWindowChanges wc;
  XEvent ev;
  struct x11bitmap *xb = XB(window)->frontbuffer ? XB(window)->frontbuffer : XB(window);

  /* Resize the window */
  wc.width = w;
  wc.height = h;
  XConfigureWindow(x11_display, xb->d, CWWidth | CWHeight, &wc);

  if (!xb->is_mapped) {
    /* Map the window, waiting for the MapNotify event */
    XSelectInput(x11_display, xb->d, StructureNotifyMask);  
    XMapWindow(x11_display, xb->d);
    do {
      XNextEvent(x11_display, &ev);
    } while (ev.type != MapNotify);
    xb->is_mapped = 1;
  }

  /* Set input event mask */
  XSelectInput(x11_display, xb->d,
	       KeyPressMask | KeyReleaseMask | ExposureMask | ButtonMotionMask |
	       ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);
  XAutoRepeatOn(x11_display);

  XFlush(x11_display);
}

void x11_window_get_position(hwrbitmap window, s16 *x, s16 *y) {
  struct x11bitmap *xb = XB(window)->frontbuffer ? XB(window)->frontbuffer : XB(window);
  int ix,iy,iw,ih,border,depth;
  Window root;
  XGetGeometry(x11_display, xb->d, &root, &ix, &iy, &iw, &ih, &border, &depth);
  *x = ix;
  *y = iy;
}

void x11_window_get_size(hwrbitmap window, s16 *w, s16 *h) {
  struct x11bitmap *xb = XB(window)->frontbuffer ? XB(window)->frontbuffer : XB(window);
  int ix,iy,iw,ih,border,depth;
  Window root;
  XGetGeometry(x11_display, xb->d, &root, &ix, &iy, &iw, &ih, &border, &depth);
  *w = iw;
  *h = ih;
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

void x11_expose(Window w, Region r) {
  struct x11bitmap *xb = x11_get_window(w);
  if (!xb) 
    return;

  if (xb->frontbuffer) {
    /* Double-buffered expose update */
    XSetRegion(x11_display,x11_gctab[PG_LGOP_NONE],r);
    XCopyArea(x11_display,xb->d,xb->frontbuffer->d,
	      x11_gctab[PG_LGOP_NONE],0,0,xb->w,xb->h,0,0);
    XSetRegion(x11_display,x11_gctab[PG_LGOP_NONE],x11_display_region);
  }
  else {
    /* Ugly non-double-buffered expose update */

    struct divtree *p;
    int i;
    
    for (i=0;i<=PG_LGOPMAX;i++)
      if (x11_gctab[i])
	XSetRegion(x11_display,x11_gctab[i],r);
    
    for (p=dts->top;p;p=p->next)
      if (p->display == (hwrbitmap)xb)
	p->flags |= DIVTREE_ALL_REDRAW;
    update(NULL,1);

    for (i=0;i<=PG_LGOPMAX;i++)
      if (x11_gctab[i])
	XSetRegion(x11_display,x11_gctab[i],x11_display_region);
  }
}

/* Create a backbuffer for double-buffering the given surface */
g_error x11_new_backbuffer(struct x11bitmap **backbuffer, struct x11bitmap *frontbuffer) {
  g_error e;

  e = VID(bitmap_new)((hwrbitmap*)backbuffer, frontbuffer->w, frontbuffer->h, vid->bpp);
  errorcheck;

  (*backbuffer)->frontbuffer = frontbuffer;
  return success;
}

/* Return the shared window used in non-rootless mode, creating it if it doens't exist */
hwrbitmap x11_monolithic_window(void) {
  if (!vid->display) {
    /* FIXME: Can't deal with an error here */
    x11_create_window(&vid->display);
    x11_monolithic_window_update();
  }
  return vid->display;
}

/* Update the title and size of the monolithic window if necessary */
void x11_monolithic_window_update(void) {
  char title[256];

  if (vid->display) {
    x11_window_set_size(vid->display, vid->xres, vid->yres);
    title[sizeof(title)-1] = 0;
    snprintf(title,sizeof(title)-1,get_param_str("video-x11","caption","PicoGUI (X11@%dx%dx%d)"),
	     vid->xres,vid->yres,vid->bpp);
    x11_window_set_title(vid->display,pgstring_tmpwrap(title));
  }  
}

/* Map an X Window to the associated picogui x11bitmap */
struct x11bitmap *x11_get_window(Window w) {
  struct x11bitmap *b;

  for (b=x11_window_list;b;b=b->next_window) {
    if (b->d == w)
      return b;
    if (b->frontbuffer && b->frontbuffer->d == w)
      return b;
  }
  
  return NULL;
}


/* Called by x11input whenever a window's size changes, 
 * even in response to x11_window_set_size 
 */
void x11_acknowledge_resize(hwrbitmap window, int w, int h) {
  if (XB(window)->w != w || XB(window)->h != h) {
    XB(window)->w = w;
    XB(window)->h = h;
    
    /* Resize the backbuffer if we're using one */
    if (XB(window)->frontbuffer) {
      XFreePixmap(x11_display, XB(window)->d);
      XB(window)->frontbuffer->w = w;
      XB(window)->frontbuffer->h = h;
      if (XB(window)->rend)
	g_free(XB(window)->rend);
      XB(window)->d = XCreatePixmap(x11_display, XB(window)->frontbuffer->d, w, h, vid->bpp);
    }
    
    /* Resize the divtree */
    if (XB(window)->dt) {
      XB(window)->dt->head->r.w = w;
      XB(window)->dt->head->r.h = h;
      XB(window)->dt->head->calc.w = w;
      XB(window)->dt->head->calc.h = h;
      XB(window)->dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_FORCE_CHILD_RECALC | DIVNODE_NEED_REBUILD;
      XB(window)->dt->flags |= DIVTREE_NEED_RECALC | DIVTREE_ALL_REDRAW;
    }

    update(NULL,1);
  }
}

/* The End */
