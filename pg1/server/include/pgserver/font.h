/* $Id$
 *
 * font.h - Common structures for defining fonts, and an interface
 *          for specific font engines to attach to
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

#ifndef __H_FONT
#define __H_FONT

#include <pgserver/video.h>

/* Weights used for font comparison */
#define FCMP_TYPE     (1<<14)
#define FCMP_CHARSET  (1<<13)
#define FCMP_FIXEDVAR (1<<12)
#define FCMP_SIZE(x)  ((0xFF-(x&0xFF))<<3)   /* This macro is passed the
						difference in size between the
						request and the actual font */
#define FCMP_NAME     (1<<2)
#define FCMP_DEFAULT  (1<<1)
#define FCMP_STYLE    (1<<0)

/* Font descriptor- engine-specific data, and the engine used */
struct font_descriptor {
  struct fontlib *lib;
  void *data;
};

/* Return structure for font_getstyle */
struct font_style {
  const char *name;
  u32 style, representation;
  int size;
};
 
/* Return structure for font_getmetrics */
struct font_metrics {
  struct sizepair charcell;
  int ascent,descent;
  int margin;
  int linegap;
  int lineheight;   /* = ascent + descent + linegap */
};

/* This should be extensible to support multiple font engines simultaneously
 * if we need to. Right now one has to be chosen at startup.
 */
g_error font_init(void);
void font_shutdown(void);

/* Create a new font using the default font engine loaded in font_init().
 * If style is NULL, get a default font.
 */
g_error font_descriptor_create(struct font_descriptor **font, const struct font_style *style);
void font_descriptor_destroy(struct font_descriptor *font);

/* Get a font style in any installed font engine */
void font_getstyle(int i, struct font_style *fs);

/* List of installed font engines, terminated with {NULL,NULL} */
struct fontengine {
  const char *name;
  g_error (*reg)(struct fontlib *f);
};
extern struct fontengine fontengine_list[];
g_error font_find_engine(struct fontengine **fe, const char *name);


/********************************** Default implementations ***/

void def_draw_string(struct font_descriptor *fd, hwrbitmap dest, struct pgpair *position,
		     hwrcolor col, const struct pgstring *str, struct pgquad *clip,
		     s16 lgop, s16 angle);

void def_measure_string(struct font_descriptor *fd, const struct pgstring *str,
			s16 angle, s16 *w, s16 *h);


/********************************** Registration functions ***/

g_error bdf_regfunc(struct fontlib *f);
g_error freetype_regfunc(struct fontlib *f);
g_error textmode_regfunc(struct fontlib *f);
g_error xft_regfunc(struct fontlib *f);
g_error ftgl_regfunc(struct fontlib *f);


/********************************** Font engine interface ***/

struct fontlib {
  g_error (*engine_init)(void);
  void (*engine_shutdown)(void);

  /*   Draw a single character to the screen at the given position
   *   and angle, into a clipping rectangle. The position is mesured
   *   at the top-left of the character cel. The font is in a
   *   driver-defined format.
   */
  void (*draw_char)(struct font_descriptor *fd, hwrbitmap dest, struct pgpair *position,
		    hwrcolor col, int ch, struct pgquad *clip, s16 lgop, s16 angle);
  
  /*   Measure the size of a character. This is defined to be a function that
   *   has the same side effect on 'position' as draw_char without
   *   actually drawing anything.
   */
  void (*measure_char)(struct font_descriptor *fd, struct pgpair *position,
		       int ch, s16 angle);
  
  /*   Draw a string to the screen at the given position
   *   and angle, into a clipping rectangle. The position is mesured
   *   at the top-left of the first character cel. The font is in a
   *   driver-defined format.
   */
  void (*draw_string)(struct font_descriptor *fd, hwrbitmap dest, struct pgpair *position,
		      hwrcolor col, const struct pgstring *str, struct pgquad *clip,
		      s16 lgop, s16 angle);
  
  /*   Measure the size of a string.
   */
  void (*measure_string)(struct font_descriptor *fd, const struct pgstring *str,
			 s16 angle, s16 *w, s16 *h);
  
  /*   Create a font descriptor for the font that's the closest match for the
   *   given font style.
   */
  g_error (*create)(struct font_descriptor *font, const struct font_style *style);
  
  /*   Destroy a font descriptor allocated with new
   */
  void (*destroy)(struct font_descriptor *fd);
  
  /*   Return information about the i'th font style in the database
   *   into a struct font_style.
   */
  void (*getstyle)(int i, struct font_style *fs);
  
  /*   Return the metrics associated with a font descriptor
   */
  void (*getmetrics)(struct font_descriptor *fd, struct font_metrics *m);

  /* Optionally process driver messages */
  void (*message)(u32 message, u32 param, u32 *ret);
};



#endif /* __H_FONT */

/* The End */
