/* $Id: x11_primitives.c,v 1.12 2002/11/07 09:41:33 micahjd Exp $
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


/******************************************************** Utilities */

/* We can optionally profile how often this has to synchronize.
 * Note that XSHM_PROFILE can't be an inline function, since
 * inlines won't inline if they are nested.
 */
#ifdef CONFIG_X11_SHM_PROFILE
int xshm_sync, xshm_total;
#define XSHM_PROFILE \
  xshm_total++; \
  if (xshm_total==50000) { \
    printf("X11 SHM Profile: %d sync / %d total (%d.%02d%%)\n", \
      xshm_sync, xshm_total, xshm_sync*100/xshm_total, \
      (xshm_sync*10000/xshm_total)%100); \
    xshm_total = xshm_sync = 0; \
  }
#endif


/* Turn on or off SHM rendering, synchronizing if needed.
 * Versions that work on one or two bitmaps at a time are provided.
 * We only need to sync if we've been drawing using X primitives
 * and we're about to use our own via SHM.
 */
static inline void set_shm1(hwrbitmap a, int shmflag) {
  if(shmflag && !XB(a)->using_shm) {
    XSync(x11_display,False);
#ifdef CONFIG_X11_SHM_PROFILE
    xshm_sync++;
#endif
  }
#ifdef CONFIG_X11_SHM_PROFILE
  XSHM_PROFILE;
#endif
  XB(a)->using_shm = shmflag;
}
static inline void set_shm2(hwrbitmap a, hwrbitmap b, int shmflag) {
  if(shmflag && ((!XB(b)->using_shm) || (!XB(a)->using_shm))) {
    XSync(x11_display,False);
#ifdef CONFIG_X11_SHM_PROFILE
    xshm_sync++;
#endif
  }
#ifdef CONFIG_X11_SHM_PROFILE
  XSHM_PROFILE;
#endif
  XB(a)->using_shm = shmflag;
  XB(b)->using_shm = shmflag;
}

/* For functions that prefer SHM, this returns 1 if it's possible */
static inline int use_shm1(hwrbitmap a) {
  if (!XB(a)->lib) {
    set_shm1(a, 0);
    return 0;
  }
  set_shm1(a, 1);
  return 1;
}
static inline int use_shm2(hwrbitmap a, hwrbitmap b) {
  if (!(XB(a)->lib && XB(b)->lib)) {
    set_shm2(a,b,0);
    return 0;
  }
  set_shm2(a,b,1);
  return 1;
}


/******************************************************** Primitives */

void x11_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  GC g;
  
  /* Always use SHM if we can */
  if (use_shm1(dest)) {
    XB(dest)->lib->pixel(&XB(dest)->sb,x,y,c,lgop);
    return;
  }

  g = x11_gctab[lgop];
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

  if (use_shm1(src))
    return XB(src)->lib->getpixel(&XB(src)->sb,x,y);

#ifdef CONFIG_X11_NOPIXEL
  return VID(color_pgtohwr)(0xFF0000);
#else
  /* Terribly slow method of getting a pixel...
   * Hopefully we'll be using the SHM method instead.
   */
  img = XGetImage (x11_display, XB(src)->d, x, y, 1, 1, AllPlanes, ZPixmap);
  c = XGetPixel(img,0,0);
  XDestroyImage(img);
  return c;
#endif
}

void x11_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g && use_shm1(dest)) {
    XB(dest)->lib->rect(&XB(dest)->sb, x,y,w,h,c,lgop);
    return;
  }
  
  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XFillRectangle(x11_display,XB(dest)->d,g,x,y,w,h);
}

void x11_line(hwrbitmap dest, s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g && use_shm1(dest)) {
    XB(dest)->lib->line(&XB(dest)->sb, x1,y1,x2,y2,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XDrawLine(x11_display,XB(dest)->d,g,x1,y1,x2,y2);
}

void x11_slab(hwrbitmap dest, s16 x,s16 y,s16 w, hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g && use_shm1(dest)) {
    XB(dest)->lib->slab(&XB(dest)->sb, x,y,w,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XFillRectangle(x11_display,XB(dest)->d,g,x,y,w,1);
}

void x11_bar(hwrbitmap dest, s16 x,s16 y,s16 h, hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g && use_shm1(dest)) {
    XB(dest)->lib->bar(&XB(dest)->sb, x,y,h,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XFillRectangle(x11_display,XB(dest)->d,g,x,y,1,h);
}

void x11_ellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g && use_shm1(dest)) {
    XB(dest)->lib->ellipse(&XB(dest)->sb, x,y,w,h,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XDrawArc(x11_display,XB(dest)->d,g,x,y,w-1,h-1,0,360*64);
}

void x11_fellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g && use_shm1(dest)) {
    XB(dest)->lib->fellipse(&XB(dest)->sb, x,y,w,h,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XFillArc(x11_display,XB(dest)->d,g,x,y,w-1,h-1,0,360*64);
}

void x11_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g && use_shm2(src,dest)) {
    XB(dest)->lib->blit(&XB(dest)->sb, x,y,w,h, &XB(src)->sb, src_x,src_y,lgop);
    return;
  }

  set_shm2(src,dest,0);
  XCopyArea(x11_display,XB(src)->d,XB(dest)->d,g,src_x,src_y,w,h,x,y);
}

void x11_update(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h) {
  /* Double-buffered? */
  if (XB(dest)->frontbuffer)
    XCopyArea(x11_display, XB(dest)->d, XB(XB(dest)->frontbuffer)->d,
	      x11_gctab[PG_LGOP_NONE],x,y,w,h,x,y);
}

void x11_multiblit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		   hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 xo, s16 yo, s16 lgop) {
  GC g;
  
  /* Use the default multiblit if:
   *   1. The source isn't the entire bitmap. X can't handle this case
   *   2. We're using stippling. This needs to set the X fill style too, so there
   *      would be a conflict and the stipple GC would be reset incorrectly.
   *   3. The destination rectangle isn't larger than the source. In this case the
   *      extra setup work here isn't worth it.
   */
  if (sx!=0 || sy!=0 || sw!=XB(src)->sb.w || sh!=XB(src)->sb.h || 
      lgop==PG_LGOP_STIPPLE || (w<=sw && h<=sh)) {
    def_multiblit(dest,x,y,w,h,src,sx,sy,sw,sh,xo,yo,lgop);
    return;
  }

  /* Now if we got this far, this is probably a large background area that's
   * tiled with a complete pixmap, so it will be efficiently handled by X (we hope)
   */
  set_shm1(dest,0);
  g = x11_gctab[lgop];
  XSetTile(x11_display,g,XB(src)->d);
  XSetTSOrigin(x11_display,g,x-xo,y-yo);
  XSetFillStyle(x11_display,g,FillTiled);
  x11_rect(dest,x,y,w,h,0,lgop);
  XSetFillStyle(x11_display,g,FillSolid);
}

void x11_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
		  s16 lines, s16 angle, hwrcolor c, struct quad *clip,
		  s16 lgop, int char_pitch) {
  if (use_shm1(dest))
    XB(dest)->lib->charblit(&XB(dest)->sb,chardat,x,y,w,h,lines,angle,c,clip,lgop,char_pitch);
  else
    def_charblit(dest,chardat,x,y,w,h,lines,angle,c,clip,lgop,char_pitch);
}

#ifdef CONFIG_FONTENGINE_FREETYPE
void x11_alpha_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
			int char_pitch, u8 *gammatable, s16 angle, hwrcolor c,
			struct quad *clip, s16 lgop) {
  if (use_shm1(dest))
    XB(dest)->lib->alpha_charblit(&XB(dest)->sb,chardat,x,y,w,h,char_pitch,gammatable,
				  angle,c,clip,lgop);
  else
    def_alpha_charblit(dest,chardat,x,y,w,h,char_pitch,gammatable,angle,c,clip,lgop);
}
#endif

void x11_blur(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, s16 radius) {
  if (use_shm1(dest))
    XB(dest)->lib->blur(&XB(dest)->sb,x,y,w,h,radius);
  else
    def_blur(dest,x,y,w,h,radius);
}

void x11_fpolygon(hwrbitmap dest, s32* array, s16 xoff, s16 yoff , hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];
  XPoint *points;
  int npoints;
  int i;

  if (!g && use_shm1(dest)) {
    XB(dest)->lib->fpolygon(&XB(dest)->sb, array,xoff,yoff,c,lgop);
    return;
  }

  /* Convert our array into an XPoint array */
  npoints = (*(array++)) >> 1;
  points = alloca(sizeof(XPoint)*npoints);
  for (i=0;i<npoints;i++) {
    points[i].x = *(array++);
    points[i].y = *(array++);
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XFillPolygon(x11_display, XB(dest)->d, g, points, npoints, Complex, CoordModeOrigin);
}

void x11_rotateblit(hwrbitmap dest, s16 dest_x, s16 dest_y,
		    hwrbitmap src, s16 src_x, s16 src_y, s16 src_w, s16 src_h,
		    struct quad *clip, s16 angle, s16 lgop) {
  if (use_shm2(dest,src))
    XB(dest)->lib->rotateblit(&XB(dest)->sb,dest_x,dest_y,&XB(src)->sb,src_x,src_y,
			      src_w,src_h,clip,angle,lgop);
  else
    def_rotateblit(dest,dest_x,dest_y,src,src_x,src_y,src_w,src_h,clip,angle,lgop);
}

void x11_gradient(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,s16 angle,
		  pgcolor c1, pgcolor c2, s16 lgop) {
  if (use_shm1(dest))
    XB(dest)->lib->gradient(&XB(dest)->sb,x,y,w,h,angle,c1,c2,lgop);
  else
    def_gradient(dest,x,y,w,h,angle,c1,c2,lgop);
}

void x11_scrollblit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		    s16 src_x, s16 src_y, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g && use_shm2(src,dest)) {
    XB(dest)->lib->scrollblit(&XB(dest)->sb, x,y,w,h, &XB(src)->sb, src_x,src_y,lgop);
    return;
  }

  set_shm2(src,dest,0);
  XCopyArea(x11_display,XB(src)->d,XB(dest)->d,g,src_x,src_y,w,h,x,y);
}

/* The End */
