/* $Id$
 *
 * font_base.c - Handles multiple font backends, and provides default
 *               implementations of fontlib functions
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
#include <pgserver/configfile.h>
#include <pgserver/pgstring.h>

/* Current global font engine- this should eventually be replaced
 * with the ability to use multiple font engines concurrently.
 */
struct fontlib global_fontlib;

/* Populate a fontlib with our default implementations */
void font_register_defaults(struct fontlib *f);


/************************************************** Font engine list */

struct fontengine fontengine_list[] = {
#ifdef CONFIG_FONTENGINE_FREETYPE
  {"freetype", &freetype_regfunc },
#endif
#ifdef CONFIG_FONTENGINE_XFT
  {"xft", &xft_regfunc },
#endif
#ifdef CONFIG_FONTENGINE_BDF
  {"bdf", &bdf_regfunc },
#endif
#ifdef CONFIG_FONTENGINE_TEXTMODE
  {"textmode", &textmode_regfunc },
#endif
#ifdef CONFIG_FONTENGINE_FTGL
  {"ftgl", &ftgl_regfunc },
#endif
  {NULL,NULL}
};


/************************************************** Public functions */

/* This should be extensible to support multiple font engines simultaneously
 * if we need to. Right now one has to be chosen at startup.
 */
g_error font_init(void) {
  g_error e;
  struct fontengine *fe;

  e = font_find_engine(&fe,get_param_str("pgserver","font_engine",NULL));
  errorcheck;
  
  font_register_defaults(&global_fontlib);
  e = fe->reg(&global_fontlib);
  errorcheck;
  
  if (global_fontlib.engine_init) {
    e = global_fontlib.engine_init();
    errorcheck;
  }

  return success;
}

void font_shutdown(void) {
  if (global_fontlib.engine_shutdown)
    global_fontlib.engine_shutdown();
}

/* Create a new font using the default font engine loaded in font_init().
 * If style is NULL, get a default font.
 */
g_error font_descriptor_create(struct font_descriptor **font, const struct font_style *style) {
  g_error e;

  e = g_malloc((void**) font, sizeof(struct font_descriptor));
  errorcheck;

  /* FIXME: Choose from the available font engines */
  (*font)->lib = &global_fontlib;

  return (*font)->lib->create(*font,style);
}

void font_descriptor_destroy(struct font_descriptor *font) {
  font->lib->destroy(font);
  g_free(font);
}

g_error font_find_engine(struct fontengine **fe, const char *name) {
  struct fontengine *p = fontengine_list;

  while (p->name) {
    if (!name || !strcmp(name,p->name)) {
      *fe = p;
      return success;
    }
    p++;
  }

  return mkerror(PG_ERRT_INTERNAL,65);   /* Can't find font engine */
}

/* Get a font style in any installed font engine */
void font_getstyle(int i, struct font_style *fs) {
  /* FIXME: Choose from the available font engines */
  global_fontlib.getstyle(i,fs);
}


/************************************************** Default implementations */

void def_draw_string(struct font_descriptor *fd, hwrbitmap dest, struct pgpair *position,
		     hwrcolor col, const struct pgstring *str, struct pgquad *clip,
		     s16 lgop, s16 angle) {
  struct font_metrics m;
  struct pgstr_iterator p;
  int ch;
  s16 b;
  pgstring_seek(str,&p,0,PGSEEK_SET);
  fd->lib->getmetrics(fd,&m);

  switch (angle) {
    
  case 0:
    position->x += m.margin;
    position->y += m.margin;
    b = position->x;
    break;
    
  case 90:
    position->x += m.margin;
    position->y -= m.margin;
    b = position->y;
    break;
    
  case 180:
    position->x -= m.margin;
    position->y -= m.margin;
    b = position->x;
    break;
    
  case 270:
    position->x -= m.margin;
    position->y += m.margin;
    b = position->y;
    break;
    
  }
  
  while ((ch = pgstring_decode(str,&p))) {
    if (ch=='\n')
      switch (angle) {
	
      case 0:
	position->y += m.lineheight;
	position->x = b;
	break;
	
      case 90:
	position->x += m.lineheight;
	position->y = b;
	break;
	
      case 180:
	position->y -= m.lineheight;
	position->x = b;
	break;
	
      case 270:
	position->x -= m.lineheight;
	position->y = b;
	break;
	
      }
    else if (ch!='\r') {
      if (lgop != PG_LGOP_NULL)
	fd->lib->draw_char(fd,dest,position,col,ch,clip,lgop,angle);
      else
	fd->lib->measure_char(fd,position,ch,angle);
    }
  }
}
 
void def_measure_string(struct font_descriptor *fd, const struct pgstring *str,
			s16 angle, s16 *w, s16 *h) {
  struct font_metrics m;
  int o_w=0, ch;
  struct pgstr_iterator p;
  pgstring_seek(str,&p,0,PGSEEK_SET);
  fd->lib->getmetrics(fd,&m);

  *w = m.margin << 1;
  *h = (*w) + m.ascent + m.descent;
  
  while ((ch = pgstring_decode(str,&p))) {
    if (ch=='\n') {
      *h += m.lineheight;
      if ((*w)>o_w) o_w = *w;
      *w = m.margin << 1;
    }
    else if (ch!='\r') {
      struct pgpair x;
      x.x = 0;
      fd->lib->measure_char(fd,&x,ch,0);
      *w += x.x;
    }
  }
  if ((*w)<o_w) *w = o_w;

  if (angle==90 || angle==270) {
    s16 tmp;
    tmp = *h;
    *h = *w;
    *w = tmp;
  }
}
  
void font_register_defaults(struct fontlib *f) {
  memset(f,0,sizeof(*f));

  f->draw_string = &def_draw_string;
  f->measure_string = &def_measure_string;
}

/* The End */

