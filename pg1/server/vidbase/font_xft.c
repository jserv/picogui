/* $Id$
 *
 * font_xft.c - Font engine for X implemented using Xft
 *
 *          This driver is still INCOMPLETE and EXPERIMENTAL!
 *          There's probably no need to finsih this driver any
 *          time soon, as the freetype font engine looks and works better.
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
#include <pgserver/font.h>
#include <pgserver/x11.h>
#include <pgserver/video.h>

XftDraw *xft_draw;

struct xft_fontdesc {
  XftFont *f;
  int flags;
};
#define DATA ((struct xft_fontdesc *)self->data)

/* Utility function to do the setup work before Xft rendering */
void xft_draw_setup(hwrbitmap dest, hwrcolor col, struct pgquad *clip, XftColor *xftc);

void xft_measure_char(struct font_descriptor *self, struct pgpair *position,
		      int ch, s16 angle);
void xft_measure_string(struct font_descriptor *fd, const struct pgstring *str,
			s16 angle, s16 *w, s16 *h);
void xft_getmetrics(struct font_descriptor *self, struct font_metrics *m);


/********************************** Implementations ***/

g_error xft_engine_init(void) {
  if (!x11_display)
    return mkerror(PG_ERRT_BADPARAM, 146);  /* Requires X */

  /* We use just one global Xft drawable, since all our
   * drawables will use the default pixel format and visual.
   */
  xft_draw = XftDrawCreate(x11_display,
			   RootWindow(x11_display, x11_screen),
			   DefaultVisual(x11_display, x11_screen), 
			   DefaultColormap(x11_display, x11_screen));
  return success;
}

void xft_engine_shutdown(void) {
  XftDrawDestroy(xft_draw);
}

void xft_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pgpair *position,
		   hwrcolor col, int ch, struct pgquad *clip, s16 lgop, s16 angle) {
  u32 ch32 = ch;
  XftColor xftc;

  xft_draw_setup(dest,col,clip,&xftc);

  XftDrawString32(xft_draw, &xftc, DATA->f, position->x,
		  position->y + DATA->f->ascent, &ch32, 1);

  xft_measure_char(self,position,ch,angle);
}

void xft_measure_char(struct font_descriptor *self, struct pgpair *position,
		      int ch, s16 angle) {
  XGlyphInfo xgi;
  u32 ch32 = ch;
  XftTextExtents32(x11_display,DATA->f,&ch32,1,&xgi);
  position->x += xgi.xOff;
  position->y += xgi.yOff;
}

g_error xft_create(struct font_descriptor *self, const struct font_style *fs) {
  g_error e;

  e = g_malloc((void**)&self->data, sizeof(struct xft_fontdesc));
  errorcheck;
  memset(self->data,0,sizeof(struct xft_fontdesc));

  //  DATA->flags = fs->style;

  /* FIXME: do this right */
  DATA->f = XftFontOpenName(x11_display, x11_screen, "arial-12");

  return success;
}

void xft_destroy(struct font_descriptor *self) {
  XftFontClose(x11_display, DATA->f);
  g_free(self->data);
}
 
void xft_getstyle(int i, struct font_style *fs) {
}

void xft_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
  m->charcell.w = DATA->f->max_advance_width;
  m->charcell.h = DATA->f->ascent + DATA->f->descent;
  m->ascent = DATA->f->ascent;
  m->descent = DATA->f->descent;
  m->lineheight = DATA->f->height;
  m->linegap = m->lineheight - m->ascent - m->descent;

  if (DATA->flags & PG_FSTYLE_FLUSH)
    m->margin = 0;
  else
    m->margin = m->descent;
}


/********************************** Internal utilities ***/

/* Utility function to do the setup work before Xft rendering */
void xft_draw_setup(hwrbitmap dest, hwrcolor col, struct pgquad *clip, XftColor *xftc) {
  Region r;
  XRectangle rect;
  pgcolor pgc;

  XftDrawChange(xft_draw, XB(dest)->d);

  /* Set up the clip rectangle to the intercection of clip and our
   * global clipping rectangle (needed to handle expose events right)
   */
  r = XCreateRegion();
  rect.x = clip->x1;
  rect.y = clip->y1;
  rect.width = clip->x2 - clip->x1 + 1;
  rect.height = clip->y2 - clip->y1 + 1;
  XUnionRectWithRegion(&rect,r,r);
  XIntersectRegion(x11_current_region,r,r);
  XftDrawSetClip(xft_draw,r);
  XDestroyRegion(r);

  /* This assumes we know what we're doing, but we do, right? :) */
  pgc = VID(color_hwrtopg)(col);
  xftc->color.red   = getred(pgc) << 8;
  xftc->color.green = getgreen(pgc) << 8;
  xftc->color.blue  = getblue(pgc)  << 8;
  xftc->color.alpha = 0xFFFF;
  xftc->pixel = col;
}


/********************************** Registration ***/

g_error xft_regfunc(struct fontlib *f) {
  f->engine_init = &xft_engine_init;
  f->engine_shutdown = &xft_engine_shutdown;
  f->draw_char = &xft_draw_char;
  f->measure_char = &xft_measure_char;
  f->create = &xft_create;
  f->destroy = &xft_destroy;
  f->getstyle = &xft_getstyle;
  f->getmetrics = &xft_getmetrics;
  return success;
}

/* The End */

