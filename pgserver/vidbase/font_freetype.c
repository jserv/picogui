/* $Id: font_freetype.c,v 1.1 2002/10/12 14:46:35 micahjd Exp $
 *
 * font_freetype.c - Font engine that uses Freetype2 to render
 *                   spiffy antialiased Type1 and TrueType fonts
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

/********************************** Implementations ***/

void freetype_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
		   hwrcolor col, int ch, struct quad *clip, s16 lgop, s16 angle) {
}

void freetype_measure_char(struct font_descriptor *self, struct pair *position,
		      int ch, s16 angle) {
}

g_error freetype_create(struct font_descriptor *self, const struct font_style *fs) {
  return success;
}

void freetype_destroy(struct font_descriptor *self) {
}
 
void freetype_getstyle(int i, struct font_style *fs) {
}

void freetype_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
}

void freetype_measure_string(struct font_descriptor *self, const struct pgstring *str,
			s16 angle, s16 *w, s16 *h) {
}


/********************************** Registration ***/

g_error freetype_regfunc(struct fontlib *f) {
  f->draw_char = &freetype_draw_char;
  f->measure_char = &freetype_measure_char;
  f->create = &freetype_create;
  f->destroy = &freetype_destroy;
  f->getstyle = &freetype_getstyle;
  f->getmetrics = &freetype_getmetrics;
  f->measure_string = &freetype_measure_string;
  return success;
}

/* The End */

