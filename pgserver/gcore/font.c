/* $Id: font.c,v 1.28 2001/10/02 08:47:50 micahjd Exp $
 *
 * font.c - loading and rendering fonts
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
#define FCMP_FIXEDVAR (1<<13)
#define FCMP_CHARSET  (1<<12)
#define FCMP_SIZE(x)  ((0xFF-(x&0xFF))<<3)   /* This macro is passed the
						difference in size between the
						request and the actual font */
#define FCMP_NAME     (1<<2)
#define FCMP_STYLE    (1<<1)
#define FCMP_DEFAULT  (1<<0)

/* A function used by findfont that computes the 'closeness' between
   the request and a particular font */
int fontcmp(struct fontstyle_node *fs,char *name, int size, stylet flags);

/* Small helper function used in outchar_fake and outchar */
struct fontglyph const *font_getglyph(struct fontdesc *fd, u8 c) {
  s16 ch = c;

  ch -= fd->font->beginglyph;
  if (ch < 0 || ch >= fd->font->numglyphs)
    ch = fd->font->defaultglyph - fd->font->beginglyph;
  return fd->font->glyphs+ch;
}

/* Outputs a character. It also updates (*x,*y) as a cursor position. */
void outchar(hwrbitmap dest, struct fontdesc *fd,
	     s16 *x, s16 *y,hwrcolor col,unsigned char c,struct quad *clip,
	     bool fill, hwrcolor bg, s16 lgop, s16 angle) {
   int i,j;
   s16 cel_w; /* Total width of this character cel */
   struct fontglyph const *g = font_getglyph(fd,c);
   u8 *glyph;
   s16 mx,my; /* Character positions we are free to munge (screen logical coordinates) */
   s16 u,v;   /* Displacement (in font coordinate space) of character from the cursor position */

   cel_w = g->dwidth + fd->boldw + fd->interchar_space;
   
   mx = *x;
   my = *y;

   u = g->x;
   v = fd->font->ascent - g->h - g->y;

   /* Only render if the character has a bitmap */
   if (g->w && g->h) {
      glyph = (((unsigned char *)fd->font->bitmaps)+g->bitmap);

      switch (angle) {
    
       case 0:
	 mx += u;
	 my += v;

	 /* underline, overline, strikeout */
	 if (fd->hline>=0) {
	   /* We must clip this! */

	   s16 sx,sy,w;
	   sx = mx;
	   sy = fd->hline+(my);
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
	     VID(slab) (dest,mx,fd->hline+(my),cel_w,fd->hline_c,lgop);
	 }	 

	 /* The actual character */
	 i=0;
	 if (fd->skew) 
	   i = fd->italicw;
	 VID(charblit) (dest,glyph,(mx)+i,my,g->w,g->h,fd->skew,angle,col,
			clip,fill,bg,lgop);
	 
	 /* bold */
	 for (i++,j=0;j<fd->boldw;i++,j++)
	   VID(charblit) (dest,glyph,(mx)+i,my,g->w,g->h,fd->skew,angle,col,
			  clip,fill,bg,lgop);
	 break;
	 
       case 90:
	 mx += v;
	 my -= u;

	 /* underline, overline, strikeout */
	 if (fd->hline>=0)
	   VID(bar) (dest,(mx)+fd->hline,(my)-cel_w+1,cel_w,fd->hline_c,lgop);
	 
	 /* The actual character */
	 i=0;
	 if (fd->skew) 
	   i = fd->italicw;
	 VID(charblit) (dest,glyph,mx,(my)-i,g->w,g->h,fd->skew,angle,col,
			clip,fill,bg,lgop);
	 
	 /* bold */
	 for (i++,j=0;j<fd->boldw;i++,j++)
	   VID(charblit) (dest,glyph,mx,(my)-i,g->w,g->h,fd->skew,angle,col,
			  clip,fill,bg,lgop);
	 break;
	 
       case 180:
	 mx -= v;
	 my -= u;

	 /* underline, overline, strikeout */
	 if (fd->hline>=0)
	   VID(slab) (dest,(mx)+cel_w-1,(my)-fd->hline,cel_w,fd->hline_c,lgop);
	 
	 /* The actual character */
	 i=0;
	 if (fd->skew) 
	   i = fd->italicw;
	 VID(charblit) (dest,glyph,(mx)-i,my,g->w,g->h,fd->skew,angle,col,
			clip,fill,bg,lgop);
	 
	 /* bold */
	 for (i++,j=0;j<fd->boldw;i++,j++)
	   VID(charblit) (dest,glyph,(mx)-i,my,g->w,g->h,fd->skew,angle,col,
			  clip,fill,bg,lgop);
	 break;
	 
       case 270:
	 /* underline, overline, strikeout */
	 if (fd->hline>=0)
	   VID(slab) (dest,(mx)+fd->hline,my,cel_w,fd->hline_c,lgop);
	 
	 /* The actual character */
	 i=0;
	 if (fd->skew) 
	   i = fd->italicw;
	 VID(charblit) (dest,glyph,mx,(my)+i,g->w,g->h,fd->skew,angle,col,
			clip,fill,bg,lgop);
	 
	 /* bold */
	 for (i++,j=0;j<fd->boldw;i++,j++)
	   VID(charblit) (dest,glyph,mx,(my)+i,g->w,g->h,fd->skew,angle,col,
			  clip,fill,bg,lgop);
	 break;
	 
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
      
    case 270: 
      *y += cel_w; 
      break;
      
   }
}

/* A version of outchar that doesn't make any
   output. Used for sizetext */
void outchar_fake(struct fontdesc *fd, s16 *x,unsigned char c) {
  *x += font_getglyph(fd,c)->dwidth + fd->boldw + fd->interchar_space;
}

/* Output text, interpreting '\n' but no other control chars.
 * This function does add the margin as specified by fd->margin.
 */
void outtext(hwrbitmap dest, struct fontdesc *fd,
	     s16 x,s16 y,hwrcolor col,char *txt,struct quad *clip,
	     bool fill, hwrcolor bg, s16 lgop, s16 angle) {
   int b;
   
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
      
   while (*txt) {
      if (*txt=='\n')
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
      else
	outchar(dest,fd,&x,&y,col,(unsigned char) *txt,clip,fill,bg,lgop,angle);
      txt++;
   }
}

/* Measure the width and height of text as output by outtext
 * This includes the characters themselves , internal spacing,
 * and the margin as specified by fd->margin
 */
void sizetext(struct fontdesc *fd, s16 *w, s16 *h, char *txt) {
  int o_w=0;
   
  if (!(fd && txt && w && h)) return;

  /* No text, no size */
  if (!*txt) {
    *w = 0;
    *h = 0;
    return;
  }

  *w = fd->margin << 1;
  *h = (*w) + fd->fs->ulineh;

  while (*txt) {
    if ((*txt)=='\n') {
      *h += fd->font->h + fd->interline_space;
      if ((*w)>o_w) o_w = *w;
      *w = fd->margin << 1;
    }
    else {
      outchar_fake(fd,w,*txt);
    }
    txt++;
  }
  if ((*w)<o_w) *w = o_w;
  *w -= fd->interchar_space;
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
   fd->hline_c = VID(color_pgtohwr) (0x000000);
   fd->style = flags;
   
   if (!(flags & PG_FSTYLE_FLUSH)) fd->margin = 2;
   if (flags & PG_FSTYLE_GRAYLINE) {
      fd->hline_c = VID(color_pgtohwr) (0x808080);
      flags |= PG_FSTYLE_UNDERLINE;
   }
   
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
					 font_getglyph(fd,-1)->dwidth+
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
      
   }
   
   /* Let the video driver transmogrify it if necessary */
   VID(font_newdesc) (fd,name,size,flags);
   
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






