/* $Id: x11_primitives.c,v 1.1 2002/11/04 08:36:25 micahjd Exp $
 *
 * x11_primitives.c - Implementation of picogui primitives on top of the
 *                    X window system.
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


/******************************************************** Primitives */

void x11_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  GC g = x11_gctab[lgop];
  if (!g) {
    def_pixel(dest,x,y,c,lgop);
    return;
  }

  XSetForeground(x11_display,g,c);
  XDrawPoint(x11_display,XB(dest)->d,g,x,y);
}

hwrcolor x11_getpixel(hwrbitmap src,s16 x,s16 y) {
  XImage *img;
  hwrcolor c;

  /* This is really _really_ damn slow. Our goal is to never 
   * have to actually use this function, but we need it for
   * compatibility. I thought about caching the XImage to
   * improve consecutive reads from the same drawable, but
   * there's no good way to determine if the drawable's been
   * modified since the last call.
   */
  img = XGetImage (x11_display, XB(src)->d, x, y, 1, 1, AllPlanes, ZPixmap);
  c = XGetPixel(img,0,0);
  XDestroyImage(img);
  return c;
}

void x11_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    def_rect(dest,x,y,w,h,c,lgop);
    return;
  }
  XSetForeground(x11_display,g,c);
  XFillRectangle(x11_display,XB(dest)->d,g,x,y,w,h);
}

void x11_line(hwrbitmap dest, s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    def_line(dest,x1,y1,x2,y2,c,lgop);
    return;
  }
  XSetForeground(x11_display,g,c);
  XDrawLine(x11_display,XB(dest)->d,g,x1,y1,x2,y2);
}

void x11_slab(hwrbitmap dest, s16 x,s16 y,s16 w, hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    def_slab(dest,x,y,w,c,lgop);
    return;
  }
  XSetForeground(x11_display,g,c);
  XFillRectangle(x11_display,XB(dest)->d,g,x,y,w,1);
}

void x11_bar(hwrbitmap dest, s16 x,s16 y,s16 h, hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    def_bar(dest,x,y,h,c,lgop);
    return;
  }
  XSetForeground(x11_display,g,c);
  XFillRectangle(x11_display,XB(dest)->d,g,x,y,1,h);
}

void x11_ellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    def_ellipse(dest,x,y,w,h,c,lgop);
    return;
  }
  XSetForeground(x11_display,g,c);
  XDrawArc(x11_display,XB(dest)->d,g,x,y,w,h,0,360*64);
}

void x11_fellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    def_fellipse(dest,x,y,w,h,c,lgop);
    return;
  }
  XSetForeground(x11_display,g,c);
  XFillArc(x11_display,XB(dest)->d,g,x,y,w,h,0,360*64);
}

void x11_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop) {
  GC g = x11_gctab[lgop];
  if (!g) {
    def_blit(dest,x,y,w,h,src,src_x,src_y,lgop);
    return;
  }

  XCopyArea(x11_display,XB(src)->d,XB(dest)->d,g,src_x,src_y,w,h,x,y);
}

void x11_update(hwrbitmap display,s16 x,s16 y,s16 w,s16 h) {
  XFlush(x11_display);
}

/* The End */
