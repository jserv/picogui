/* $Id: font_ttfgl.c,v 1.2 2002/11/21 12:43:40 micahjd Exp $
 *
 * font_ttfgl.c - Font engine that uses OpenGL textures prepared with SDL_ttf.
 *                This engine is very minimalistic compared to the freetype engine:
 *                it doesn't support caching, hinting, font indexing, or Unicode.
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
#define GL_FONT_TEX_POWER 9 

/* Size of the textures to use for font conversion, in pixels. MUST be a power of 2 */ 
#define GL_FONT_TEX_SIZE (1<<(GL_FONT_TEX_POWER))

/* There must be sufficient spacing between characters so that even in the mipmapped  
 * font textures, there is no bleeding of colors between characters. This should be 
 * one greater than the power of two used in GL_FONT_TEX_SIZE */ 
#define GL_FONT_SPACING ((GL_FONT_TEX_POWER)+1)   

/* This driver doesn't support Unicode! This is the number of glyphs from each font to load */
#define NUM_GLYPHS 256


/********************************** Data structures ***/

struct ttfgl_data {
  float scale;
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


/********************************** Utility declarations ***/

g_error ttfgl_load_font(struct ttfgl_fontload *fl,const char *file);
void ttfgl_fontload_storetexture(struct ttfgl_fontload *fl);
g_error ttfgl_fontload_init(struct ttfgl_fontload **fl);
void ttfgl_fontload_finish(struct ttfgl_fontload *fl);


/********************************** Implementations ***/

g_error ttfgl_engine_init(void) {
  g_error e;
  struct ttfgl_fontload *fl;

  e = ttfgl_fontload_init(&fl);
  errorcheck;
  
  e = ttfgl_load_font(fl,get_param_str("font-ttfgl","font","/usr/share/fonts/truetype/openoffice/helmetb.ttf")); 
  errorcheck;

  ttfgl_fontload_finish(fl);
  return success;
}

void ttfgl_engine_shutdown(void) {
  while (ttfgl_font_list) {
    struct ttfgl_font *f = ttfgl_font_list;
    ttfgl_font_list = f->next;
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
  int size;

  if (fs && fs->size)
    size = fs->size;
  else
    size = get_param_int("font-ttfgl","default_size",14);

  e = g_malloc((void**)&self->data, sizeof(struct ttfgl_data));
  errorcheck;

  DATA->font = ttfgl_font_list;
  DATA->scale = ((float)size) / ((float)DATA->font->style.size);

  return success;
}

void ttfgl_destroy(struct font_descriptor *self) {
  g_free(DATA);
}
 
void ttfgl_getstyle(int i, struct font_style *fs) {
}

void ttfgl_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
  /* Scale the font's metrics to the size we're using */
  m->charcell.w = DATA->font->metrics.charcell.w * DATA->scale + 0.5;
  m->charcell.h = DATA->font->metrics.charcell.h * DATA->scale + 0.5;
  m->ascent     = DATA->font->metrics.ascent     * DATA->scale + 0.5;
  m->descent    = DATA->font->metrics.descent    * DATA->scale + 0.5;
  m->margin     = DATA->font->metrics.margin     * DATA->scale + 0.5;
  m->linegap    = DATA->font->metrics.linegap    * DATA->scale + 0.5;
  m->lineheight = DATA->font->metrics.lineheight * DATA->scale + 0.5;
}


/********************************** Internal utilities ***/


/* Convert a TrueType font to a series of textures and store the metadata
 * in picogui's fontstyle list.
 */
g_error ttfgl_load_font(struct ttfgl_fontload *fl,const char *file) {
  FT_Face face;
  g_error e;
  Uint16 ch;
  struct ttfgl_font *f;

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
  f->style.name = "ttfgl font";
  f->style.size = get_param_int("font-ttfgl","font_resolution",32);

  FT_Set_Pixel_Sizes(face,0,f->style.size);

  /* Get metrics */
  f->metrics.ascent = face->size->metrics.ascender >> 6;
  f->metrics.descent = (-face->size->metrics.descender) >> 6;
  f->metrics.lineheight = face->size->metrics.height >> 6;
  f->metrics.linegap = f->metrics.lineheight - f->metrics.ascent - f->metrics.descent;
  f->metrics.charcell.w = face->size->metrics.max_advance >> 6;
  f->metrics.charcell.h = f->metrics.lineheight;

  for (ch=0;ch<NUM_GLYPHS;ch++) {
    struct ttfgl_glyph *g = &f->glyphs[ch];
    static SDL_Color white = {0xFF,0xFF,0xFF,0};
    static SDL_Color black = {0x00,0x00,0x00,0};
    int i;
    u8 *src,*dest;

    FT_Load_Char(face, ch, FT_LOAD_RENDER | FT_LOAD_NO_BITMAP);

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
    g->x = face->glyph->bitmap_left;
    g->y = f->metrics.ascent - face->glyph->bitmap_top;
    g->w = face->glyph->bitmap.width;
    g->h = face->glyph->bitmap.rows;
    g->tx1 = (float)fl->tx / (float)GL_FONT_TEX_SIZE;
    g->ty1 = (float)fl->ty / (float)GL_FONT_TEX_SIZE;
    g->tx2 = (float)(fl->tx+g->w) / (float)GL_FONT_TEX_SIZE;
    g->ty2 = (float)(fl->ty+g->h) / (float)GL_FONT_TEX_SIZE;
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

