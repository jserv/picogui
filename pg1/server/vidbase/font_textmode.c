/* $Id$
 *
 * font_textmode.c - Font rendering in text mode, for ncurses and LCD drivers
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

void textmode_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pgpair *position,
			hwrcolor col, int ch, struct pgquad *clip, s16 lgop, s16 angle);
void textmode_measure_char(struct font_descriptor *self, struct pgpair *position,
			   int ch, s16 angle);
g_error textmode_create(struct font_descriptor *self, const struct font_style *fs);
void textmode_destroy(struct font_descriptor *self);
void textmode_getstyle(int i, struct font_style *fs);
void textmode_getmetrics(struct font_descriptor *self, struct font_metrics *m);
hwrcolor textmode_rgb_to_colorcode(hwrcolor c);

/********************************** Implementations ***/

void textmode_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pgpair *position,
		   hwrcolor col, int ch, struct pgquad *clip, s16 lgop, s16 angle) {
  hwrcolor oc, c;

  /* Make sure we're within clip */
  if (clip && (position->x<clip->x1 || position->y<clip->y1 ||
	       position->x>clip->x2 || position->y>clip->y2))
    return;

  /* Generate a pixel in PGCF_TEXT_ASCII format, containing the existing background color, 
   * our new foreground color, and the new character. This is desined for the ncurses driver,
   * but should also work on other text mode devices.
   */

  oc = VID(getpixel)(dest,position->x,position->y);
  c = PGCF_TEXT_ASCII | ch;

  /* Add in background color */
  if (oc & (PGCF_TEXT_ASCII | PGCF_TEXT_ACS))
    c |= oc & 0x00FF0000;
  else
    c |= textmode_rgb_to_colorcode(oc) << 16;

  /* Foreground color */
  c |= textmode_rgb_to_colorcode(col) << 8;

  VID(pixel)(dest,position->x,position->y,c,lgop);
  textmode_measure_char(self,position,ch,angle);
}

void textmode_measure_char(struct font_descriptor *self, struct pgpair *position,
		      int ch, s16 angle) {
  switch (angle) {
  case 0:
    position->x++;
    break;
  case 90:
    position->y--;
    break;
  case 180:
    position->x--;
    break;
  case 270:
    position->y++;
    break;
  }
}

/* There's only one font, no need for extra info in the descriptor */
g_error textmode_create(struct font_descriptor *self, const struct font_style *fs) {
  return success;
}
void textmode_destroy(struct font_descriptor *self) {
}
 
void textmode_getstyle(int i, struct font_style *fs) {
  memset(fs,0,sizeof(*fs));
  
  /* There's only one font */
  if (!i) {
    fs->name = "Text mode font";
    fs->size = 1;
    fs->style = PG_FSTYLE_FIXED;
  }
}

void textmode_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
  m->charcell.w = 1;
  m->charcell.h = 1;
  m->ascent = 1;
  m->descent = 0;
  m->margin = 0;
  m->linegap = 0;
  m->lineheight = 1;
}

/* Convert from RGB to the color codes used by the ncurses driver */
hwrcolor textmode_rgb_to_colorcode(hwrcolor c) {
  hwrcolor cc;
  if ((c & 0xFF0000) > 0x400000) cc |= 4;
  if ((c & 0x00FF00) > 0x004000) cc |= 2;
  if ((c & 0x0000FF) > 0x000040) cc |= 1;
  if (((c&0xFF0000) > 0xA00000) || 
      ((c&0x00FF00) > 0x00A000) || 
      ((c&0x0000FF) > 0x0000A0))
    cc |= 8;
  return cc;
}

/********************************** Registration ***/

g_error textmode_regfunc(struct fontlib *f) {
  f->draw_char = &textmode_draw_char;
  f->measure_char = &textmode_measure_char;
  f->create = &textmode_create;
  f->destroy = &textmode_destroy;
  f->getstyle = &textmode_getstyle;
  f->getmetrics = &textmode_getmetrics;
  return success;
}

/* The End */

