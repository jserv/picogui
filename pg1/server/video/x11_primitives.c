/* $Id$
 *
 * x11_primitives.c - Implementation of picogui primitives on top of the
 *                    X window system.
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
  if (use_shm1(src))
    return XB(src)->lib->getpixel(&XB(src)->sb,x,y);

  /* We could use XGetImage to get the pixel value,
   * but that is too horrible to speak of...
   * Since we should never have to use this, just return
   * bright red so we know something's wrong.
   */
  return VID(color_pgtohwr)(0xFF0000);
}

void x11_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    if (use_shm1(dest))
      XB(dest)->lib->rect(&XB(dest)->sb, x,y,w,h,c,lgop);
    else
      def_rect(dest,x,y,w,h,c,lgop);
    return;
  }
  
  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XFillRectangle(x11_display,XB(dest)->d,g,x,y,w,h);
}

void x11_line(hwrbitmap dest, s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    if (use_shm1(dest))
      XB(dest)->lib->line(&XB(dest)->sb, x1,y1,x2,y2,c,lgop);
    else
      def_line(dest,x1,y1,x2,y2,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XDrawLine(x11_display,XB(dest)->d,g,x1,y1,x2,y2);
}

void x11_slab(hwrbitmap dest, s16 x,s16 y,s16 w, hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    if (use_shm1(dest))
      XB(dest)->lib->slab(&XB(dest)->sb, x,y,w,c,lgop);
    else
      def_slab(dest,x,y,w,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XFillRectangle(x11_display,XB(dest)->d,g,x,y,w,1);
}

void x11_bar(hwrbitmap dest, s16 x,s16 y,s16 h, hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    if (use_shm1(dest))
      XB(dest)->lib->bar(&XB(dest)->sb, x,y,h,c,lgop);
    else
      def_bar(dest,x,y,h,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XFillRectangle(x11_display,XB(dest)->d,g,x,y,1,h);
}

void x11_ellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    if (use_shm1(dest))
      XB(dest)->lib->ellipse(&XB(dest)->sb, x,y,w,h,c,lgop);
    else
      def_ellipse(dest,x,y,w,h,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XDrawArc(x11_display,XB(dest)->d,g,x,y,w-1,h-1,0,360*64);
}

void x11_fellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    if (use_shm1(dest))
      XB(dest)->lib->fellipse(&XB(dest)->sb, x,y,w,h,c,lgop);
    else
      def_fellipse(dest,x,y,w,h,c,lgop);
    return;
  }

  set_shm1(dest,0);
  XSetForeground(x11_display,g,c);
  XFillArc(x11_display,XB(dest)->d,g,x,y,w-1,h-1,0,360*64);
}

void x11_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop) {
  GC g = x11_gctab[lgop];

  if (!g) {
    if (use_shm2(src,dest))
      XB(dest)->lib->blit(&XB(dest)->sb, x,y,w,h, &XB(src)->sb, src_x,src_y,lgop);
    else
      def_blit(dest,x,y,w,h,src,src_x,src_y,lgop);
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
  s16 i,j;
  int blit_x, blit_y, blit_w, blit_h, blit_src_x, blit_src_y;
  int full_line_y = -1;

  if (!(sw && sh)) return;

  if (!use_shm2(src,dest)) {
    def_multiblit(dest,x,y,w,h,src,sx,sy,sw,sh,xo,yo,lgop);
    return;
  }

  /* Split the tiled blit up into individual blits clipped against the destination.
   * We do y clipping once per line, since only x coordinates change in the inner loop
   */
  
  for (j=-yo;j<h;j+=sh) {
    blit_y = y+j;
    blit_h = sh;
    blit_src_y = sy;
    if (j<0) {
      blit_y = y;
      blit_h += j;
      blit_src_y -= j;
    }
    if (blit_y + blit_h > y + h)
      blit_h = y + h - blit_y;
    
    if (lgop == PG_LGOP_NONE && full_line_y >= 0 && blit_h == sh) {
      /* If this blit isn't blended, this line is full-height, and there's already been
       * one full-height line drawn, we can copy that line instead of drawing a new one.
       * This can reduce the total number of blits from approximately (w/sw)*(h/sw) to (w/sw)+(h/sw)
       */
      
      XB(dest)->lib->blit(&XB(dest)->sb,x,blit_y,w,blit_h,&XB(dest)->sb,x,full_line_y,lgop);
    }
    else {
      /* Draw the line normally */
      
      for (i=-xo;i<w;i+=sw) {
	blit_x = x+i;
	blit_w = sw;
	blit_src_x = sx;
	if (i<0) {
	  blit_x = x;
	  blit_w += i;
	  blit_src_x -= i;
	}
	if (blit_x + blit_w > x + w)
	  blit_w = x + w - blit_x;
	
	XB(dest)->lib->blit(&XB(dest)->sb,blit_x,blit_y,blit_w,blit_h,&XB(src)->sb,blit_src_x,blit_src_y,lgop);
      }
      if (blit_h == sh)
	full_line_y = blit_y;
    }
  }
}

void x11_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
		  s16 lines, s16 angle, hwrcolor c, struct pgquad *clip,
		  s16 lgop, int char_pitch) {
  if (use_shm1(dest))
    XB(dest)->lib->charblit(&XB(dest)->sb,chardat,x,y,w,h,lines,angle,c,clip,lgop,char_pitch);
  else
    def_charblit(dest,chardat,x,y,w,h,lines,angle,c,clip,lgop,char_pitch);
}

#ifdef CONFIG_FONTENGINE_FREETYPE
void x11_alpha_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
			int char_pitch, u8 *gammatable, s16 angle, hwrcolor c,
			struct pgquad *clip, s16 lgop) {
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

  if (!g) {
    if (use_shm1(dest))
      XB(dest)->lib->fpolygon(&XB(dest)->sb, array,xoff,yoff,c,lgop);
    else
      def_fpolygon(dest,array,xoff,yoff,c,lgop);
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
		    struct pgquad *clip, s16 angle, s16 lgop) {
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

  if (!g) {
    if (use_shm2(src,dest))
      XB(dest)->lib->scrollblit(&XB(dest)->sb, x,y,w,h, &XB(src)->sb, src_x,src_y,lgop);
    else
      def_scrollblit(dest,x,y,w,h,src,src_x,src_y,lgop);
    return;
  }

  set_shm2(src,dest,0);
  XCopyArea(x11_display,XB(src)->d,XB(dest)->d,g,src_x,src_y,w,h,x,y);
}

/* The End */
