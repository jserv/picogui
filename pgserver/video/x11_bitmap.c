/* $Id: x11_bitmap.c,v 1.1 2002/11/07 00:44:57 micahjd Exp $
 *
 * x11_bitmap.c - Utilities for dealing with bitmaps in X
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


g_error x11_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend) {
  return def_bitmap_get_groprender(&XB(bmp)->sb,rend);
}

g_error x11_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h) {
  return def_bitmap_getsize(&XB(bmp)->sb,w,h);
}

g_error x11_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp) {
  struct x11bitmap **pxb = (struct x11bitmap **) bmp;
  g_error e;

  /* Allocate an x11bitmap structure for this */
  e = g_malloc((void **) pxb,sizeof(struct x11bitmap));
  errorcheck;
  memset(*pxb,0,sizeof(struct x11bitmap));
  (*pxb)->sb.w = w;
  (*pxb)->sb.h = h;

  e = x11_new_bitmap_pixmap(*pxb);
  errorcheck;

  return success;
}

void x11_bitmap_free(hwrbitmap bmp) {
  x11_internal_bitmap_free(XB(bmp));
  g_free(bmp);
}

/* Create a backbuffer for double-buffering the given surface */
g_error x11_new_backbuffer(struct x11bitmap **backbuffer, struct x11bitmap *frontbuffer) {
  g_error e;

  e = VID(bitmap_new)((hwrbitmap*)backbuffer, frontbuffer->sb.w, frontbuffer->sb.h, vid->bpp);
  errorcheck;

  (*backbuffer)->frontbuffer = frontbuffer;
  return success;
}

g_error x11_bitmap_getshm(hwrbitmap bmp, u32 uid, struct pgshmbitmap *shm) {
}

/* Free the internal representation of a bitmap without freeing
 * the x11bitmap structure itself.
 */
void x11_internal_bitmap_free(struct x11bitmap *xb) {
  if (xb->sb.rend) {
    g_free(xb->sb.rend);
    xb->sb.rend = NULL;
  }

  if (xb->sb.freebits) {
    g_free(xb->sb.bits);
    xb->sb.freebits = 0;
    xb->sb.bits = NULL;
  }

  if (xb->sb.shm_id) {
    os_shm_free(xb->sb.bits, xb->sb.shm_id);
    xb->sb.shm_id = 0;
  }

  if (xb->is_window)
    XDestroyWindow(x11_display,xb->d);
  else
    XFreePixmap(x11_display,xb->d);
}

/* Allocate a pixmap for internal representation
 * of the given bitmap, using SHM if possible.
 */
g_error x11_new_bitmap_pixmap(struct x11bitmap *xb) {
  int w=xb->sb.w, h=xb->sb.h;

  /* X doesn't like 0 dimensions */
  if (!w) w = 1;
  if (!h) h = 1;
  
  /* Allocate a corresponding X pixmap */
  xb->d = XCreatePixmap(x11_display,RootWindow(x11_display, x11_screen),w,h,vid->bpp);

}

/* The End */
