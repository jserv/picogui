/* $Id: x11.c,v 1.19 2001/12/14 22:56:44 micahjd Exp $
 *
 * x11.c - Use the X Window System as a graphics backend for PicoGUI
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/video.h>
#include <pgserver/configfile.h>
#include <pgserver/appmgr.h>
#include <pgserver/render.h>
#include <pgserver/input.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#ifdef CONFIG_X11_XFT
#include <X11/Xft/Xft.h>
#endif

/* Global X display- shared with x11input driver */
Display *xdisplay;

/* Redefine PicoGUI's hwrbitmap so it references an X11 Drawable */
struct x11bitmap {
  Drawable d;
  s16 w,h;
  struct groprender *rend;   /* Context for pgRender() */
  struct x11bitmap *tile;    /* Cached tile            */
#ifdef CONFIG_X11_XFT
  XftDraw *xftd;
#endif
} x11_display;

/* This is our back-buffer when we double-buffer. Even if we're not
 * double-buffering everything, we still double-buffer sprites
 */
struct x11bitmap *x11_backbuffer;

/* In X11 there is a comparatively large per-blit penalty, and
 * no API function for tiling a bitmap. To speed things up
 * substantially, bitmaps below this size that need to be tiled
 * are pre-tiled up to at least this size and stored in the
 * bitmap's 'tile' variable.
 *
 * Larger values for this increase speed and memory consumption.
 */
#define X11_TILESIZE 128

/* A table of graphics contexts for each LGOP mode */
GC x11_gctab[PG_LGOPMAX+1];

/* Hook for handling expose events from the input driver */
void (*x11_expose)(Region r);

/* Saved config options */
int x11_autowarp;
int x11_sound;

/* Stipple bitmap */
#define stipple_width 8
#define stipple_height 8
static unsigned char stipple_bits[] = {
   0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};

/* A region specifying the entire screen, for resetting clipping */
Region display_region;

/******************************************** Low-level primitives */

void x11_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

#ifdef CONFIG_X11_NOPIXEL
  /* Paint it bright red, marking areas we can't render */
  c = vid->color_pgtohwr(0xFF0000);
  g = x11_gctab[PG_LGOP_NONE];
#else
  if (!g) {
#ifndef CONFIG_X11_NOSLOWLGOP
    def_pixel(dest,x,y,c,lgop);
#endif
    return;
  }
#endif
  XSetForeground(xdisplay,g,c);
  XDrawPoint(xdisplay,xb->d,g,x,y);
}

hwrcolor x11_getpixel(hwrbitmap src,s16 x,s16 y) {
#ifdef CONFIG_X11_NOPIXEL
  /* Return bright red, to mark the areas we're blocking out */
  return vid->color_pgtohwr(0xFF0000);
#else

  struct x11bitmap *xb = (struct x11bitmap *) src;
  XImage *img;
  hwrcolor c;

  /* This is really _really_ damn slow. Our goal is to never 
   * have to actually use this function, but we need it for
   * compatibility. I thought about caching the XImage to
   * improve consecutive reads from the same drawable, but
   * there's no good way to determine if the drawable's been
   * modified since the last call.
   */
  img = XGetImage (xdisplay, xb->d, x, y, 1, 1, AllPlanes, ZPixmap);
  c = XGetPixel(img,0,0);
  XDestroyImage(img);
  return c;
#endif
}

/******************************************** Standard primitives */

void x11_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_rect(dest,x,y,w,h,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XFillRectangle(xdisplay,xb->d,g,x,y,w,h);
}

void x11_line(hwrbitmap dest, s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_line(dest,x1,y1,x2,y2,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XDrawLine(xdisplay,xb->d,g,x1,y1,x2,y2);
}

void x11_slab(hwrbitmap dest, s16 x,s16 y,s16 w, hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_slab(dest,x,y,w,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XFillRectangle(xdisplay,xb->d,g,x,y,w,1);
}

void x11_bar(hwrbitmap dest, s16 x,s16 y,s16 h, hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_bar(dest,x,y,h,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XFillRectangle(xdisplay,xb->d,g,x,y,1,h);
}

void x11_ellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_ellipse(dest,x,y,w,h,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XDrawArc(xdisplay,xb->d,g,x,y,w,h,0,360*64);
}

void x11_fellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_fellipse(dest,x,y,w,h,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XFillArc(xdisplay,xb->d,g,x,y,w,h,0,360*64);
}

/* Blit the backbuffer to the display- we always need this,
 * at the very least for sprite buffering
 */
void x11_buffered_update(s16 x,s16 y,s16 w,s16 h) {
  XCopyArea(xdisplay,((struct x11bitmap*)vid->display)->d,x11_display.d,
	    x11_gctab[PG_LGOP_NONE],x,y,w,h,x,y);
  XFlush(xdisplay);
}

/* Non-buffered update. Only need this if we're not doublebuffering
 */
void x11_nonbuffered_update(s16 x,s16 y,s16 w,s16 h) {
  XFlush(xdisplay);
}

/* Similar to the update function, but triggered by the X server */
void x11_buffered_expose(Region r) {
  /* If we're double-buffered, this is just 
   * x11_buffered_update without the XFlush */

  XSetRegion(xdisplay,x11_gctab[PG_LGOP_NONE],r);
  XCopyArea(xdisplay,((struct x11bitmap*)vid->display)->d,x11_display.d,
	    x11_gctab[PG_LGOP_NONE],0,0,vid->xres,vid->yres,0,0);
  XSetRegion(xdisplay,x11_gctab[PG_LGOP_NONE],display_region);
}

/* We're not so lucky... set all the GCs to clip to this expose region,
 * redraw all the divtree layers, then set the GCs back to normal.
 * Ugly, but avoids an extra buffer.
 */
void x11_nonbuffered_expose(Region r) {
  struct divtree *p;
  int i;
  
  for (i=0;i<=PG_LGOPMAX;i++)
    if (x11_gctab[i])
      XSetRegion(xdisplay,x11_gctab[i],r);

  for (p=dts->top;p;p=p->next)
    p->flags |= DIVTREE_ALL_REDRAW;
  update(NULL,1);

  for (i=0;i<=PG_LGOPMAX;i++)
    if (x11_gctab[i])
      XSetRegion(xdisplay,x11_gctab[i],display_region);
}

g_error x11_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend) {
  g_error e;
  struct x11bitmap *xb = (struct x11bitmap *) bmp;
  s16 w,h;

  if (xb->rend) {
    *rend = xb->rend;
    return success;
  }

  VID(bitmap_getsize)(bmp,&w,&h);

  /* New groprender context for this bitmap */
  e = g_malloc((void **) rend,sizeof(struct groprender));
  errorcheck;
  xb->rend = *rend;
  memset(*rend,0,sizeof(struct groprender));
  (*rend)->lgop = PG_LGOP_NONE;
  (*rend)->output = bmp;
  (*rend)->hfont = defaultfont;
  (*rend)->clip.x2 = w - 1;
  (*rend)->clip.y2 = h - 1;
  (*rend)->orig_clip = (*rend)->clip;
  (*rend)->output_rect.w = w;
  (*rend)->output_rect.h = h;

  return success;
}

g_error x11_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h) {
  struct x11bitmap *xb = (struct x11bitmap *) bmp;
  *w = xb->w;
  *h = xb->h;
  return success;
}

g_error x11_bitmap_new(hwrbitmap *bmp,s16 w,s16 h) {
  struct x11bitmap **pxb = (struct x11bitmap **) bmp;
  g_error e;

  int lw;
  u32 size;

  /* Allocate an x11bitmap structure for this */
  e = g_malloc((void **) pxb,sizeof(struct x11bitmap));
  errorcheck;
  memset(*pxb,0,sizeof(struct x11bitmap));
  (*pxb)->w = w;
  (*pxb)->h = h;

  /* Allocate a corresponding X pixmap */
  (*pxb)->d  = XCreatePixmap(xdisplay,x11_display.d,w,h,vid->bpp);

#ifdef CONFIG_X11_XFT
  /* Allocate an XftDraw */
  (*pxb)->xftd = XftDrawCreate(xdisplay, (Drawable) (*pxb)->d,
			  DefaultVisual(xdisplay, 0),
			  DefaultColormap(xdisplay, 0));
#endif

  return success;
}

void x11_bitmap_free(hwrbitmap bmp) {
  struct x11bitmap *xb = (struct x11bitmap *) bmp;
  if (xb->tile)
    x11_bitmap_free((hwrbitmap) xb->tile);
  if (xb->rend)
    g_free(xb->rend);
#ifdef CONFIG_X11_XFT
  XftDrawDestroy(xb->xftd);
#endif
  XFreePixmap(xdisplay,xb->d);
  g_free(xb);
}

void x11_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop) {
  struct x11bitmap *sxb = (struct x11bitmap *) src;
  struct x11bitmap *dxb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  /* If it's an unsupported LGOP fall back on def_blit.
   */
  if (!g) {
    def_blit(dest,x,y,w,h,src,src_x,src_y,lgop);
    return;
  }

  /* If we're tiling, expand the bitmap to the minimum tile size
   * if necessary, and cache it. Then, if the tile is still smaller
   * than our blit size send it to the default blitter to be
   * decomposed into individual tiles.
   */

  if ((w > sxb->w || h > sxb->h) && 
      (sxb->w < X11_TILESIZE || sxb->h < X11_TILESIZE)) {
    /* Create a pre-tiled image */
    if (!sxb->tile) {
      vid->bitmap_new( (hwrbitmap*)&sxb->tile,
		       (X11_TILESIZE/sxb->w + 1) * sxb->w,
		       (X11_TILESIZE/sxb->h + 1) * sxb->h );
      def_blit((hwrbitmap)sxb->tile,0,0,sxb->tile->w,
	       sxb->tile->h,(hwrbitmap)sxb,0,0,PG_LGOP_NONE);
    }
    sxb = sxb->tile;
  }

  if (w > sxb->w || h > sxb->h)
    def_blit(dest,x,y,w,h,(hwrbitmap) sxb,src_x,src_y,lgop);
  else
    XCopyArea(xdisplay,sxb->d,dxb->d,g,src_x,src_y,w,h,x,y);
}

/* The default sprite code is fine on slow LCDs, or
 * on double-buffered displays. On X11 without double-buffering,
 * the flickering's pretty bad. x11_sprite_update() does a little
 * magic to double-buffer sprite updates.
 */
void x11_buffered_sprite_update(struct sprite *spr) {

  /* Only bother with this if the sprite is big. Small sprites
   * won't flicker anyway, and this code would slow them down
   * dramatically, due to the 2 extra blits.
   *
   * This defines 32x32 as a small sprite, but that's a fudge factor.
   */
  if (spr->x<=32 && spr->y<=32) {
    def_sprite_update(spr);
    return;
  }

  /* What's the relevant portion of the display? Assume that the
   * old and new positions of the sprite are close together. Use
   * add_updarea to calculate us an update region.
   */
  add_updarea(spr->x,spr->y,spr->w,spr->h);
  add_updarea(spr->ox,spr->oy,spr->ow,spr->oh);

  /* Copy the relevant portion of the display to the backbuffer */
  XCopyArea(xdisplay,x11_display.d,x11_backbuffer->d,
  	    x11_gctab[PG_LGOP_NONE],upd_x,upd_y,upd_w,upd_h,upd_x,upd_y);

  /* Normal sprite update, but draw to the backbuffer instead
   * of the display itself.
   */
  vid->display = (hwrbitmap) x11_backbuffer;
  vid->sprite_hide(spr);
  vid->sprite_show(spr);
  
  /* Copy backbuffer to display */
  x11_buffered_update(upd_x,upd_y,upd_w,upd_h);
  vid->display = (hwrbitmap) &x11_display;
}

void x11_message(u32 message, u32 param, u32 *ret) {
  switch (message) {

  case PGDM_SOUNDFX:
    /* XFree86 ignores the volume, it seems */
    if (x11_sound)
      XBell(xdisplay,50);
    break;


    /* Handle cursor warping */
  case PGDM_CURSORWARP: 
    if (x11_autowarp) {
      /* Get the physical coordinates of the new mouse position */
      s16 cx,cy;
      cx = cursor->x;
      cy = cursor->y;
      VID(coord_physicalize)(&cx,&cy);
      XWarpPointer(xdisplay,None,x11_display.d,0,0,0,0,cx,cy);
    }
    break;

  }
}

/******************************************** XFreeType text rendering */
#ifdef CONFIG_X11_XFT

struct font const x11_font = {
   w: 10, 
   h: 10,
   defaultglyph: ' ',
   ascent: 1,
   descent: 0,
   bitmaps: NULL,
   glyphs: NULL,
};

/* Bogus fontstyle node */
struct fontstyle_node x11_font_style = {
   name: "X11 FreeType font",
   size: 1,
   flags: 0,
   next: NULL,
   normal: (struct font *) &x11_font,
   bold: NULL,
   italic: NULL,
   bolditalic: NULL,
   boldw: 0
};

void x11_xft_font_outchar_hook(hwrbitmap *dest, struct fontdesc **fd,
			       s16 *x,s16 *y,hwrcolor *col,int *c,
			       struct quad **clip, s16 *lgop, s16 *angle) {
  s16 ch = (s16) *c;
  XftFont *font = (XftFont *) (*fd)->extra;
  struct x11bitmap *xb = (struct x11bitmap *) *dest;
  XftColor color;
  pgcolor pgc = vid->color_hwrtopg(*col);
  XGlyphInfo xgi;
  
  color.color.red = getred(pgc) << 8;
  color.color.green = getgreen(pgc) << 8;
  color.color.blue = getblue(pgc) << 8;
  color.color.alpha = 0x00FFFF;
  color.pixel = *col;

  XftTextExtents16(xdisplay,font,&ch,1,&xgi);

  /* Draw character- XFreeType measures from the bottom-left, so add height */
  XftDrawString16(xb->xftd,&color,font, *x, *y + xgi.height,&ch, 1);  

  /* Update cursor position */
  *x += 10;
  *y += xgi.yOff;

  /* Disable normal rendering */
  *lgop = PG_LGOP_NULL;
}

void x11_xft_font_sizetext_hook(struct fontdesc *fd, s16 *w, s16 *h, 
				char *txt) {
  XGlyphInfo xgi;
  XftFont *font = (XftFont *) fd->extra;
 
  if (fd->style & PG_FSTYLE_ENCODING_UNICODE)
    XftTextExtentsUtf8(xdisplay,font,txt,strlen(txt),&xgi);
  else
    XftTextExtents8(xdisplay,font,txt,strlen(txt),&xgi);
  
  if (w) *w = xgi.xOff;
  if (h) *h = xgi.height;
}

/* Override outtext to provide proper sub-pixel character spacing */
void x11_xft_font_outtext_hook(hwrbitmap *dest, struct fontdesc **fd,
			       s16 *x,s16 *y,hwrcolor *col,char **txt,
			       struct quad **clip, s16 *lgop, s16 *angle) {
  XftFont *font = (XftFont *) (*fd)->extra;
  struct x11bitmap *xb = (struct x11bitmap *) (*dest); 
  XftColor color;
  pgcolor pgc = vid->color_hwrtopg(*col);
  XGlyphInfo xgi;

  color.color.red = getred(pgc) << 8;
  color.color.green = getgreen(pgc) << 8;
  color.color.blue = getblue(pgc) << 8;
  color.color.alpha = 0x00FFFF;
  color.pixel = *col;

  /* FreeType measures y coordinates relative to the bottom-left */
  if ((*fd)->style & PG_FSTYLE_ENCODING_UNICODE)
    XftTextExtentsUtf8(xdisplay,font,*txt,strlen(*txt),&xgi);
  else
    XftTextExtents8(xdisplay,font,*txt,strlen(*txt),&xgi);
  *y += xgi.height;

  /* Do we have clipping?
   * FIXME: XFreeType clipping is slow
   */
  if (*clip) {
    Region clipreg;
    XRectangle rect;
    
    rect.x = (*clip)->x1;
    rect.y = (*clip)->y1;
    rect.width = (*clip)->x2 - (*clip)->x1 + 1;
    rect.height = (*clip)->y2 - (*clip)->y1 + 1;
    
    clipreg = XCreateRegion();
    XUnionRectWithRegion(&rect,clipreg,clipreg);
    XftDrawSetClip(xb->xftd,clipreg);
    XDestroyRegion(clipreg);  
  }
  else
    XftDrawSetClip(xb->xftd,NULL);

  /* Draw text */
  if ((*fd)->style & PG_FSTYLE_ENCODING_UNICODE)  
    XftDrawStringUtf8(xb->xftd,&color,font,*x,*y,*txt,strlen(*txt));
  else
    XftDrawString8(xb->xftd,&color,font,*x,*y,*txt,strlen(*txt));

  /* Suppress normal outtext behavior */
  *txt = "";
}

void x11_xft_font_newdesc(struct fontdesc *fd, char *name,
			  int size, int flags) {
  XftFont *f;

  /* a little trickery to make the font look legit */
  fd->font = (struct font *) &x11_font;
  fd->italicw = 0;
  fd->fs = &x11_font_style;

  /* Default size */
  if (!size)
    size = 10;

  /* Default family */
  if (!name)
    name = "lucidux";

  /* Allocate an XftFont */
  f = XftFontOpen(xdisplay, 0,
		  XFT_FAMILY, XftTypeString, name,
		  XFT_PIXEL_SIZE, XftTypeInteger, size,
		  XFT_SLANT,XftTypeInteger,(flags & PG_FSTYLE_ITALIC) ?
		      XFT_SLANT_ITALIC : XFT_SLANT_ROMAN,
		  XFT_WEIGHT,XftTypeInteger,(flags & PG_FSTYLE_BOLD) ?
		      XFT_WEIGHT_BOLD : XFT_WEIGHT_LIGHT,
		  0);
  fd->extra = (void *) f;
}

struct fontglyph *x11_xft_font_getglyph(struct fontdesc *fd, int c) {
  static struct fontglyph dummy_fg;
  return &dummy_fg;
}

#endif /* CONFIG_X11_XFT */
/******************************************** Init/shutdown */

g_error x11_init(void) {
  /* Connect to the default X server */
  xdisplay = XOpenDisplay(NULL);
  if (!xdisplay)
    return mkerror(PG_ERRT_IO,46);   /* Error initializing video */

  /* Load the matching input driver */
  return load_inlib(&x11input_regfunc,&inlib_main);
}

/* Create a window */
g_error x11_setmode(s16 xres,s16 yres,s16 bpp,unsigned long flags) {
  int black;
  XEvent ev;
  XTextProperty titleprop;
  char title[80];
  g_error e;
  Visual *xvisual;
  XRectangle rect;

  /* Default resolution is 640x480 
   */
  if (!xres) xres = 640;
  if (!yres) yres = 480;

  /* Create the window
   */
  black = BlackPixel(xdisplay, DefaultScreen(xdisplay));
  x11_display.d = XCreateSimpleWindow(xdisplay, DefaultRootWindow(xdisplay),
				      0, 0, xres, yres, 0, black, black);
  
  /* Map the window, waiting for the MapNotify event 
   */
  XSelectInput(xdisplay, x11_display.d, StructureNotifyMask);  
  XMapWindow(xdisplay,x11_display.d);
  do {
    XNextEvent(xdisplay, &ev);
  } while (ev.type != MapNotify);

  /* Save display information
   */
  xvisual = DefaultVisual(xdisplay,0);
  vid->xres = xres;
  vid->yres = yres;
  vid->bpp  = DefaultDepth(xdisplay,0);
  x11_display.w = xres;
  x11_display.h = yres;

#ifdef CONFIG_X11_XFT
  /* Allocate an XftDraw for the x11_display */
  x11_display.xftd = XftDrawCreate(xdisplay, x11_display.d,
				   DefaultVisual(xdisplay, 0), 
				   DefaultColormap(xdisplay, 0));
#endif

  /* Set up some kind of double-buffering */
  switch (get_param_int("video-x11","doublebuffer",1)) {
    
    /* Fully double-buffered */
  case 1:
    e = vid->bitmap_new((hwrbitmap*) &x11_backbuffer,vid->xres,vid->yres);
    errorcheck;  
    vid->display = (hwrbitmap) x11_backbuffer;
    vid->update = &x11_buffered_update;
    x11_expose = &x11_buffered_expose;
    break;

    /* Only double-buffer sprites */
  case 2:
    e = vid->bitmap_new((hwrbitmap*) &x11_backbuffer,vid->xres,vid->yres);
    errorcheck;  
    vid->display = (hwrbitmap) &x11_display;
    vid->update = &x11_nonbuffered_update;
    x11_expose = &x11_nonbuffered_expose;
    vid->sprite_update = &x11_buffered_sprite_update;
    break;
    
    /* No double-buffer */
  default:
    vid->display = (hwrbitmap) &x11_display;
    vid->update = &x11_nonbuffered_update;
    x11_expose = &x11_nonbuffered_expose;
   break;
  }

  /* Set up our GCs for each supported LGOP */
  /* Set up graphics contexts for each LGOP that X can support directly 
   */
  x11_gctab[PG_LGOP_NONE]       = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_OR]         = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_AND]        = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_XOR]        = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_INVERT]     = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_INVERT_OR]  = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_INVERT_AND] = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_INVERT_XOR] = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_STIPPLE]    = XCreateGC(xdisplay,x11_display.d,0,NULL);

  XSetFunction(xdisplay,x11_gctab[PG_LGOP_OR],        GXor);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_AND],       GXand);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_XOR],       GXxor);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_INVERT],    GXcopyInverted);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_INVERT_OR], GXorInverted);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_INVERT_AND],GXandInverted);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_INVERT_XOR],GXequiv);

  /* Save a region specifying the entire display */
  if (display_region)
    XDestroyRegion(display_region);
  display_region = XCreateRegion();
  rect.x = rect.y = 0;
  rect.width = vid->xres;
  rect.height = vid->yres;
  XUnionRectWithRegion(&rect,display_region,display_region);

  /* Set up a stipple bitmap for PG_LGOP_STIPPLE */
  XSetLineAttributes(xdisplay,x11_gctab[PG_LGOP_STIPPLE],
		     1,LineOnOffDash,CapRound,JoinRound);
  XSetFillStyle(xdisplay,x11_gctab[PG_LGOP_STIPPLE],FillStippled);
  XSetStipple(xdisplay,x11_gctab[PG_LGOP_STIPPLE],
	      XCreateBitmapFromData(xdisplay,x11_display.d,
				    stipple_bits,stipple_width,stipple_height));
  
  /* Save other settings */
  x11_sound = get_param_int("video-x11","sound",1);
  x11_autowarp = get_param_int("input-x11","autowarp",1);

  /* Blank the cursor if we don't want it */
  if (!get_param_int("input-x11","xcursor",1)) {
    XColor black, white;
    Pixmap p;
    Cursor c;
    GC g;
    
    p = XCreatePixmap(xdisplay,x11_display.d,8,8,1);
    g = XCreateGC(xdisplay,p,0,NULL);
    XFillRectangle(xdisplay,p,g,0,0,8,8);
    
    black.red = black.green = black.blue = 0x0000;
    white.red = white.green = white.blue = 0xFFFF;
    
    c = XCreatePixmapCursor(xdisplay,p,p,&black,&white,0,0);
    XDefineCursor(xdisplay, x11_display.d, c);
  }

  /* Set the window title
   */
  sprintf(title,get_param_str("video-x11","caption","PicoGUI (X11@%dx%dx%d)"),
  	  vid->xres,vid->yres,vid->bpp);
  XStoreName(xdisplay, x11_display.d, title);

  /* Set input event mask */
  XSelectInput(xdisplay, x11_display.d,
	       KeyPressMask | KeyReleaseMask | ExposureMask | ButtonMotionMask |
	       ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
  XAutoRepeatOn(xdisplay);

  XFlush(xdisplay);
  return success;
}

void x11_close(void) {
  if (x11_display.rend)
    g_free(x11_display.rend);
  if (x11_backbuffer)
    vid->bitmap_free((hwrbitmap) x11_backbuffer);
  unload_inlib(inlib_main);   /* Take out our input driver */
  XCloseDisplay(xdisplay);
  xdisplay = NULL;
}

/******************************************** Driver registration */

g_error x11_regfunc(struct vidlib *v) {
  setvbl_default(v);
  
  v->init = &x11_init;
  v->setmode = &x11_setmode;
  v->close = &x11_close;
  v->pixel = &x11_pixel;
  v->getpixel = &x11_getpixel;
  v->bitmap_get_groprender = &x11_bitmap_get_groprender;
  v->bitmap_getsize = &x11_bitmap_getsize;
  v->bitmap_new = &x11_bitmap_new;
  v->bitmap_free = &x11_bitmap_free;
  v->rect = &x11_rect;
  v->blit = &x11_blit;
  v->slab = &x11_slab;
  v->bar = &x11_bar;
  v->line = &x11_line;
  v->message = &x11_message;

#ifdef CONFIG_X11_XFT
  v->font_newdesc = &x11_xft_font_newdesc;
  v->font_sizetext_hook = &x11_xft_font_sizetext_hook;
  v->font_outtext_hook = &x11_xft_font_outtext_hook;
  v->font_outchar_hook = &x11_xft_font_outchar_hook;
  v->font_getglyph = &x11_xft_font_getglyph;
#endif

  if (!get_param_int("video-x11","defaultellipse",0)) {
    v->ellipse = &x11_ellipse;
    v->fellipse = &x11_fellipse;
  }

  return success;
}

/* The End */
