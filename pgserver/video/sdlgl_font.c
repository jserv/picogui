/* $Id: sdlgl_font.c,v 1.2 2002/03/03 07:35:03 micahjd Exp $
 *
 * sdlgl_font.c - OpenGL driver for picogui, using SDL for portability.
 *                Replace PicoGUI's normal font rendering with TrueType
 *                fonts mapped onto textured quads. Requires SDL_ttf.
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
#include <pgserver/sdlgl.h>

/* Convert a TrueType font to a series of textures and store the metadata
 * in picogui's fontstyle list.
 */
g_error gl_load_font(struct gl_fontload *fl,const char *file) {
  TTF_Font *ttf;
  struct fontstyle_node *fsn;
  g_error e;
  char smallname[40];
  char *p;

  ttf = TTF_OpenFont(file, get_param_int(GL_SECTION,"font_resolution",32));
  /* Don't complain about font loading errors, it's not that important */
  if (!ttf) return;

  /* Allocate a picogui fontstyle node */
  e = g_malloc((void**)&fsn, sizeof(struct fontstyle_node));
  errorcheck;
  memset(fsn,0,sizeof(struct fontstyle_node));

  /* Make a small name for the font, using the filename */
  p = strrchr(file,'/');
  if (!p) p = strrchr(file,'\\');
  if (!p) p = (char*) file;
  strncpy(smallname,p,sizeof(smallname));
  smallname[sizeof(smallname)-1] = 0;
  p = strrchr(smallname,'.');
  if (p) *p = 0;
  e = g_malloc((void**)&fsn->name, strlen(smallname)+1);
  errorcheck;
  strcpy((char*) fsn->name,smallname);

  fsn->size = TTF_FontHeight(ttf);

  /* Build all the font representations */

  e = gl_load_font_style(fl,ttf,&fsn->normal,TTF_STYLE_NORMAL);
  errorcheck;
  e = gl_load_font_style(fl,ttf,&fsn->bold,TTF_STYLE_BOLD);
  errorcheck;
  e = gl_load_font_style(fl,ttf,&fsn->italic,TTF_STYLE_ITALIC);
  errorcheck;
  e = gl_load_font_style(fl,ttf,&fsn->bolditalic,TTF_STYLE_BOLD | TTF_STYLE_ITALIC);
  errorcheck;

  /* Link into picogui's font list */
  fsn->next = fontstyles;
  fontstyles = fsn;

  TTF_CloseFont(ttf);
  return success;
}

/* Load all the glyphs from one font style into a texture, allocating a new struct font
 *
 * FIXME: This just loads glyphs 0 through 255. This breaks Unicode!
 *         We need a way to determine which glyphs actually exist.
 */
g_error gl_load_font_style(struct gl_fontload *fl,TTF_Font *ttf, struct font **ppf, int style) {
  g_error e;
  struct font *f;
  Uint16 ch;
  struct gl_glyph *glg;
  struct fontglyph *fg;
  int minx, maxx, miny, maxy, advance;
  SDL_Surface *surf;
  static SDL_Color white = {0xFF,0xFF,0xFF,0};
  static SDL_Color black = {0x00,0x00,0x00,0};
  int x,y;
  u8 *src,*dest,*p;

  TTF_SetFontStyle(ttf, style);

  /* Set up a new picogui font structure */
  e = g_malloc((void**)ppf, sizeof(struct font));
  errorcheck;
  f = *ppf;
  memset(f,0,sizeof(struct font));

  f->numglyphs = 256;
  f->ascent = TTF_FontAscent(ttf);
  f->descent = -TTF_FontDescent(ttf);
  
  /* Allocate the glyph bitmap array and the glyph array */
  e = g_malloc((void**)&f->bitmaps, sizeof(struct gl_glyph) * f->numglyphs);
  errorcheck;
  glg = (struct gl_glyph*) f->bitmaps;
  e = g_malloc((void**)&f->glyphs, sizeof(struct fontglyph) * f->numglyphs);
  errorcheck;
  fg = (struct fontglyph*) f->glyphs;

  /* FIXME: This won't work for Unicode, and it's inefficient anyways.
   * we need a way to figure out what glyphs actually exist in the font.
   */
  for (ch=0;ch<256;ch++) {
    TTF_GlyphMetrics(ttf, ch, &minx, &maxx, &miny, &maxy, &advance);
    surf = TTF_RenderGlyph_Shaded(ttf, ch, white,black);
    
    /* Not enough space on this line? */
    if (fl->tx+surf->w+GL_FONT_SPACING > GL_FONT_TEX_SIZE) {
      fl->tx = 0;
      fl->ty += fl->tline+GL_FONT_SPACING;
    }

    /* Texture nonexistant or full? Make a new one */
    if (!fl->texture || fl->ty+surf->h+1 > GL_FONT_TEX_SIZE)
      gl_fontload_storetexture(fl);

    /* Scale the glyph bitmap into the full range 0-255
     * while copying it into the right spot in our buffer
     */
    src = (u8*) surf->pixels;
    dest = fl->pixels + fl->tx + (fl->ty << GL_FONT_TEX_POWER);
    for (y=0;y<surf->h;y++,dest+=GL_FONT_TEX_SIZE)
      for (x=0,p=dest;x<surf->w;x++,p++,src++)
	*p = (*src)*255/4;

    /* Save the texture data in the 'bitmaps' table */
    glg[ch].texture = fl->texture;
    glg[ch].tx1 = (float)fl->tx / (float)GL_FONT_TEX_SIZE;
    glg[ch].ty1 = (float)fl->ty / (float)GL_FONT_TEX_SIZE;
    glg[ch].tx2 = (float)(fl->tx+surf->w) / (float)GL_FONT_TEX_SIZE;
    glg[ch].ty2 = (float)(fl->ty+surf->h) / (float)GL_FONT_TEX_SIZE;

    /* Fill in the normal picogui glyph data */
    fg[ch].encoding = ch;
    fg[ch].bitmap = ch * sizeof(struct gl_glyph);
    fg[ch].dwidth = advance;
    fg[ch].w = surf->w;
    fg[ch].h = surf->h;
    fg[ch].x = minx;
    fg[ch].y = 0;

    if (surf->w > f->w)
      f->w = surf->w;
    if (surf->h > f->h+f->descent)
      f->h = surf->h - f->descent;

    /* Increment cursor in the texture */
    fl->tx += surf->w+GL_FONT_SPACING;
    if (fl->tline < surf->h)
      fl->tline = surf->h;
    
    SDL_FreeSurface(surf);
  }  

  return success;
}

void sdlgl_font_newdesc(struct fontdesc *fd, const u8 *name, int size, int flags) {
  /* Let them have any size, dont constrain to the size in the fontstyle.
   * Store the size in "extra"
   */
  if (!size) size = 15;
  fd->extra = (void*) size;
}

void sdlgl_font_outtext_hook(hwrbitmap *dest, struct fontdesc **fd,
			     s16 *x,s16 *y,hwrcolor *col,const u8 **txt,
			     struct quad **clip, s16 *lgop, s16 *angle) {
  int size = (int) (*fd)->extra;  
  float scale = (float)size / (*fd)->font->h;
  int ch;
  struct fontglyph const *g;
  struct gl_glyph *glg;

  //  DBG("outtext: '%s' at scale %f, size %d\n",*txt,scale,size);

  /* FIXME: No lgops for text yet, but i don't think anything actually
   *        uses those yet.
   * FIXME: Can't render text to offscreen bitmaps yet.
   */

  /* Set up blending and texturing */
  gl_lgop(PG_LGOP_ALPHA);
  glEnable(GL_TEXTURE_2D);
  gl_color(*col);

  /* Use OpenGL's matrix for translation, scaling, and rotation 
   * (So much easier than picogui's usual method :)
   */
  glPushMatrix();
  glTranslatef(*x,*y,0);
  glScalef(scale,scale,scale);
  glRotatef(*angle,0,0,1);

  while ((ch = (*fd)->decoder(txt))) {
    if (ch=='\n')
      glTranslatef(0,(*fd)->font->h + (*fd)->interline_space,0);
    else if (ch!='\r') {
      if ((*fd)->passwdc > 0)
	ch = (*fd)->passwdc;

      g = vid->font_getglyph(*fd,ch);
      glg = (struct gl_glyph*)(((u8*)(*fd)->font->bitmaps)+g->bitmap);
      
      glBindTexture(GL_TEXTURE_2D, glg->texture);
      glBegin(GL_QUADS);
      glNormal3f(0.0f,0.0f,1.0f);
      glTexCoord2f(glg->tx1,glg->ty1);
      glVertex2f(0,0);
      glTexCoord2f(glg->tx2,glg->ty1);
      glVertex2f(g->w,0);
      glTexCoord2f(glg->tx2,glg->ty2);
      glVertex2f(g->w,g->h);
      glTexCoord2f(glg->tx1,glg->ty2);
      glVertex2f(0,g->h);
      glEnd();

      glTranslatef(g->dwidth,0,0);
    }
  }

  /* Clean up */
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
  gl_lgop(PG_LGOP_NONE);

  /* Prevent normal outtext rendering */
  *txt = "";
  *lgop = PG_LGOP_NONE;
}
 
void sdlgl_font_sizetext_hook(struct fontdesc *fd, s16 *w, s16 *h, const u8 *txt) {
  int size = (int) fd->extra;

  /* Most of the sizetext should still be fine for us, but we need to scale
   * the final result by the real requested font size.
   */
  //  DBG("sizing from %d,%d. size=%d, font->h=%d\n",*w,*h,size,fd->font->h);
  *w = (*w) * size / fd->font->h;
  *h = (*h) * size / fd->font->h;
}

/* Convert the bitmap for a font into a texture, get a new texture */
void gl_fontload_storetexture(struct gl_fontload *fl) {
  if ((fl->tx || fl->ty) && fl->texture) {
    glBindTexture(GL_TEXTURE_2D, fl->texture);
  
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);

    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_INTENSITY4, GL_FONT_TEX_SIZE, GL_FONT_TEX_SIZE,
		      GL_LUMINANCE, GL_UNSIGNED_BYTE, fl->pixels);

#if 0    /** Debuggative cruft **/
    printf("Showing texture %d\n", fl->texture);
    gl_lgop(PG_LGOP_NONE);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    gl_color(0x000080);
    glVertex2f(0,0);
    glVertex2f(10000,0);
    glVertex2f(10000,10000);
    glVertex2f(0,10000);
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fl->texture);
    glBegin(GL_QUADS);
    gl_color(0xFFFFFF);
    glNormal3f(0.0f,0.0f,1.0f);
    glTexCoord2f(0,0);
    glVertex2f(0,0);
    glTexCoord2f(1,0);
    glVertex2f(GL_FONT_TEX_SIZE,0);
    glTexCoord2f(1,1);
    glVertex2f(GL_FONT_TEX_SIZE,GL_FONT_TEX_SIZE);
    glTexCoord2f(0,1);
    glVertex2f(0,GL_FONT_TEX_SIZE);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    SDL_GL_SwapBuffers();
    usleep(100000);
#endif
  }

  fl->tx = fl->ty = fl->tline = 0;
  memset(fl->pixels,0,GL_FONT_TEX_SIZE * GL_FONT_TEX_SIZE);
  glGenTextures(1,&fl->texture);
}
 
g_error gl_fontload_init(struct gl_fontload **fl) {
  g_error e;

  e = g_malloc((void**) fl, sizeof(struct gl_fontload));
  errorcheck;
  memset(*fl,0,sizeof(struct gl_fontload));

  /* The "pixels" pointer is an 8bpp bitmap holding the font texture
   * as we assemble it. In gl_fontload_storetexture it gets converted into
   * a texture and several mipmaps.
   */
  e = g_malloc((void**) &(*fl)->pixels, GL_FONT_TEX_SIZE*GL_FONT_TEX_SIZE);
  errorcheck;

  /* Prime the pump by clearing the texture buffer and getting a new texture ID */
  gl_fontload_storetexture(*fl);

  return success;
}

void gl_fontload_finish(struct gl_fontload *fl) {
  /* Store the last texture we were working on */
  gl_fontload_storetexture(fl);

  glDeleteTextures(1,&fl->texture);
  g_free(fl->pixels);
  g_free(fl);
}

/* The End */









