/* $Id: font_ttfgl.c,v 1.3 2002/11/21 15:12:48 micahjd Exp $
 *
 * font_ttfgl.c - Font engine that uses OpenGL textures prepared with SDL_ttf.
 *                This engine is very minimalistic compared to the freetype engine:
 *                it doesn't support caching, font indexing, or Unicode.
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
#include <pgserver/sdlgl.h>

#include <ft2build.h>
#include FT_FREETYPE_H


/********************************** Constants ***/

/* Power of two of our font texture's size */ 
#define GL_FONT_TEX_POWER 8

/* Size of the textures to use for font conversion, in pixels. MUST be a power of 2 */ 
#define GL_FONT_TEX_SIZE (1<<(GL_FONT_TEX_POWER))

/* There must be sufficient spacing between characters so that even in the mipmapped  
 * font textures, there is no bleeding of colors between characters. This should be 
 * one greater than the power of two used in GL_FONT_TEX_SIZE */ 
#define GL_FONT_SPACING ((GL_FONT_TEX_POWER)+1)   

/* This driver doesn't support Unicode! This is the number of glyphs from each font to load */
#define NUM_GLYPHS 128

#define CFGSECTION "font-ttfgl"


/********************************** Data structures ***/

struct ttfgl_data {
  float scale;
  int style;
  struct ttfgl_font *font;
};
#define DATA ((struct ttfgl_data*)self->data)

/* Texture and texture coordinates for a glyph */
struct ttfgl_glyph {
  GLuint texture;
  float tx1,ty1,tx2,ty2;   /* Texture coords */
  int x,y,w,h;             /* Character cel */
  int advance;
};  

struct ttfgl_fontload {
  FT_Library ft_lib;
  GLuint texture;
  u8 *pixels;
  int tx,ty,tline;   
};

struct ttfgl_font {
  struct ttfgl_glyph glyphs[NUM_GLYPHS];
  struct font_style style;
  struct font_metrics metrics;
  struct ttfgl_font *next;
};

struct ttfgl_font *ttfgl_font_list = NULL;
struct ttfgl_fontload *ttfgl_load;


/********************************** Utility declarations ***/

g_error ttfgl_load_font(struct ttfgl_fontload *fl, const char *file, int size);
void ttfgl_fontload_storetexture(struct ttfgl_fontload *fl);
g_error ttfgl_fontload_init(struct ttfgl_fontload **fl);
void ttfgl_fontload_finish(struct ttfgl_fontload *fl);
int ttfgl_fontcmp(const struct ttfgl_font *f, const struct font_style *fs);
void ttfgl_load_callback(const char *file, int pathlen);


/********************************** Implementations ***/

g_error ttfgl_engine_init(void) {
  g_error e;

  e = ttfgl_fontload_init(&ttfgl_load);
  errorcheck;

  os_dir_scan(get_param_str(CFGSECTION,"path","/usr/share/fonts"), &ttfgl_load_callback);

  if (!ttfgl_font_list)
    return mkerror(PG_ERRT_IO, 66);  /* Can't find fonts */

  ttfgl_fontload_finish(ttfgl_load);
  return success;
}

void ttfgl_engine_shutdown(void) {
  while (ttfgl_font_list) {
    struct ttfgl_font *f = ttfgl_font_list;
    ttfgl_font_list = f->next;
    free((void*) f->style.name);   /* Created with strdup(), use free() instead of g_free() */
    g_free(f);
  }
}

void ttfgl_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
		   hwrcolor col, int ch, struct quad *clip, s16 lgop, s16 angle) {
  struct ttfgl_glyph *g;
  if (ch > NUM_GLYPHS)
    return;
  g = &DATA->font->glyphs[ch];

  gl_lgop(PG_LGOP_ALPHA);
  glEnable(GL_TEXTURE_2D);
  gl_color(col);

  /* Rotate and scale in a matrix */
  glPushMatrix();
  glTranslatef(position->x, position->y,0);
  glScalef(DATA->scale,DATA->scale,DATA->scale);
  glRotatef(angle,0,0,-1);
  glTranslatef(g->x,g->y,0);

  glBindTexture(GL_TEXTURE_2D, g->texture);
  glBegin(GL_QUADS);
  glTexCoord2f(g->tx1,g->ty1);
  glVertex2f(0,0);
  glTexCoord2f(g->tx2,g->ty1);
  glVertex2f(g->w,0);
  glTexCoord2f(g->tx2,g->ty2);
  glVertex2f(g->w,g->h);
  glTexCoord2f(g->tx1,g->ty2);
  glVertex2f(0,g->h);
  glEnd();

  position->x += g->advance * DATA->scale + 0.5;
      
  /* Clean up */
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
  gl_lgop(PG_LGOP_NONE);
}

void ttfgl_measure_char(struct font_descriptor *self, struct pair *position,
		      int ch, s16 angle) {
  if (ch >= NUM_GLYPHS)
    return;
  position->x += DATA->font->glyphs[ch].advance * DATA->scale + 0.5;
}

g_error ttfgl_create(struct font_descriptor *self, const struct font_style *fs) {
  g_error e;
  struct ttfgl_font *closest, *f;
  int r, closeness = -1;
  struct font_style s;

  if (fs) {
    s = *fs;
  }
  else {
    s.name = NULL;
    s.size = 0;
    s.style = PG_FSTYLE_DEFAULT;
    s.representation = 0;
  }

  /* If they asked for a default font, give it to them */
  if (!s.name || !*s.name || (s.style & PG_FSTYLE_DEFAULT)) {
    if (s.style & PG_FSTYLE_FIXED)
      s.name = get_param_str(CFGSECTION,"default_face","Helmet");
    else
      s.name = get_param_str(CFGSECTION,"default_fixed_face","Nimbus Mono L");
  }     

  /* If they asked for the default size, give it to them */
  if (!s.size)
    s.size = get_param_int(CFGSECTION,"default_size",14);

  e = g_malloc((void**)&self->data, sizeof(struct ttfgl_data));
  errorcheck;
  DATA->style = s.style;

  /* Pick the closest font face */
  for (f=ttfgl_font_list;f;f=f->next) {
    r = ttfgl_fontcmp(f,&s);
    if (r > closeness) {
      closeness = r;
      closest = f;
    }
  }
  DATA->font = closest;

  /* Scale the font we found to match the requested size */
  DATA->scale = ((float)s.size) / ((float)DATA->font->style.size);

  return success;
}

void ttfgl_destroy(struct font_descriptor *self) {
  g_free(DATA);
}
 
void ttfgl_getstyle(int i, struct font_style *fs) {
  struct ttfgl_font *f = ttfgl_font_list;

  for (;i&&f;i--,f=f->next);
  if (f)
    memcpy(fs, &f->style, sizeof(struct font_style));
  else
    memset(fs, 0, sizeof(struct font_style));
}

void ttfgl_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
  /* Scale the font's metrics to the size we're using */
  m->charcell.w = DATA->font->metrics.charcell.w * DATA->scale + 0.5;
  m->charcell.h = DATA->font->metrics.charcell.h * DATA->scale + 0.5;
  m->ascent     = DATA->font->metrics.ascent     * DATA->scale + 0.5;
  m->descent    = DATA->font->metrics.descent    * DATA->scale + 0.5;
  m->linegap    = DATA->font->metrics.linegap    * DATA->scale + 0.5;
  m->lineheight = DATA->font->metrics.lineheight * DATA->scale + 0.5;
  if (DATA->style & PG_FSTYLE_FLUSH)
    m->margin = 0;
  else
    m->margin = DATA->font->metrics.margin     * DATA->scale + 0.5;
}


/********************************** Internal utilities ***/


/* Convert a TrueType font to a series of textures and store the metadata
 * in picogui's fontstyle list.
 */
g_error ttfgl_load_font(struct ttfgl_fontload *fl,const char *file,int size) {
  FT_Face face;
  g_error e;
  Uint16 ch;
  struct ttfgl_font *f;
  int load_flags;

  if (FT_New_Face( fl->ft_lib, file, 0, &face))
    return mkerror(PG_ERRT_IO, 66);  /* Can't find font */
  
  /* New font structure */
  e = g_malloc((void**)&f, sizeof(struct ttfgl_font));
  errorcheck;
  memset(f,0,sizeof(struct ttfgl_font));
  f->next = ttfgl_font_list;
  ttfgl_font_list = f;

  /* Convert the ttfstyle to a picogui style */
  f->style.representation = PG_FR_SCALABLE;
  if (face->style_flags & FT_STYLE_FLAG_ITALIC)    f->style.style |= PG_FSTYLE_ITALIC;
  if (face->style_flags & FT_STYLE_FLAG_BOLD)      f->style.style |= PG_FSTYLE_BOLD;
  if (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) f->style.style |= PG_FSTYLE_FIXED;
  f->style.name = strdup(face->family_name);
  f->style.size = size;

  /* Is there no better way to detect a condensed font? */
  if (face->style_name && strstr(face->style_name,"ondensed"))
    f->style.style |= PG_FSTYLE_CONDENSED;

  /* If the font has "Mono" in the name, trust that it's a fixed width
   * (Added because Nimbus Mono L doesn't have the fixedwidth flag set)
   */
  if (face->family_name && strstr(face->family_name,"Mono"))
    f->style.style |= PG_FSTYLE_FIXED;

  /* Get metrics */
  FT_Set_Pixel_Sizes(face,0,f->style.size);
  f->metrics.ascent = face->size->metrics.ascender >> 6;
  f->metrics.descent = (-face->size->metrics.descender) >> 6;
  f->metrics.lineheight = face->size->metrics.height >> 6;
  f->metrics.linegap = f->metrics.lineheight - f->metrics.ascent - f->metrics.descent;
  f->metrics.charcell.w = face->size->metrics.max_advance >> 6;
  f->metrics.charcell.h = f->metrics.lineheight;

  /* Custom glyph flags */
  load_flags = FT_LOAD_RENDER;
  if (get_param_int(CFGSECTION,"no_hinting",0))
    load_flags |= FT_LOAD_NO_HINTING;
  if (get_param_int(CFGSECTION,"no_bitmap",1))
    load_flags |= FT_LOAD_NO_BITMAP;
  if (get_param_int(CFGSECTION,"force_autohint",1))
    load_flags |= FT_LOAD_FORCE_AUTOHINT;

  for (ch=0;ch<NUM_GLYPHS;ch++) {
    struct ttfgl_glyph *g = &f->glyphs[ch];
    static SDL_Color white = {0xFF,0xFF,0xFF,0};
    static SDL_Color black = {0x00,0x00,0x00,0};
    int i;
    u8 *src,*dest;

    FT_Load_Char(face, ch, load_flags);

    /* Not enough space on this line? */
    if (fl->tx + face->glyph->bitmap.width + GL_FONT_SPACING > GL_FONT_TEX_SIZE) {
      fl->tx = 0;
      fl->ty += fl->tline + GL_FONT_SPACING;
    }

    /* Texture nonexistant or full? Make a new one */
    if (!fl->texture || fl->ty + face->glyph->bitmap.rows + 1 > GL_FONT_TEX_SIZE)
      ttfgl_fontload_storetexture(fl);

    /* copy it into the right spot in our buffer */
    src = face->glyph->bitmap.buffer;
    dest = fl->pixels + fl->tx + (fl->ty << GL_FONT_TEX_POWER);
    for (i=face->glyph->bitmap.rows;i;i--) {
      memcpy(dest,src,face->glyph->bitmap.width);
      src += face->glyph->bitmap.pitch;
      dest += GL_FONT_TEX_SIZE;
    }

    g->texture = fl->texture;
    g->x = face->glyph->bitmap_left - 1;
    g->y = f->metrics.ascent - face->glyph->bitmap_top - 1;
    g->w = face->glyph->bitmap.width+2;
    g->h = face->glyph->bitmap.rows+2;
    g->tx1 = (float)(fl->tx-1) / (float)GL_FONT_TEX_SIZE;
    g->ty1 = (float)(fl->ty-1) / (float)GL_FONT_TEX_SIZE;
    g->tx2 = (float)(fl->tx+g->w-1) / (float)GL_FONT_TEX_SIZE;
    g->ty2 = (float)(fl->ty+g->h-1) / (float)GL_FONT_TEX_SIZE;
    g->advance = face->glyph->advance.x >> 6;

    /* Increment cursor in the texture */
    fl->tx += g->w+GL_FONT_SPACING;
    if (fl->tline < g->h)
      fl->tline = g->h;
  }  

  FT_Done_Face(face);
  return success;
}

/* Convert the bitmap for a font into a texture, get a new texture */
void ttfgl_fontload_storetexture(struct ttfgl_fontload *fl) {
  if ((fl->tx || fl->ty) && fl->texture) {
    glBindTexture(GL_TEXTURE_2D, fl->texture);
    
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_INTENSITY4, GL_FONT_TEX_SIZE, GL_FONT_TEX_SIZE,
		      GL_LUMINANCE, GL_UNSIGNED_BYTE, fl->pixels);
  }

  fl->tx = fl->ty = fl->tline = 0;
  memset(fl->pixels,0,GL_FONT_TEX_SIZE * GL_FONT_TEX_SIZE);
  glGenTextures(1,&fl->texture);
}
 
g_error ttfgl_fontload_init(struct ttfgl_fontload **fl) {
  g_error e;

  e = g_malloc((void**) fl, sizeof(struct ttfgl_fontload));
  errorcheck;
  memset(*fl,0,sizeof(struct ttfgl_fontload));

  if (FT_Init_FreeType(&(*fl)->ft_lib))
    return mkerror(PG_ERRT_IO,119);   /* Error initializing font engine */

  /* The "pixels" pointer is an 8bpp bitmap holding the font texture
   * as we assemble it. In ttfgl_fontload_storetexture it gets converted into
   * a texture and several mipmaps.
   */
  e = g_malloc((void**) &(*fl)->pixels, GL_FONT_TEX_SIZE*GL_FONT_TEX_SIZE);
  errorcheck;

  /* Prime the pump by clearing the texture buffer and getting a new texture ID */
  ttfgl_fontload_storetexture(*fl);

  return success;
}

void ttfgl_fontload_finish(struct ttfgl_fontload *fl) {
  /* Store the last texture we were working on */
  ttfgl_fontload_storetexture(fl);

  glDeleteTextures(1,&fl->texture);
  g_free(fl->pixels);
  FT_Done_FreeType(fl->ft_lib);
  g_free(fl);
}

/* Version of fontcmp that counts the name, style, and size */
int ttfgl_fontcmp(const struct ttfgl_font *f, const struct font_style *fs) {
  int result;
  int szdif;
  
  szdif = f->style.size - fs->size;
  if (szdif < 0) {
    /* We want to get the font as small as possible without going under the requested
     * size, so give a heavy penalty if szdif is negative.
     */
    result = 0;
  }
  else {
    result = FCMP_SIZE(szdif);
  }

  if (fs->name && (!strcasecmp(fs->name,f->style.name))) 
    result |= FCMP_NAME;
  if ((fs->style&PG_FSTYLE_FIXED)==(f->style.style&PG_FSTYLE_FIXED)) 
    result |= FCMP_FIXEDVAR;
  if ((fs->style&PG_FSTYLE_DEFAULT) && (f->style.style&PG_FSTYLE_DEFAULT)) 
    result |= FCMP_DEFAULT;
  if ((fs->style&(PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC)) == (f->style.style&(PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC)))
    result |= FCMP_STYLE;
  if ( ((fs->style&PG_FSTYLE_SYMBOL)==(f->style.style&PG_FSTYLE_SYMBOL)) &&
       ((fs->style&PG_FSTYLE_SUBSET)==(f->style.style&PG_FSTYLE_SUBSET)))
    result |= FCMP_TYPE;

  return result;
}

void ttfgl_load_callback(const char *file, int pathlen) {
  /* Load one of our fonts at all sizes */
  ttfgl_load_font(ttfgl_load,file,14); 
}


/********************************** Registration ***/

g_error ttfgl_regfunc(struct fontlib *f) {
  f->engine_init = &ttfgl_engine_init;
  f->engine_shutdown = &ttfgl_engine_shutdown;
  f->draw_char = &ttfgl_draw_char;
  f->measure_char = &ttfgl_measure_char;
  f->create = &ttfgl_create;
  f->destroy = &ttfgl_destroy;
  f->getstyle = &ttfgl_getstyle;
  f->getmetrics = &ttfgl_getmetrics;
  return success;
}

/* The End */

