/* $Id: font_xft.c,v 1.1 2002/11/05 21:30:31 micahjd Exp $
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

void xft_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
			hwrcolor col, int ch, struct quad *clip, s16 lgop, s16 angle);
void xft_measure_char(struct font_descriptor *self, struct pair *position,
			   int ch, s16 angle);
g_error xft_create(struct font_descriptor *self, const struct font_style *fs);
void xft_destroy(struct font_descriptor *self);
void xft_getstyle(int i, struct font_style *fs);
void xft_getmetrics(struct font_descriptor *self, struct font_metrics *m);


/********************************** Implementations ***/

void xft_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
		   hwrcolor col, int ch, struct quad *clip, s16 lgop, s16 angle) {
}

void xft_measure_char(struct font_descriptor *self, struct pair *position,
		      int ch, s16 angle) {
}

g_error xft_create(struct font_descriptor *self, const struct font_style *fs) {
  return success;
}
void xft_destroy(struct font_descriptor *self) {
}
 
void xft_getstyle(int i, struct font_style *fs) {
}

void xft_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
  m->charcell.w = 1;
  m->charcell.h = 1;
  m->ascent = 1;
  m->descent = 0;
  m->margin = 0;
  m->linegap = 0;
  m->lineheight = 1;
}


/********************************** Registration ***/

g_error xft_regfunc(struct fontlib *f) {
  f->draw_char = &xft_draw_char;
  f->measure_char = &xft_measure_char;
  f->create = &xft_create;
  f->destroy = &xft_destroy;
  f->getstyle = &xft_getstyle;
  f->getmetrics = &xft_getmetrics;
  return success;
}

/* The End */

