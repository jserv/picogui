/* $Id$
 *
 * x11.h - Header shared by all the x11 driver components in picogui
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

#ifndef __H_PGX11
#define __H_PGX11

#include <pgserver/render.h>
#include <pgserver/video.h>
#include <pgserver/divtree.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>

#ifdef __SVR4
#include <X11/Sunkeysym.h>
#endif

#ifdef CONFIG_FONTENGINE_XFT
#include <X11/Xft/Xft.h>
#endif


/******************************************************** Data types */

/* The new data type used for hwrbitmap.
 * This must be able to represent top-level windows as well as
 * offscreen bitmaps.
 */
struct x11bitmap {
  /* Client-side representation used when we have SHM support.
   * The stdbitmap here is also used to store common parameters,
   * like size and groprender context.
   */
  struct stdbitmap sb;

  /* SHM information. If SHM support is enabled,
   * we will use both traditional X primitives, and our own
   * linear VBLs via SHM.
   */
  s32 shm_key;
  XShmSegmentInfo shminfo;
  struct vidlib *lib;            /* The primitives to use in SHM mode */
  int using_shm;                 /* 1 if we're drawing using SHM, 0
				  * if we're using plain X. Any time this
				  * changes, we have to synchronize
				  * ourself with the X server.
				  */

  /* X server-side representation, either a pixmap or window */
  Drawable d;

  /* Windows */
  unsigned int is_window:1;      /* Flag indicating if this is a window */
  unsigned int is_mapped:1;      /* Is the window mapped yet? */
  struct divtree *dt;            /* The corresponding divtree if this is a window */
  struct x11bitmap *next_window; /* Linked list of windows */
  struct x11bitmap *frontbuffer; /* If this is a backbuffer, this is the associated front buffer */
  int window_flags;
};

/* Convenience macro to cast a hwrbitmap to x11bitmap */
#define XB(b) ((struct x11bitmap *)(b))


/******************************************************** Globals */

/* Display and file descriptor for the X server */
extern Display *x11_display;
extern int x11_fd, x11_screen;

/* Stipple bitmap */
#define x11_stipple_width 8
#define x11_stipple_height 8
extern const u8 x11_stipple_bits[];

/* A table of graphics contexts for each LGOP mode */
extern GC x11_gctab[PG_LGOPMAX+1];

/* The window used by x11_window_debug */
extern hwrbitmap x11_debug_window;

/* A list of all allocated windows */
extern struct x11bitmap *x11_window_list;

/* A region specifying the entire display */
extern Region x11_display_region;

/* A region specifying the area we're currently rendering to */
extern Region x11_current_region;

/* We're using SHM if nonzero */
extern int x11_using_shm;

/* VBLs for each color depth, or NULL if not available */
#ifdef CONFIG_VBL_LINEAR16
extern struct vidlib x11_vbl_linear16;
#endif
#ifdef CONFIG_VBL_LINEAR32
extern struct vidlib x11_vbl_linear32;
#endif


/******************************************************** Shared utilities */

/* Redisplay the area inside the given expose region */
void x11_expose(struct x11bitmap *xb, Region r);

/* Generate a table of GCs for each lgop */
void x11_gc_setup(Drawable d);

/* Create a backbuffer for double-buffering the given surface */
g_error x11_new_backbuffer(struct x11bitmap **backbuffer, struct x11bitmap *frontbuffer);

/* Create a new generic window */
g_error x11_create_window(hwrbitmap *hbmp);

/* Return the shared window used in non-rootless mode, creating it if it doesn't exist */
hwrbitmap x11_monolithic_window(void);

/* Update the title and size of the monolithic window if necessary */
void x11_monolithic_window_update(void);

/* Map an X Window to the associated picogui x11bitmap */
struct x11bitmap *x11_get_window(Window w);

/* Called by x11input whenever a window's size changes, 
 * even in response to x11_window_set_size 
 */
void x11_acknowledge_resize(hwrbitmap window, int w, int h);

/* Internals of x11_window_set_resize, will work in rootless or monolithic mode */
void x11_internal_window_resize(hwrbitmap window, int w, int h);

/* Free the internal representation of a bitmap without freeing
 * the x11bitmap structure itself.
 */
void x11_internal_bitmap_free(struct x11bitmap *xb);

/* Allocate a pixmap for internal representation
 * of the given bitmap, using SHM if possible.
 */
g_error x11_new_bitmap_pixmap(struct x11bitmap *xb);


/******************************************************** Primitives */

void x11_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop);
hwrcolor x11_getpixel(hwrbitmap src,s16 x,s16 y);
void x11_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);
void x11_line(hwrbitmap dest, s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c, s16 lgop);
void x11_slab(hwrbitmap dest, s16 x,s16 y,s16 w, hwrcolor c, s16 lgop);
void x11_bar(hwrbitmap dest, s16 x,s16 y,s16 h, hwrcolor c, s16 lgop);
void x11_ellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);
void x11_fellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);
void x11_update(hwrbitmap display,s16 x,s16 y,s16 w,s16 h);
g_error x11_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend);
g_error x11_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h);
g_error x11_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp);
void x11_bitmap_free(hwrbitmap bmp);
void x11_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop);
void x11_message(u32 message, u32 param, u32 *ret);
int x11_is_rootless(void);
g_error x11_init(void);
g_error x11_setmode(s16 xres,s16 yres,s16 bpp,u32 flags);
void x11_close(void);
hwrbitmap x11_window_debug(void);  
hwrbitmap x11_window_fullscreen(void);  
g_error x11_window_new(hwrbitmap *hbmp, struct divtree *dt);
void x11_window_free(hwrbitmap window);
void x11_window_set_title(hwrbitmap window, const struct pgstring *title);
void x11_window_set_position(hwrbitmap window, s16 x, s16 y);
void x11_window_set_size(hwrbitmap window, s16 w, s16 h);
void x11_window_get_position(hwrbitmap window, s16 *x, s16 *y);
void x11_window_get_size(hwrbitmap window, s16 *w, s16 *h);
void x11_multiblit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		   hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 xo, s16 yo, s16 lgop);
g_error x11_bitmap_getshm(hwrbitmap bmp, u32 uid, struct pgshmbitmap *shm);
void x11_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
		  s16 lines, s16 angle, hwrcolor c, struct pgquad *clip,
		  s16 lgop, int char_pitch);
void x11_alpha_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
			int char_pitch, u8 *gammatable, s16 angle, hwrcolor c,
			struct pgquad *clip, s16 lgop);
void x11_blur(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, s16 radius);
void x11_fpolygon(hwrbitmap dest, s32* array, s16 xoff, s16 yoff , hwrcolor c, s16 lgop);
void x11_rotateblit(hwrbitmap dest, s16 dest_x, s16 dest_y,
		    hwrbitmap src, s16 src_x, s16 src_y, s16 src_w, s16 src_h,
		    struct pgquad *clip, s16 angle, s16 lgop);
void x11_gradient(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,s16 angle,
		  pgcolor c1, pgcolor c2, s16 lgop);
void x11_scrollblit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		    s16 src_x, s16 src_y, s16 lgop);
void x11_window_set_flags(hwrbitmap window, int flags);

#endif /* __H_PGX11 */

/* The End */
