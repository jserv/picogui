/* $Id$
 *
 * x11_bitmap.c - Utilities for dealing with bitmaps in X
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

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>



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
  (*pxb)->sb.bpp = bpp;

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
  if (!XB(bmp)->sb.shm_id)
    return mkerror(PG_ERRT_IO,145);   /* SHM bitmaps not supported */

  shm->shm_key     = htonl(XB(bmp)->shm_key);
  shm->shm_length  = htonl(XB(bmp)->sb.pitch * XB(bmp)->sb.h);
  shm->width       = htons(XB(bmp)->sb.w);
  shm->height      = htons(XB(bmp)->sb.h);
  shm->bpp         = htons(XB(bmp)->sb.bpp);
  shm->pitch       = htons(XB(bmp)->sb.pitch);

  /* Default color space information. Detect an alpha channel
   * if this bitmap has one.
   */
  def_shm_colorspace(XB(bmp)->sb.bpp,
		     XB(bmp)->sb.w && XB(bmp)->sb.h && 
		     (*(u32*)XB(bmp)->sb.bits & PGCF_ALPHA),
		     shm);
  return success;
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
    XShmDetach(x11_display,&xb->shminfo);
    XSync(x11_display, False);
    os_shm_free(xb->sb.bits, xb->sb.shm_id);
    xb->sb.shm_id = 0;
  }

  if (xb->d) {
    if (xb->is_window)
      XDestroyWindow(x11_display,xb->d);
    else
      XFreePixmap(x11_display,xb->d);
  }
}

/* Allocate a pixmap for internal representation
 * of the given bitmap, using SHM if possible.
 */
g_error x11_new_bitmap_pixmap(struct x11bitmap *xb) {
  g_error e;

  /* Nothing to create? */
  if (!(xb->sb.w && xb->sb.h && xb->sb.bpp))
    return success;
  
  if (x11_using_shm) {

    /* FIXME: This assumes that the pitch is padded to the
     *        nearest DWORD, like it is in current implementations
     *        of XFree86. This should instead use an XImage
     *        to determine the proper pitch.
     */
    xb->sb.pitch = (((xb->sb.w * xb->sb.bpp)>>3) + 3) & ~3;

    /* Set up us the SHM segment! 
     * FIXME: These SHM segments are insecure! Is there any way to have the X server,
     *        pgserver, and the client all share the segments without making them
     *        accessable to everyone?
     */
    memset(&xb->shminfo, 0, sizeof(xb->shminfo));
    e = os_shm_alloc(&xb->sb.bits, xb->sb.pitch * xb->sb.h, 
		     &xb->sb.shm_id, &xb->shm_key, 0);
    errorcheck;

    xb->shminfo.shmaddr  = xb->sb.bits;
    xb->shminfo.shmid    = xb->sb.shm_id;
    xb->shminfo.readOnly = False;

    /* Get the X server to attach this segment to a pixmap */
    XShmAttach(x11_display, &xb->shminfo);
    xb->d = XShmCreatePixmap(x11_display, RootWindow(x11_display, x11_screen),
			     xb->sb.bits,&xb->shminfo,xb->sb.w,xb->sb.h,
			     DefaultDepth(x11_display,x11_screen));

    /* Set the VBL used to draw to the SHM bitmap directly */
    switch (xb->sb.bpp) {
#ifdef CONFIG_VBL_LINEAR16
    case 16:
      xb->lib = &x11_vbl_linear16;
      break;
#endif
#ifdef CONFIG_VBL_LINEAR32
    case 32:
      xb->lib = &x11_vbl_linear32;
      break;
#endif
    default:
      xb->lib = NULL;
    }
  }
   
  else {
    /* No SHM, allocate a normal pixmap */
    xb->d = XCreatePixmap(x11_display,RootWindow(x11_display, x11_screen),
			  xb->sb.w,xb->sb.h,DefaultDepth(x11_display, x11_screen));
  }
  
  return success;
}

/* The End */
