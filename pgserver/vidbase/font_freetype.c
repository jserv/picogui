/* $Id: font_freetype.c,v 1.6 2002/10/13 08:15:35 micahjd Exp $
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

#ifdef CONFIG_FREETYPE_GAMMA
#include <math.h>
float ft_gamma;
u8 ft_gamma_table[256];
void ft_build_gamma_table(u8 *table, float gamma);
void ft_apply_gamma_table(u8 *table, FT_Bitmap *b);
#endif

struct freetype_fontdesc {
  FT_Face face;
  FT_Size size;
  int flags;
};
#define DATA ((struct freetype_fontdesc *)self->data)


FT_Library  ft_lib;
FT_Face     ft_facelist;   /* Linked list, via generic.data */
FT_Int32    ft_glyph_flags;
const char* ft_default_name;
int         ft_default_size;
int         ft_minimum_size;
int         ft_dpi;

/* Various bits turned on for matches in fontcmp.  The order
 * of these bits defines the priority of the various
 * attributes
 */
#define FCMP_FIXEDVAR (1<<12)
#define FCMP_SIZE(x)  ((0xFF-(x&0xFF))<<3)   /* This macro is passed the
						difference in size between the
						request and the actual font */
#define FCMP_NAME     (1<<2)
#define FCMP_DEFAULT  (1<<1)
#define FCMP_STYLE    (1<<0)

void ft_face_scan(const char *directory);
void ft_face_load(const char *file);
int ft_fontcmp(FT_Face f, const struct font_style *fs);
void ft_get_face_style(FT_Face f, struct font_style *fs);

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

  ft_default_size = get_param_int("font-freetype","default_size",12);
  ft_default_name = get_param_str("font-freetype","default_name",NULL);
  ft_minimum_size = get_param_int("font-freetype","minimum_size",5);
  ft_dpi = get_param_int("font-freetype","dpi",72);

#ifdef CONFIG_FREETYPE_GAMMA
  ft_gamma = atof(get_param_str("font-freetype","gamma","1.5"));
  ft_build_gamma_table(ft_gamma_table,ft_gamma);
#endif

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
#ifdef CONFIG_FREETYPE_GAMMA
  ft_apply_gamma_table(ft_gamma_table,&b);
#endif

  /* PicoGUI's character origin is at the top-left of the bounding box.
   * Add the ascent to reach the baseline, then subtract the bitmap origin
   * from that.
   */
  VID(alpha_charblit)(dest,b.buffer,
		      position->x + DATA->face->glyph->bitmap_left,
		      position->y + (DATA->size->metrics.ascender >> 6) -
		      DATA->face->glyph->bitmap_top,
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
  int size;
  struct font_style defaultfs;
  int r, closeness = -1;
  FT_Face closest, f;

  e = g_malloc((void**)&self->data,sizeof(struct freetype_fontdesc));
  errorcheck;

  if (!fs) {
    fs = &defaultfs;
    defaultfs.name = ft_default_name;
    defaultfs.size = 0;
    defaultfs.style = PG_FSTYLE_DEFAULT;
    defaultfs.representation = 0;
  }

  /* Pick the closest font face */
  for (f=ft_facelist;f;f=(FT_Face)f->generic.data) {
    r = ft_fontcmp(f,fs);
    if (r > closeness) {
      closeness = r;
      closest = f;
    }
  }
  DATA->face = closest;

  /* Set the size */
  size = fs->size ? fs->size : ft_default_size;
  if (size < ft_minimum_size)
    size = ft_minimum_size;
  DATA->flags = fs->style;
  FT_New_Size(DATA->face,&DATA->size);
  FT_Activate_Size(DATA->size);
  FT_Set_Char_Size(DATA->face,0,size*64,ft_dpi,ft_dpi);

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

  if (DATA->flags & PG_FSTYLE_FLUSH)
    m->margin = 0;
  else
    m->margin = m->descent;
}

/* Our own version of draw_string that handles subpixel pen movement 
 */
void freetype_draw_string(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
			  hwrcolor col, const struct pgstring *str, struct quad *clip,
			  s16 lgop, s16 angle) {
  struct font_metrics m;
  struct pgstr_iterator p = PGSTR_I_NULL;
  int x,y,margin,lh,b,ch;
  self->lib->getmetrics(self,&m);

  x = position->x * 64;
  y = position->y * 64;
  margin = m.margin * 64;
  lh = DATA->size->metrics.height;

  switch (angle) {
    
  case 0:
    x += margin;
    y += margin;
    b = x;
    break;
    
  case 90:
    x += margin;
    y -= margin;
    b = y;
    break;
    
  case 180:
    x -= margin;
    y -= margin;
    b = x;
    break;

  case 270:
    x -= margin;
    y += margin;
    b = y;
    break;
    
  }
  
  while ((ch = pgstring_decode(str,&p))) {
    if (ch=='\n')
      switch (angle) {
	
      case 0:
	y += lh;
	x = b;
	break;
	
      case 90:
	x += lh;
	y = b;
	break;
	
      case 180:
	y -= lh;
	x = b;
	break;
	
      case 270:
	x -= lh;
	y = b;
	break;
	
      }
    else if (ch!='\r') {
      position->x = x>>6;
      position->y = y>>6;
      self->lib->draw_char(self,dest,position,col,ch,clip,lgop,angle);

      /* Ignore the position modifications made by
       * this, do our own in subpixel units 
       */
      switch (angle) {
      case 0:
	x += DATA->face->glyph->advance.x;
	y += DATA->face->glyph->advance.y;
	break;
      case 90:
	x += DATA->face->glyph->advance.y;
	y -= DATA->face->glyph->advance.x;
	break;
      case 180:
	x -= DATA->face->glyph->advance.x;
	y -= DATA->face->glyph->advance.y;
	break;
      case 270:
	x -= DATA->face->glyph->advance.y;
	y += DATA->face->glyph->advance.x;
	break;
      }
    }
  }

  position->x = x >> 6;
  position->y = y >> 6;
}

/* Our own version of measure_string that handles subpixel pen movement 
 */
void freetype_measure_string(struct font_descriptor *self, const struct pgstring *str,
			     s16 angle, s16 *w, s16 *h) {
  struct font_metrics m;
  int max_x = 0, ch, x, y, original_x;
  struct pgstr_iterator p = PGSTR_I_NULL;
  self->lib->getmetrics(self,&m);

  original_x = x = m.margin << 7;
  y = x + DATA->size->metrics.height + DATA->size->metrics.descender;

  while ((ch = pgstring_decode(str,&p))) {
    if (ch=='\n') {
      y += DATA->size->metrics.height;
      if (x>max_x) max_x = x;
      x = original_x;
    }
    else if (ch!='\r') {
      FT_Activate_Size(DATA->size);
      FT_Load_Char(DATA->face,ch, ft_glyph_flags);
      x += DATA->face->glyph->advance.x;
      y += DATA->face->glyph->advance.y;
    }
  }
  if (x>max_x) max_x = x;

  if (angle==90 || angle==270) {
    *h = max_x >> 6;
    *w = y >> 6;
  }
  else {
    *w = max_x >> 6;
    *h = y >> 6;
  }
}

void freetype_getstyle(int i, struct font_style *fs) {
  FT_Face f;
  memset(fs,0,sizeof(*fs));

  /* Iterate to the face they asked about */
  for (f=ft_facelist;f && i>0;i--,f=(FT_Face)f->generic.data);
  if (!f) return;

  ft_get_face_style(f,fs);
}


/********************************** Internal utilities ***/

void ft_get_face_style(FT_Face f, struct font_style *fs) {
  fs->name = f->family_name;
  fs->size = f->height;
  if (f->style_flags & FT_STYLE_FLAG_ITALIC)    fs->style |= PG_FSTYLE_ITALIC;
  if (f->style_flags & FT_STYLE_FLAG_BOLD)      fs->style |= PG_FSTYLE_BOLD;
  if (f->face_flags & FT_FACE_FLAG_SCALABLE)    fs->representation |= PG_FR_SCALABLE;
  if (f->face_flags & FT_FACE_FLAG_FIXED_WIDTH) fs->style |= PG_FSTYLE_FIXED;
}

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

/* Gauge the closeness between a face and a requested style
 */
int ft_fontcmp(FT_Face f, const struct font_style *fs) {
  int result;
  int szdif;
  struct font_style f_fs;

  memset(&f_fs,0,sizeof(f_fs));
  ft_get_face_style(f,&f_fs);

  if (fs->size) { 
    szdif = abs(f_fs.size - fs->size);
    result = FCMP_SIZE(szdif);
  }    
  else {
    result = 0;
  }

  if (fs->name && (!strcasecmp(f_fs.name,fs->name)))
    result |= FCMP_NAME;
  if ((f_fs.style&PG_FSTYLE_FIXED)==(fs->style&PG_FSTYLE_FIXED))
    result |= FCMP_FIXEDVAR;
  if ((f_fs.style&PG_FSTYLE_DEFAULT) && (fs->style&PG_FSTYLE_DEFAULT)) 
    result |= FCMP_DEFAULT;
  if ((f_fs.style&(PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC)) && 
      (fs->style&(PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC))) 
    result |= FCMP_STYLE;

  /*
  printf("Comparing potential %s:%d:%d to requested %s:%d:%d, closeness %d\n",
	 f_fs.name,f_fs.size,f_fs.style,fs->name,fs->size,fs->style,result);
  */

  return result;
}

#ifdef CONFIG_FREETYPE_GAMMA
void ft_build_gamma_table(u8 *table, float gamma) {
  int i;
  for (i=0;i<256;i++)
    table[i] = 255 * pow(i/255.0, 1/gamma);
}

void ft_apply_gamma_table(u8 *table, FT_Bitmap *b) {
  u8 *l,*p;
  int i,j;

  l = b->buffer;
  for (j=0;j<b->rows;j++,l+=b->pitch)
    for (i=0,p=l;i<b->width;i++,p++)
      *p = table[*p];
}
#endif /* CONFIG_FREETYPE_GAMMA */

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
  f->measure_string = &freetype_measure_string;
  f->draw_string = &freetype_draw_string;
  return success;
}

/* The End */

