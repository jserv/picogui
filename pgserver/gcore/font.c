/* $Id: font.c,v 1.12 2001/01/07 04:06:17 micahjd Exp $
 *
 * font.c - loading and rendering fonts
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <string.h>

#include <pgserver/font.h>
#include <pgserver/g_malloc.h>
#include <pgserver/g_error.h>
#include <pgserver/video.h>

/* This defines how italic the generated italic is */
#define DEFAULT_SKEW 3

/* Various bits turned on for matches in fontcmp.  The order
 * of these bits defines the priority of the various
 * attributes
 */
#define FCMP_FIXEDVAR (1<<14)
#define FCMP_STYLE    (1<<13)
#define FCMP_SIZE(x)  ((0xFF-(x&0xFF))<<4)   /* This macro is passed the
						difference in size between the
						request and the actual font */
#define FCMP_CHARSET  (1<<3)
#define FCMP_TYPE     (1<<2)
#define FCMP_DEFAULT  (1<<1)
#define FCMP_NAME     (1<<0)

/* A function used by findfont that computes the 'closeness' between
   the request and a particular font */
int fontcmp(struct fontstyle_node *fs,char *name, int size, stylet flags);

/* Outputs a character. It also updates (*x,*y) as a cursor position.
 * To use this for measuring text but not actually outputting it, set
 * clip to NULL
 */
void outchar(struct fontdesc *fd,
	     int *x, int *y,hwrcolor col,char c,struct cliprect *clip) {
  int i,j;
  int cel_w; /* Total width of this character cel */
  int glyph_w,glyph_h;
  unsigned char *glyph;

  glyph_w = fd->font->vwtab[c];
  cel_w = glyph_w + fd->font->hspace + fd->boldw + fd->interchar_space;
  if (fd->font->trtab[c] >= 0) {
    glyph = (((unsigned char *)fd->font->bitmaps)+fd->font->trtab[c]);
    glyph_h = fd->font->h;

    /* underline, overline, strikeout */
    if (fd->hline>=0)
      (*vid->slab)(*x,fd->hline+(*y),cel_w,fd->hline_c);
    
    /* The actual character */
    i=0;
    if (fd->skew) 
      i = fd->italicw;
    (*vid->charblit)(glyph,(*x)+i,*y,glyph_w,glyph_h,fd->skew,col,clip);
    
    /* bold */
    for (i++,j=0;j<fd->boldw;i++,j++)
      (*vid->charblit)(glyph,(*x)+i,*y,glyph_w,glyph_h,fd->skew,col,clip);
  }
  *x += cel_w;  
}

/* A version of outchar that doesn't make any
   output. Used for sizetext */
void outchar_fake(struct fontdesc *fd,
	     int *x, int *y,char c) {
  int i,j;
  int cel_w; /* Total width of this character cel */
  int glyph_w,glyph_h;
  unsigned char *glyph;

  glyph_w = fd->font->vwtab[c];
  cel_w = glyph_w + fd->font->hspace + fd->boldw + fd->interchar_space;
  if (fd->font->trtab[c] >= 0) {
    glyph = (((unsigned char *)fd->font->bitmaps)+fd->font->trtab[c]);
    glyph_h = fd->font->h;
  }
  *x += cel_w;  
}

/* Output text, interpreting '\n' but no other control chars.
 * This function does add the margin as specified by fd->margin.
*/
void outtext(struct fontdesc *fd,
	     int x,int y,hwrcolor col,char *txt,struct cliprect *clip) {
  int xbase;
  x += fd->margin;
  y += fd->margin;
  xbase = x; 
   
  if (!(fd && txt)) return;

  while (*txt) {
    if (*txt=='\n') {
      y += fd->font->h+fd->font->vspace+fd->interline_space;
      x = xbase;
    }
    else
      outchar(fd,&x,&y,col,*txt,clip);
    txt++;
  }
}

/* Like outtext, but print vertically. Origin is at bottom-left */
void outtext_v(struct fontdesc *fd,
	     int x,int y,hwrcolor col,char *txt,struct cliprect *clip) {
  int ybase;
  x += fd->margin;
  y -= fd->margin;
  ybase = y; 
   
  if (!(fd && txt)) return;

  while (*txt) {
    if (*txt=='\n') {
      x += fd->font->h+fd->font->vspace+fd->interline_space;
      y = ybase;
    }
    else {
      int i,j;
      int cel_w; /* Total width of this character cel */
      int glyph_w,glyph_h;
      unsigned char *glyph;
      char c = *txt;
      
      glyph_w = fd->font->vwtab[c];
      cel_w = glyph_w + fd->font->hspace + fd->boldw + fd->interchar_space;
      if (fd->font->trtab[c] >= 0) {
	glyph = (((unsigned char *)fd->font->bitmaps)+fd->font->trtab[c]);
	glyph_h = fd->font->h;
	
	/* underline, overline, strikeout */
	if (fd->hline>=0)
	  (*vid->bar)(y,fd->hline+y,cel_w,fd->hline_c);
	
	/* The actual character */
	i=0;
	if (fd->skew) 
	  i = fd->italicw;
	(*vid->charblit_v)(glyph,x,y-i,glyph_w,glyph_h,fd->skew,col,clip);
	
	/* bold */
	for (i++,j=0;j<fd->boldw;i++,j++)
	  (*vid->charblit_v)(glyph,x,y-i,glyph_w,glyph_h,fd->skew,col,clip);
      }
      y -= cel_w;  
    }
    txt++;
  }
}

/* Measure the width and height of text as output by outtext
 * This includes the characters themselves , internal spacing,
 * and the margin as specified by fd->margin
 */
void sizetext(struct fontdesc *fd, int *w, int *h, char *txt) {
  int o_w=0;
  *w = fd->margin << 1;
  *h = (*w) + fd->fs->ulineh;
   
  if (!(fd && txt && w && h)) return;

  while (*txt) {
    if ((*txt)=='\n') {
      *h += fd->font->h + fd->font->vspace + fd->interline_space;
      if ((*w)>o_w) o_w = *w;
      *w = fd->margin << 1;
    }
    else {
      outchar_fake(fd,w,h,*txt);
    }
    txt++;
  }
  if ((*w)<o_w) *w = o_w;
  *w -= fd->font->hspace + fd->interchar_space;
  *w += fd->italicw;
}

/* Find a font and fill in the fontdesc structure */
g_error findfont(handle *pfh,int owner, char *name,int size,stylet flags) {
  struct fontstyle_node *p;
  struct fontstyle_node *closest = NULL;
  struct fontdesc *fd; 
  int closeness = -1;
  int r;
  g_error e;

  e = g_malloc((void **) &fd,sizeof(struct fontdesc));
  errorcheck;
  e = mkhandle(pfh,PG_TYPE_FONTDESC,owner,fd);
  errorcheck;

  /* Initialize the fd */
  memset(fd,0,sizeof(struct fontdesc));
  fd->hline = -1;
  fd->hline_c = (*vid->color_pgtohwr)(0x000000);
  
  if (!(flags & PG_FSTYLE_FLUSH)) fd->margin = 2;
  if (flags & PG_FSTYLE_GRAYLINE) {
    fd->hline_c = (*vid->color_pgtohwr)(0x808080);
    flags |= PG_FSTYLE_UNDERLINE;
  }

  /* Now that the easy stuff is taken care of, find the font to use */
  p = fontstyles;
  while (p) {
    r = fontcmp(p,name,size,flags);
    if (r>closeness) {
      closeness = r;
      closest = p;
    }
    p = p->next;
  }
  fd->fs = closest;

  if ((flags&PG_FSTYLE_BOLD) && (flags&PG_FSTYLE_ITALIC) && closest->bolditalic) {
    flags &= ~(PG_FSTYLE_BOLD|PG_FSTYLE_ITALIC);
    fd->font = closest->bolditalic;
  }
  else if ((flags&PG_FSTYLE_ITALIC) && closest->italic) {
    flags &= ~PG_FSTYLE_ITALIC;
    fd->font = closest->italic;
  }
  else if ((flags&PG_FSTYLE_BOLD) && closest->bold) {
    flags &= ~PG_FSTYLE_BOLD;
    fd->font = closest->bold;
  }
  else					
    fd->font = closest->normal;

  if (flags&PG_FSTYLE_BOLD) fd->boldw = closest->boldw;

  if (flags&PG_FSTYLE_DOUBLESPACE) fd->interline_space = 
				  fd->font->h+fd->font->vspace;
  if (flags&PG_FSTYLE_DOUBLEWIDTH) fd->interchar_space =
				  fd->font->vwtab[' ']+fd->font->hspace+
				  fd->boldw;
  if (flags&PG_FSTYLE_UNDERLINE) fd->hline = closest->ulineh;
  if (flags&PG_FSTYLE_STRIKEOUT) fd->hline = closest->slineh;

  if (flags&PG_FSTYLE_ITALIC2) {
    fd->skew = DEFAULT_SKEW / 2;
    fd->italicw = closest->ulineh / fd->skew; 
  }
  else if (flags&PG_FSTYLE_ITALIC) {
    fd->skew = DEFAULT_SKEW;
    fd->italicw = closest->ulineh / fd->skew; 
  }

  return sucess;
}

/* A function used by findfont that computes the 'closeness' between
   the request and a particular font */
int fontcmp(struct fontstyle_node *fs,char *name, int size, stylet flags) {
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
  if ( ((flags&PG_FSTYLE_EXTENDED)==(fs->flags&PG_FSTYLE_EXTENDED)) &&
       ((flags&PG_FSTYLE_IBMEXTEND)==(fs->flags&PG_FSTYLE_IBMEXTEND)))
    result |= FCMP_CHARSET;

  return result;
}

/* The End */






