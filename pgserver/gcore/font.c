/* $Id: font.c,v 1.47 2002/01/18 16:42:59 pney Exp $
 *
 * font.c - loading and rendering fonts
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

#include <string.h>

#include <pgserver/font.h>
#include <pgserver/video.h>
#include <pgserver/appmgr.h>
#include <pgserver/render.h>

/* This defines how italic the generated italic is */
#define DEFAULT_SKEW 3

/* Various bits turned on for matches in fontcmp.  The order
 * of these bits defines the priority of the various
 * attributes
 */
#define FCMP_TYPE     (1<<14)
#define FCMP_CHARSET  (1<<13)
#define FCMP_FIXEDVAR (1<<12)
#define FCMP_SIZE(x)  ((0xFF-(x&0xFF))<<3)   /* This macro is passed the
						difference in size between the
						request and the actual font */
#define FCMP_NAME     (1<<2)
#define FCMP_STYLE    (1<<1)
#define FCMP_DEFAULT  (1<<0)

/* A function used by findfont that computes the 'closeness' between
   the request and a particular font */
int fontcmp(struct fontstyle_node *fs,const u8 *name, int size, stylet flags);

/* Utility to do a binary search for a font glyph */
const struct fontglyph *font_findglyph(const struct fontglyph *start, 
				 const struct fontglyph *end, s32 key) {
  const struct fontglyph *middle;

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

/* Outputs a character. It also updates (*x,*y) as a cursor position. */
void outchar(hwrbitmap dest, struct fontdesc *fd,
	     s16 *x, s16 *y,hwrcolor col,int c,struct quad *clip,
	     s16 lgop, s16 angle) {
   int i,j;
   s16 cel_w; /* Total width of this character cel */
   struct fontglyph const *g;
   u8 *glyph;
   s16 u,v;   /* Displacement (in font coordinate space) of character from the cursor position */

   VID(font_outchar_hook)(&dest,&fd,x,y,&col,&c,&clip,&lgop,&angle);

   /* Test this here so the hook can set lgop to disable normal rendering */
   if (lgop == PG_LGOP_NULL)
     return;

   g = VID(font_getglyph)(fd,c);

   cel_w = g->dwidth + fd->boldw + fd->interchar_space;
   
   u = g->x;
   v = fd->font->ascent - g->h - g->y;

   /* Only render if the character has a bitmap */
   if (g->w && g->h) {
      glyph = (((u8 *)fd->font->bitmaps)+g->bitmap);

      switch (angle) {
    
       case 0:
	 /* The actual character */
	 i=0;
	 if (fd->skew) 
	   i = fd->italicw;
	 VID(charblit) (dest,glyph,*x+u+i,*y+v,g->w,g->h,fd->skew,angle,col,
			clip,lgop);
	 
	 /* bold */
	 for (i++,j=0;j<fd->boldw;i++,j++)
	   VID(charblit) (dest,glyph,*x+u+i,*y+v,g->w,g->h,fd->skew,angle,col,
			  clip,lgop);

	 /* underline, overline, strikeout */
	 if (fd->hline>=0) {
	   /* We must clip this! */

	   s16 sx,sy,w;
	   sx = *x;
	   sy = fd->hline+(*y);
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
	     VID(slab) (dest,*x,fd->hline+(*y),cel_w,col,lgop);
	 }	 
	 break;
	 
       case 90:
	 /* FIXME: Support underline for rotated text */

	 /* The actual character */
	 i=0;
	 if (fd->skew) 
	   i = fd->italicw;
	 VID(charblit) (dest,glyph,*x+v,*y-u-i,g->w,g->h,fd->skew,angle,col,
			clip,lgop);
	 
	 /* bold */
	 for (i++,j=0;j<fd->boldw;i++,j++)
	   VID(charblit) (dest,glyph,*x+v,*y-u-i,g->w,g->h,fd->skew,angle,col,
			  clip,lgop);
	 break;
	 
       case 180:
	 /* The actual character */
	 i=0;
	 if (fd->skew) 
	   i = fd->italicw;
	 VID(charblit) (dest,glyph,*x-v-i,*y-u,g->w,g->h,fd->skew,angle,col,
			clip,lgop);
	 
	 /* bold */
	 for (i++,j=0;j<fd->boldw;i++,j++)
	   VID(charblit) (dest,glyph,*x-v-i,*y-u,g->w,g->h,fd->skew,angle,col,
			  clip,lgop);
	 break;

#if 0              /* 270-degree code not needed yet */	 
       case 270:
	 /* underline, overline, strikeout */
	 if (fd->hline>=0)
	   VID(slab) (dest,(mx)+fd->hline,my,cel_w,col,lgop);
	 
	 /* The actual character */
	 i=0;
	 if (fd->skew) 
	   i = fd->italicw;
	 VID(charblit) (dest,glyph,mx,(my)+i,g->w,g->h,fd->skew,angle,col,
			clip,lgop);
	 
	 /* bold */
	 for (i++,j=0;j<fd->boldw;i++,j++)
	   VID(charblit) (dest,glyph,mx,(my)+i,g->w,g->h,fd->skew,angle,col,
			  clip,lgop);
	 break;
#endif
	 
      }
   }
   
   switch (angle) {

    case 0: 
      *x += cel_w; 
      break;
      
    case 90: 
      *y -= cel_w; 
      break;
      
    case 180: 
      *x -= cel_w; 
      break;

#if 0              /* 270-degree code not needed yet */	       
    case 270: 
      *y += cel_w; 
      break;
#endif
      
   }
}

/* A version of outchar that doesn't make any
   output. Used for sizetext */
void outchar_fake(struct fontdesc *fd, s16 *x,int  c) {
  *x += VID(font_getglyph)(fd,c)->dwidth + fd->boldw + fd->interchar_space;
}

/* Output text, interpreting '\n' but no other control chars.
 * This function does add the margin as specified by fd->margin.
 */
void outtext(hwrbitmap dest, struct fontdesc *fd,
	     s16 x,s16 y,hwrcolor col, const u8 *txt,struct quad *clip,
	     s16 lgop, s16 angle) {
   int b,ch;

   VID(font_outtext_hook)(&dest,&fd,&x,&y,&col,&txt,&clip,&lgop,&angle);
   
   switch (angle) {
      
    case 0:
      x += fd->margin;
      y += fd->margin;
      b = x;
      break;
      
    case 90:
      x += fd->margin;
      y -= fd->margin;
      b = y;
      break;
      
    case 180:
      x -= fd->margin;
      y -= fd->margin;
      b = x;
      break;
      
    case 270:
      x -= fd->margin;
      y += fd->margin;
      b = y;
      break;
            
   }
      
   while ((ch = fd->decoder(&txt))) {
      if (ch=='\n')
	switch (angle) {
	 
	 case 0:
	   y += fd->font->h+fd->interline_space;
	   x = b;
	   break;
	   
	 case 90:
	   x += fd->font->h+fd->interline_space;
	   y = b;
	   break;
	   
	 case 180:
	   y -= fd->font->h+fd->interline_space;
	   x = b;
	   break;
	   
	 case 270:
	   x -= fd->font->h+fd->interline_space;
	   y = b;
	   break;
	   
	}
      else if (ch!='\r') {
	if(fd->passwdc > 0)    /* Is the font to be a password? */
          outchar(dest,fd,&x,&y,col,'*',clip,lgop,angle);
	else
	  outchar(dest,fd,&x,&y,col,ch,clip,lgop,angle);
      }
   }
}

/* Measure the width and height of text as output by outtext
 * This includes the characters themselves , internal spacing,
 * and the margin as specified by fd->margin
 */
void sizetext(struct fontdesc *fd, s16 *w, s16 *h, const u8 *txt) {
  int o_w=0, ch;
  const u8 *original_txt = txt;

  if (!(fd && txt && w && h)) return;

  *w = fd->margin << 1;
  *h = (*w) + fd->font->h + fd->interline_space;

  while ((ch = fd->decoder(&txt))) {
    if (ch=='\n') {
      *h += fd->font->h + fd->interline_space;
      if ((*w)>o_w) o_w = *w;
      *w = fd->margin << 1;
    }
    else if (ch!='\r') {
      if(fd->passwdc > 0)      /* If the font is set to a password */
	outchar_fake(fd,w,'*');
      else
	outchar_fake(fd,w,ch);
    }
  }
  if ((*w)<o_w) *w = o_w;
  *w -= fd->interchar_space;
  *w += fd->italicw;

  VID(font_sizetext_hook)(fd,w,h,original_txt);
}

/* Find a font and fill in the fontdesc structure */
g_error findfont(handle *pfh,int owner, const u8 *name,int size,stylet flags) {
   struct fontstyle_node *p;
   struct fontstyle_node *closest = NULL;
   struct fontdesc *fd; 
   int closeness = -1;
   int r;
   g_error e;
   
   /* If the font size is zero, assume the default font size */
   if ((!size) && 
       (!iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,defaultfont))) && fd)
     size = fd->font->h;
     
   e = g_malloc((void **) &fd,sizeof(struct fontdesc));
   errorcheck;
   e = mkhandle(pfh,PG_TYPE_FONTDESC,owner,fd);
   errorcheck;
   
   /* Initialize the fd */
   memset(fd,0,sizeof(struct fontdesc));
   fd->hline = -1;
   fd->style = flags;
   fd->decoder = &decode_ascii;
   
   if (!(flags & PG_FSTYLE_FLUSH)) fd->margin = 2;
   
   /* Normally having no fonts compiled in is a very bad thing.
    * If the video driver doesn't need them, though, as in the case of
    * the ncurses driver then it's ok and we can just skip this junk
    * and let the driver handle it.
    */
   if (fontstyles) {
   
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
      
      if (flags&PG_FSTYLE_DOUBLESPACE) fd->interline_space = fd->font->h;
      if (flags&PG_FSTYLE_DOUBLEWIDTH) fd->interchar_space =
					 VID(font_getglyph)(fd,-1)->dwidth+
					 fd->boldw;
      if (flags&PG_FSTYLE_UNDERLINE) fd->hline = closest->normal->ascent +
				       (closest->normal->descent >> 1);
      if (flags&PG_FSTYLE_STRIKEOUT) fd->hline = closest->normal->ascent >> 1;
      
      if (flags&PG_FSTYLE_ITALIC2) {
	 fd->skew = DEFAULT_SKEW / 2;
	 fd->italicw = closest->normal->ascent / fd->skew; 
      }
      else if (flags&PG_FSTYLE_ITALIC) {
	 fd->skew = DEFAULT_SKEW;
	 fd->italicw = closest->normal->ascent / fd->skew; 
      }

      if (closest->flags & PG_FSTYLE_ENCODING_UNICODE)
	fd->decoder = &decode_utf8;
   }
   
   /* Let the video driver transmogrify it if necessary */
   VID(font_newdesc) (fd,name,size,flags);
   
   return success;
}

/* A function used by findfont that computes the 'closeness' between
   the request and a particular font */
int fontcmp(struct fontstyle_node *fs,const u8 *name, int size, stylet flags) {
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

/* Decode one character from the specified UTF-8 string, 
 * advancing the pointer 
 *
 * For a description of the UTF-8 standard, see:
 * http://www.cl.cam.ac.uk/~mgk25/unicode.html
 */
int decode_utf8(const u8 **str) {
  int ch = 0;
  u8 b;
  int length,i;


  /* The first character determines the sequence's length */
  ch = *((*str)++);

  if (!(ch & 0x80))
    /* 1-byte code, return it as-is */
    return ch;
  else if ((ch & 0xC0) == 0x80)
    return -1;
  else if (!(ch & 0x20)) {
    length = 2;
    ch &= 0x1F;
  }
  else if (!(ch & 0x10)) {
    length = 3;
    ch &= 0x0F;
  }
  else if (!(ch & 0x08)) {
    length = 4;
    ch &= 0x07;
  }
  else if (!(ch & 0x04)) {
    length = 5;
    ch &= 0x03;
  }
  else if (!(ch & 0x02)) {
    length = 6;
    ch &= 0x01;
  }
  else
    /* Invalid code */
    return -1;

  /* Decode each byte of the sequence */
  for (i=1;i<length;i++) {
    b = *((*str)++);
    if (!b)
      return 0;
    if ((b & 0xC0) != 0x80) {
      (*str)--;
      return -1;
    }
    ch <<= 6;
    ch |= b & 0x3F;
  }  

  /* Make sure it is a unique representation */
  if (ch <= 0x7F && length > 1) return -1;
  if (ch <= 0x7FF && length > 2) return -1;
  if (ch <= 0xFFFF && length > 3) return -1;
  if (ch <= 0x1FFFFF && length > 4) return -1;
  if (ch <= 0x3FFFFFF && length > 5) return -1;

  return ch;
}

/* Simple decoder for 8-bit text */
int decode_ascii(const u8 **str) {
  return *((*str)++);
}

/* The End */






