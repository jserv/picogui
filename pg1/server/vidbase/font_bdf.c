/* $Id$
 *
 * font_bdf.c - Font engine that uses fonts compiled into pgserver,
 *              converted from BDF fonts at compile-time.
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
#include <pgserver/font_bdf.h>
#include <pgserver/appmgr.h>      /* Default font */
#include <pgserver/pgstring.h>

#define DATA ((struct bdf_fontdesc *)self->data)

/* This defines how italic the generated italic is */
#define DEFAULT_SKEW 3

/* A function used by findfont that computes the 'closeness' between
   the request and a particular font */
int bdf_fontcmp(struct bdf_fontstyle_node *fs,const char *name,
		int size, int flags);

/* Utility to do a binary search for a font glyph */
const struct bdf_fontglyph *bdf_findglyph(const struct bdf_fontglyph *start, 
					  const struct bdf_fontglyph *end, s32 key);

/* Search for a glyph, falling back when necessary */
struct bdf_fontglyph const *bdf_getglyph(struct bdf_fontdesc *fd, int ch);


/********************************** Implementations ***/

void bdf_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pgpair *position,
		   hwrcolor col, int ch, struct pgquad *clip, s16 lgop, s16 angle) {
  int i,j;
  s16 cel_w; /* Total width of this character cel */
  struct bdf_fontglyph const *g;
  u8 *glyph;
  s16 u,v;   /* Displacement (in font coordinate space) of character from the cursor position */
  int pitch;

  g = bdf_getglyph(DATA,ch);
  
  cel_w = g->dwidth + DATA->boldw + DATA->interchar_space;
  
  u = g->x;
  v = DATA->font->ascent - g->h - g->y;
  
  /* Only render if the character has a bitmap */
  if (g->w && g->h) {
    glyph = (((u8 *)DATA->font->bitmaps)+g->bitmap);
    pitch = (g->w+7) >> 3;
    
    switch (angle) {
      
    case 0:
      /* The actual character */
      i=0;
      if (DATA->skew) 
	i = DATA->italicw;
      VID(charblit) (dest,glyph,position->x+u+i,position->y+v,g->w,g->h,DATA->skew,angle,col,
		     clip,lgop,pitch);
      
      /* bold */
      for (i++,j=0;j<DATA->boldw;i++,j++)
	VID(charblit) (dest,glyph,position->x+u+i,position->y+v,g->w,g->h,DATA->skew,angle,col,
		       clip,lgop,pitch);
      
      /* underline, overline, strikeout */
      if (DATA->hline>=0) {
	/* We must clip this! */
	
	s16 sx,sy,w;
	sx = position->x;
	sy = DATA->hline+(position->y);
	w  = cel_w;
	
	if (clip) {
	  if (sx < clip->x1) {
	    w -= clip->x1 - sx;
	    sx = clip->x1;
	  }
	  if (sx+w >= clip->x2)
	    w = clip->x2 - sx - 1;
	}	   
	
	if (w>0 && ( (!clip) || (sy >= clip->y1 && sy <= clip->y2) ))
	  VID(slab) (dest,position->x,DATA->hline+(position->y),cel_w,col,lgop);
      }	 
      break;
      
    case 90:
      /* FIXME: Support underline for rotated text */
      
      /* The actual character */
      i=0;
      if (DATA->skew) 
	i = DATA->italicw;
      VID(charblit) (dest,glyph,position->x+v,position->y-u-i,g->w,g->h,DATA->skew,angle,col,
		     clip,lgop,pitch);
      
      /* bold */
      for (i++,j=0;j<DATA->boldw;i++,j++)
	VID(charblit) (dest,glyph,position->x+v,position->y-u-i,g->w,g->h,DATA->skew,angle,col,
		       clip,lgop,pitch);
      break;
      
    case 180:
      /* The actual character */
      i=0;
      if (DATA->skew) 
	i = DATA->italicw;
      VID(charblit) (dest,glyph,position->x-v-i,position->y-u,g->w,g->h,DATA->skew,angle,col,
		     clip,lgop,pitch);
      
      /* bold */
      for (i++,j=0;j<DATA->boldw;i++,j++)
	VID(charblit) (dest,glyph,position->x-v-i,position->y-u,g->w,g->h,DATA->skew,angle,col,
		       clip,lgop,pitch);
      break;
      
#if 0
    case 270:
      /* underline, overline, strikeout */
      if (DATA->hline>=0)
	VID(slab) (dest,(mx)+DATA->hline,my,cel_w,col,lgop);
      
      /* The actual character */
      i=0;
      if (DATA->skew) 
	i = DATA->italicw;
      VID(charblit) (dest,glyph,mx,(my)+i,g->w,g->h,DATA->skew,angle,col,
		     clip,lgop,pitch);
      
      /* bold */
      for (i++,j=0;j<DATA->boldw;i++,j++)
	VID(charblit) (dest,glyph,mx,(my)+i,g->w,g->h,DATA->skew,angle,col,
		       clip,lgop,pitch);
      break;
#endif
      
    }
  }
  
  switch (angle) {
    
  case 0: 
    position->x += cel_w; 
    break;
    
  case 90: 
    position->y -= cel_w; 
    break;
    
  case 180: 
    position->x -= cel_w; 
    break;
    
  case 270: 
    position->y += cel_w; 
    break;
  }
}

void bdf_measure_char(struct font_descriptor *self, struct pgpair *position,
		      int ch, s16 angle) {
  int w = bdf_getglyph(DATA,ch)->dwidth + DATA->boldw + DATA->interchar_space;
  switch (angle) {

  case 0:
    position->x += w;
    break;

  case 90:
    position->y -= w;
    break;

  case 180:
    position->x -= w;
    break;

  case 270:
    position->y += w;
    break;
  }
}

g_error bdf_create(struct font_descriptor *self, const struct font_style *fs) {
  struct bdf_fontstyle_node *p;
  struct bdf_fontstyle_node *closest = NULL;
  struct font_descriptor *fd;
  int closeness = -1;
  int r;
  g_error e;
  int size = 0;
  int flags = PG_FSTYLE_DEFAULT;
  const char *name = NULL;

  if (fs) {
    size = fs->size;
    flags = fs->style;
    name = fs->name;
  }

  /* If the font size is zero, assume the default font size */
  if ((!size) && 
      (!iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,res[PGRES_DEFAULT_FONT]))) && fd) {
    struct font_metrics m;
    fd->lib->getmetrics(fd,&m);
    size = m.charcell.h;
  }
  
  e = g_malloc((void **) &self->data,sizeof(struct bdf_fontdesc));
  errorcheck;
  
  /* Initialize the DATA */
  memset(DATA,0,sizeof(struct bdf_fontdesc));
  DATA->hline = -1;
  DATA->style = flags;
  
  if (!(flags & PG_FSTYLE_FLUSH)) DATA->margin = 2;
  
  if (!bdf_fontstyles)
    return mkerror(PG_ERRT_IO,66);   /* Can't find any fonts */

  /* Now that the easy stuff is taken care of, find the font to use */
  p = bdf_fontstyles;
  while (p) {
    r = bdf_fontcmp(p,name,size,flags);
    if (r>closeness) {
      closeness = r;
      closest = p;
    }
    p = p->next;
  }
  DATA->fs = closest;
  
  if ((flags&PG_FSTYLE_BOLD) && (flags&PG_FSTYLE_ITALIC) && closest->bolditalic) {
    flags &= ~(PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC);
    DATA->font = closest->bolditalic;
  }
  else if ((flags&PG_FSTYLE_ITALIC) && closest->italic) {
    flags &= ~PG_FSTYLE_ITALIC;
    DATA->font = closest->italic;
  }
  else if ((flags&PG_FSTYLE_BOLD) && closest->bold) {
    flags &= ~PG_FSTYLE_BOLD;
    DATA->font = closest->bold;
  }
  else					
    DATA->font = closest->normal;
  
  if (flags&PG_FSTYLE_BOLD) DATA->boldw = closest->boldw;
  
  if (flags&PG_FSTYLE_DOUBLESPACE) DATA->interline_space = DATA->font->h;
  if (flags&PG_FSTYLE_DOUBLEWIDTH) DATA->interchar_space = bdf_getglyph(DATA,-1)->dwidth+DATA->boldw;
  if (flags&PG_FSTYLE_UNDERLINE) DATA->hline = closest->normal->ascent +
				   (closest->normal->descent >> 1);
  if (flags&PG_FSTYLE_STRIKEOUT) DATA->hline = closest->normal->ascent >> 1;
  
  if (flags&PG_FSTYLE_ITALIC2) {
    DATA->skew = DEFAULT_SKEW / 2;
    DATA->italicw = closest->normal->ascent / DATA->skew; 
  }
  else if (flags&PG_FSTYLE_ITALIC) {
    DATA->skew = DEFAULT_SKEW;
    DATA->italicw = closest->normal->ascent / DATA->skew; 
  }

  return success;
}

void bdf_destroy(struct font_descriptor *self) {
  g_free(DATA);
}
 
void bdf_getstyle(int i, struct font_style *fs) {
  struct bdf_fontstyle_node *n;
  memset(fs,0,sizeof(*fs));
  
  /* Iterate to the selected font style */
  for (n=bdf_fontstyles;i&&n;i--,n=n->next);
  
  /* If it's good, return the info. Otherwise, name stays zero */
  if (n) {    
    /* Put together representation flags */
    if (n->normal)
      fs->representation |= PG_FR_BITMAP_NORMAL;
    if (n->bold)
      fs->representation |= PG_FR_BITMAP_BOLD;
    if (n->italic)
      fs->representation |= PG_FR_BITMAP_ITALIC;
    if (n->bolditalic)
      fs->representation |= PG_FR_BITMAP_BOLDITALIC;

    fs->name = n->name;
    fs->size = n->size;
    fs->style = n->flags;
  }
}

void bdf_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
  m->charcell.w = DATA->font->w;
  m->charcell.h = DATA->font->h;
  m->ascent = DATA->font->ascent;
  m->descent = DATA->font->descent;
  m->margin = DATA->margin;
  m->linegap = 0;
  m->lineheight = m->ascent + m->descent;
}

void bdf_measure_string(struct font_descriptor *self, const struct pgstring *str,
			s16 angle, s16 *w, s16 *h) {
  def_measure_string(self,str,angle,w,h);
  if (angle==90 || angle==270)
    *h +=  DATA->italicw - DATA->interchar_space;
  else
    *w +=  DATA->italicw - DATA->interchar_space;
}

/********************************** Internal utilities ***/

/* Utility to do a binary search for a font glyph */
const struct bdf_fontglyph *bdf_findglyph(const struct bdf_fontglyph *start, 
					  const struct bdf_fontglyph *end, s32 key) {
  const struct bdf_fontglyph *middle;

  while (start<=end) {
    middle = start + ((end-start)>>1);
    if (key > middle->encoding)
      start = middle+1;
    else if (key < middle->encoding)
      end = middle-1;
    else
      return middle;
  }
  return NULL;
}

/* A function used by findfont that computes the 'closeness' between
   the request and a particular font */
int bdf_fontcmp(struct bdf_fontstyle_node *fs,const char *name, int size, int flags) {
  int result;
  int szdif;
  
  if (size) { 
    szdif = size-fs->size;
    if (szdif<0) szdif = 0-szdif;
    result = FCMP_SIZE(szdif);
  }    
  else {
    result = 0;
  }

  if (name && (!strcasecmp(name,fs->name))) result |= FCMP_NAME;
  if ((flags&PG_FSTYLE_FIXED)==(fs->flags&PG_FSTYLE_FIXED)) result |= FCMP_FIXEDVAR;
  if ((flags&PG_FSTYLE_DEFAULT) && (fs->flags&PG_FSTYLE_DEFAULT)) 
    result |= FCMP_DEFAULT;

  if (((flags&(PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC)) == PG_FSTYLE_BOLD)
      && fs->bold) result |= FCMP_STYLE;
  if (((flags&(PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC)) == PG_FSTYLE_ITALIC)
      && fs->italic) result |= FCMP_STYLE;
  if (((flags&(PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC)) == (PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC))
      && fs->bolditalic) result |= FCMP_STYLE;
  if ( ((flags&PG_FSTYLE_SYMBOL)==(fs->flags&PG_FSTYLE_SYMBOL)) &&
       ((flags&PG_FSTYLE_SUBSET)==(fs->flags&PG_FSTYLE_SUBSET)))
    result |= FCMP_TYPE;
  if ((flags&PG_FSTYLE_ENCODING_MASK) && (flags&PG_FSTYLE_ENCODING_MASK)==
      (fs->flags&PG_FSTYLE_ENCODING_MASK))
    result |= FCMP_CHARSET;

  return result;
}

struct bdf_fontglyph const *bdf_getglyph(struct bdf_fontdesc *fd, int ch) {
  const struct bdf_fontglyph *g, *start, *end;

  start = fd->font->glyphs;
  end = start + fd->font->numglyphs-1;

  /* Try the requested character */
  g = bdf_findglyph(start,end,ch);
  if (g) return g;

  /* Get the Unicode symbol for an unknown character */
  g = bdf_findglyph(start,end,0xFFFD);
  if (g) return g;

  /* Try an ASCII question mark */
  g = bdf_findglyph(start,end,'?');
  if (g) return g;

  /* The font's default character */
  g = bdf_findglyph(start,end,fd->font->defaultglyph);
  if (g) return g;

  /* shouldn't get here, but to avoid crashing return the 1st glyph */
  return fd->font->glyphs;
}


/********************************** Registration ***/

g_error bdf_regfunc(struct fontlib *f) {
  f->draw_char = &bdf_draw_char;
  f->measure_char = &bdf_measure_char;
  f->create = &bdf_create;
  f->destroy = &bdf_destroy;
  f->getstyle = &bdf_getstyle;
  f->getmetrics = &bdf_getmetrics;
  f->measure_string = &bdf_measure_string;
  return success;
}

/* The End */

