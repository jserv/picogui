/* $Id$
 *
 * x11_util.c - Utility functions for picogui's driver for the X window system
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
#include <pgserver/x11.h>
#include <pgserver/configfile.h>


hwrbitmap x11_window_debug(void) {
  if (VID(is_rootless)()) {
    if (!x11_debug_window) {
      /* FIXME: There's nothing we can do with an error here */
      x11_window_new(&x11_debug_window,NULL);
      XStoreName(x11_display, XB(x11_debug_window)->frontbuffer ? 
		 XB(x11_debug_window)->frontbuffer->d : XB(x11_debug_window)->d,
		 "PicoGUI Debug Window");
      x11_internal_window_resize(x11_debug_window,640,480);
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
  g_error e;
  XSetWindowAttributes attr;

  e = g_malloc((void**)&xb, sizeof(struct x11bitmap));
  errorcheck;
  memset(xb,0,sizeof(struct x11bitmap));
  xb->is_window = 1;

  /* Create the window.
   * The size and everything else will be configured in x11_internal_window_resize()
   */
  xb->d = XCreateSimpleWindow(x11_display, RootWindow(x11_display, x11_screen),0,0,1,1,0,0,0);
  xb->sb.w = xb->sb.h = 0;

  /* This prevents X from redrawing the window background at all */
  XSetWindowBackgroundPixmap(x11_display, xb->d, None);

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
    if (XB(window)->frontbuffer)
      x11_bitmap_free((hwrbitmap) XB(window)->frontbuffer);

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
  struct pgstring *s;
  struct pgstr_iterator i;

  /* Make a null-terminated UTF-8 string 
   */
  if (!iserror(pgstring_convert(&s, PGSTR_ENCODE_UTF8, title))) {
    pgstring_seek(s, &i, 1, PGSEEK_END);
    if (!iserror(pgstring_insert_char(s, &i, 0, NULL)))
      XStoreName(x11_display, xb->d, s->buffer);
    pgstring_delete(s);
  }
}

void x11_window_set_position(hwrbitmap window, s16 x, s16 y) {
  struct x11bitmap *xb = XB(window)->frontbuffer ? XB(window)->frontbuffer : XB(window);
  XWindowChanges wc;

  if (VID(is_rootless)()) {
    wc.x = x;
    wc.y = y;
    XConfigureWindow(x11_display, xb->d, CWX | CWY, &wc);
  }
}

void x11_window_set_size(hwrbitmap window, s16 w, s16 h) {
  if (VID(is_rootless)())
    x11_internal_window_resize(window,w,h);
}

void x11_internal_window_resize(hwrbitmap window, int w, int h) {
  XWindowChanges wc;
  XEvent ev;
  struct x11bitmap *xb = XB(window)->frontbuffer ? XB(window)->frontbuffer : XB(window);
  Atom a;

  /* Can't have a zero size, X will throw an error */
  if (!w) w = 1;
  if (!h) h = 1;
  
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

  /* Set WM protocols - we just need close events */
  a = XInternAtom(x11_display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(x11_display, xb->d, &a, 1);

  /* Do we need to grab the pointer? */
  if (xb->window_flags & PG_WINDOW_GRAB)
    XGrabPointer(x11_display, xb->d, False, EnterWindowMask | LeaveWindowMask |
		 PointerMotionMask | ButtonMotionMask | ButtonPressMask |
		 ButtonReleaseMask | Button1MotionMask | Button2MotionMask |
		 Button3MotionMask, GrabModeAsync, GrabModeAsync, None,
		 None, CurrentTime);

  x11_acknowledge_resize(window,w,h);
}

void x11_window_get_position(hwrbitmap window, s16 *x, s16 *y) {
  struct x11bitmap *xb = XB(window)->frontbuffer ? XB(window)->frontbuffer : XB(window);
  int ix,iy;
  Window child;
  
  /* We want the position relative to the root, even if this is not
   * a child of the root. (It probably won't be, due to the window manager)
   */
  XTranslateCoordinates(x11_display, xb->d,
			RootWindow(x11_display,x11_screen),
			0,0,&ix,&iy,&child);
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

void x11_expose(struct x11bitmap *xb, Region r) {
  if (xb->frontbuffer) {
    /* Double-buffered expose update */
    XSetRegion(x11_display,x11_gctab[PG_LGOP_NONE],r);
    XCopyArea(x11_display,xb->d,xb->frontbuffer->d,
	      x11_gctab[PG_LGOP_NONE],0,0,xb->sb.w,xb->sb.h,0,0);
    XSetRegion(x11_display,x11_gctab[PG_LGOP_NONE],x11_display_region);
  }
  else {
    /* Ugly non-double-buffered expose update */

    struct divtree *p;
    int i;
    
    for (i=0;i<=PG_LGOPMAX;i++)
      if (x11_gctab[i])
	XSetRegion(x11_display,x11_gctab[i],r);
    x11_current_region = r;

    for (p=dts->top;p;p=p->next)
      if (p->display == (hwrbitmap)xb)
	p->flags |= DIVTREE_ALL_REDRAW;
    update(NULL,1);

    for (i=0;i<=PG_LGOPMAX;i++)
      if (x11_gctab[i])
	XSetRegion(x11_display,x11_gctab[i],x11_display_region);
    x11_current_region = x11_display_region;
  }
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
    x11_internal_window_resize(vid->display, vid->xres, vid->yres);
    title[sizeof(title)-1] = 0;

    snprintf(title,sizeof(title)-1,get_param_str("video-x11","caption","PicoGUI (X11@%dx%dx%d)"),
	     vid->xres,vid->yres,vid->bpp);
    XStoreName(x11_display, XB(vid->display)->frontbuffer ? 
	       XB(vid->display)->frontbuffer->d : XB(vid->display)->d,
	       title);
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
  if (XB(window)->sb.w != w || XB(window)->sb.h != h) {
    XB(window)->sb.w = w;
    XB(window)->sb.h = h;
    
    /* Resize the backbuffer if we're using one */
    if (XB(window)->frontbuffer) {
      /* FIXME: can't deal with errors here! */
      x11_internal_bitmap_free(XB(window));
      XB(window)->sb.w = XB(window)->frontbuffer->sb.w = w;
      XB(window)->sb.h = XB(window)->frontbuffer->sb.h = h;
      x11_new_bitmap_pixmap(XB(window));
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

    /* If this is monolithic mode, we need to set the video mode to match */
    if ((!VID(is_rootless)()) && (vid->xres!=w || vid->yres!=h))
      video_setmode(w, h, vid->bpp, PG_FM_ON,0); 
  }
}

void x11_window_set_flags(hwrbitmap window, int flags) {
  struct x11bitmap *xb = XB(window)->frontbuffer ? XB(window)->frontbuffer : XB(window);
  XSetWindowAttributes attr;
  xb->window_flags = flags;

  /* If this is an unmanaged window or the background,
   * use OverrideRedirect to keep the window manager from messing with it.
   */
  attr.override_redirect = (flags & (PG_WINDOW_UNMANAGED | PG_WINDOW_BACKGROUND)) != 0;
  XChangeWindowAttributes(x11_display, xb->d, CWOverrideRedirect, &attr);

  /* Background windows sit below everything else and take the whole screen */
  if (flags & PG_WINDOW_BACKGROUND) {
    XLowerWindow(x11_display, xb->d);
    VID(window_set_size)(dts->top->display,vid->lxres,vid->lyres);
  }
}

/* The End */
