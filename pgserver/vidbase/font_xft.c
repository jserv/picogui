/* $Id: font_xft.c,v 1.3 2002/11/06 02:07:13 micahjd Exp $
 *
 * font_xft.c - Font engine for X implemented using Xft
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
void xft_draw_setup(hwrbitmap dest, hwrcolor col, struct quad *clip, XftColor *xftc);

void xft_measure_char(struct font_descriptor *self, struct pair *position,
		      int ch, s16 angle);
void xft_measure_string(struct font_descriptor *fd, const struct pgstring *str,
			s16 angle, s16 *w, s16 *h);
void xft_getmetrics(struct font_descriptor *self, struct font_metrics *m);


/********************************** Implementations ***/

g_error xft_engine_init(void) {
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

void xft_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
		   hwrcolor col, int ch, struct quad *clip, s16 lgop, s16 angle) {
  u32 ch32 = ch;
  XftColor xftc;

  xft_draw_setup(dest,col,clip,&xftc);

  XftDrawString32(xft_draw, &xftc, DATA->f, position->x,
		  position->y + DATA->f->ascent, &ch32, 1);

  xft_measure_char(self,position,ch,angle);
}

void xft_draw_string(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
		     hwrcolor col, const struct pgstring *str, struct quad *clip,
		     s16 lgop, s16 angle) {
  XftColor xftc;
  s16 w,h;
  struct font_metrics m;

  switch (str->flags & PGSTR_ENCODE_MASK) {
  case PGSTR_ENCODE_ASCII:
  case PGSTR_ENCODE_UTF8:
    //    break;
  default:
    def_draw_string(self,dest,position,col,str,clip,lgop,angle);
    return;
  }
  
  xft_draw_setup(dest,col,clip,&xftc);
  
  /*
  xft_getmetrics(self,&m);
  position->x += m.margin;
  position->y += m.margin;
  */

  XftDrawStringUtf8(xft_draw, &xftc, DATA->f, position->x,
		    position->y + DATA->f->ascent, str->buffer, str->num_chars);
}

void xft_measure_string(struct font_descriptor *self, const struct pgstring *str,
			s16 angle, s16 *w, s16 *h) {
  XGlyphInfo xgi;

  switch (str->flags & PGSTR_ENCODE_MASK) {
  case PGSTR_ENCODE_ASCII:
  case PGSTR_ENCODE_UTF8:
    break;
  default:
    def_measure_string(self,str,angle,w,h);
    return;
  }

  XftTextExtentsUtf8(x11_display,DATA->f,str->buffer,str->num_chars,&xgi);
  *w = xgi.xOff;
  *h = xgi.yOff;  
}

void xft_measure_char(struct font_descriptor *self, struct pair *position,
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
  DATA->f = XftFontOpenName(x11_display, x11_screen, "foo-15");

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
void xft_draw_setup(hwrbitmap dest, hwrcolor col, struct quad *clip, XftColor *xftc) {
  Region r;
  XRectangle rect;
  pgcolor pgc;

  XftDrawChange(xft_draw, XB(dest)->d);

  /* Yucky way of setting the clip rectangle... 
   * this is messy and probably slow 
   */
  r = XCreateRegion();
  rect.x = clip->x1;
  rect.y = clip->y1;
  rect.width = clip->x2 - clip->x1 + 1;
  rect.height = clip->y2 - clip->y1 + 1;
  XUnionRectWithRegion(&rect,r,r);
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
  f->draw_string = &xft_draw_string;
  f->measure_string = &xft_measure_string;
  return success;
}

/* The End */

