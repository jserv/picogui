/* $Id: font_freetype.c,v 1.3 2002/10/12 19:53:49 micahjd Exp $
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
#include <pgserver/configfile.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <ft2build.h>
#include FT_FREETYPE_H


struct freetype_fontdesc {
  FT_Face face;
  FT_Size size;
};
#define DATA ((struct freetype_fontdesc *)self->data)


FT_Library ft_lib;
FT_Face    ft_facelist;   /* Linked list, via generic.data */
FT_Int32   ft_glyph_flags;

void ft_face_scan(const char *directory);
void ft_face_load(const char *file);

/********************************** Implementations ***/

g_error freetype_engine_init(void) {
  g_error e;

  if (FT_Init_FreeType(&ft_lib))
    return mkerror(PG_ERRT_IO,119);   /* Error initializing font engine */

  ft_facelist = NULL;
  ft_face_scan(get_param_str("font-freetype","path","/usr/share/fonts"));
  
  if (!ft_facelist)
    return mkerror(PG_ERRT_IO,66);  /* Can't find fonts */

  ft_glyph_flags = 0;
  if (get_param_int("font-freetype","no_hinting",0))
    ft_glyph_flags |= FT_LOAD_NO_HINTING;
  if (get_param_int("font-freetype","no_bitmap",1))
    ft_glyph_flags |= FT_LOAD_NO_BITMAP;
  if (get_param_int("font-freetype","force_autohint",1))
    ft_glyph_flags |= FT_LOAD_FORCE_AUTOHINT;
  if (get_param_int("font-freetype","no_autohint",0))
    ft_glyph_flags |= FT_LOAD_NO_AUTOHINT;
  
  return success;
}

void freetype_engine_shutdown(void) {
  FT_Face f = ft_facelist;
  FT_Face condemn;

  while (f) {
    condemn = f;
    f = f->generic.data;
    FT_Done_Face(condemn);
  }

  FT_Done_FreeType(ft_lib);
}

void freetype_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
		   hwrcolor col, int ch, struct quad *clip, s16 lgop, s16 angle) {
  int i,j;
  FT_Bitmap b;

  FT_Activate_Size(DATA->size);
  FT_Load_Char(DATA->face,ch, FT_LOAD_RENDER | ft_glyph_flags);

  b = DATA->face->glyph->bitmap;

  VID(alpha_charblit)(dest,b.buffer,position->x,position->y,
		      b.width,b.rows,b.pitch,angle,col,clip,lgop);

  position->x += DATA->face->glyph->advance.x >> 6;
  position->y += DATA->face->glyph->advance.y >> 6;
}

void freetype_measure_char(struct font_descriptor *self, struct pair *position,
		      int ch, s16 angle) {
  FT_Activate_Size(DATA->size);
  FT_Load_Char(DATA->face,ch, ft_glyph_flags);
  position->x += DATA->face->glyph->advance.x >> 6;
  position->y += DATA->face->glyph->advance.y >> 6;
}

g_error freetype_create(struct font_descriptor *self, const struct font_style *fs) {
  g_error e;

  e = g_malloc((void**)&self->data,sizeof(struct freetype_fontdesc));
  errorcheck;

  /* Pick the closest font face */
  DATA->face = ft_facelist;

  /* Set the size */
  FT_New_Size(DATA->face,&DATA->size);
  if (fs) {
    FT_Activate_Size(DATA->size);
    FT_Set_Pixel_Sizes(DATA->face,0,fs->size);
  }

  return success;
}

void freetype_destroy(struct font_descriptor *self) {
  FT_Done_Size(DATA->size);
  g_free(DATA);
}
 
void freetype_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
  FT_Activate_Size(DATA->size);

  m->ascent = DATA->size->metrics.ascender >> 6;
  m->descent = (-DATA->size->metrics.descender) >> 6;
  m->lineheight = DATA->size->metrics.height >> 6;
  m->linegap = m->lineheight - m->ascent - m->descent;
  m->charcell.w = DATA->size->metrics.max_advance >> 6;
  m->charcell.h = m->ascent + m->descent;
  m->margin = m->descent;
}

void freetype_measure_string(struct font_descriptor *self, const struct pgstring *str,
			s16 angle, s16 *w, s16 *h) {
}

void freetype_getstyle(int i, struct font_style *fs) {
}

/********************************** Internal utilities ***/

/* Recursively scan for fonts
 */
void ft_face_scan(const char *directory) {
  DIR *d = opendir(directory);
  struct dirent *dent;
  char buf[NAME_MAX];
  struct stat st;

  if (!d)
    return;

  while ((dent = readdir(d))) {
    /* Skip hidden files, "..", and "." */
    if (dent->d_name[0] == '.')
      continue;

    /* Find the path of this entry */
    buf[NAME_MAX-1] = 0;
    strncpy(buf,directory,NAME_MAX-2);
    strcat(buf,"/");
    strncat(buf,dent->d_name,NAME_MAX-1-strlen(buf));

    /* If it's a directory, recurse */
    if (stat(buf,&st) < 0)
      continue;
    if (S_ISDIR(st.st_mode)) {
      ft_face_scan(buf);
      continue;
    }

    /* Otherwise, try to load it */
    ft_face_load(buf);
  }

  closedir(d);
}

/* Load one font face from a file into our linked list if we can 
 */
void ft_face_load(const char *file) {
  FT_Face face;
  if (FT_New_Face(ft_lib,file,0,&face))
    return;
  face->generic.data = ft_facelist;
  ft_facelist = face;
}

/********************************** Registration ***/

g_error freetype_regfunc(struct fontlib *f) {
  f->engine_init = &freetype_engine_init;
  f->engine_shutdown = &freetype_engine_shutdown;
  f->draw_char = &freetype_draw_char;
  f->measure_char = &freetype_measure_char;
  f->create = &freetype_create;
  f->destroy = &freetype_destroy;
  f->getstyle = &freetype_getstyle;
  f->getmetrics = &freetype_getmetrics;
  //  f->measure_string = &freetype_measure_string;
  return success;
}

/* The End */

