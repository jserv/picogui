/* $Id: font.c,v 1.54 2002/09/17 23:01:56 micahjd Exp $
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
#include <pgserver/pgstring.h>

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
	     s16 x,s16 y,hwrcolor col, const struct pgstring *txt,struct quad *clip,
	     s16 lgop, s16 angle) {
   int b,ch;
   struct pgstr_iterator p = PGSTR_I_NULL;

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
      
   while ((ch = pgstring_decode(txt,&p))) {
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
	if(fd->passwdc > 0) {    /* Is the font to be a password? */
	  /* We don't need to run the fd->decoder() here since fd->password
	   * is a character code, not a string.
	   */
	  outchar(dest,fd,&x,&y,col,fd->passwdc,clip,lgop,angle);
	}
	else
	  outchar(dest,fd,&x,&y,col,ch,clip,lgop,angle);
      }
   }
}

/* Measure the width and height of text as output by outtext
 * This includes the characters themselves , internal spacing,
 * and the margin as specified by fd->margin.
 * 
 * If txt is NULL, return the size of a typical character.
 */
void sizetext(struct fontdesc *fd, s16 *w, s16 *h, const struct pgstring *txt) {
  int o_w=0, ch;
  struct pgstr_iterator p = PGSTR_I_NULL;

  if (!(fd && w && h)) return;

  if (!txt) {
    *w = fd->font->w;
    *h = fd->font->h;
  }
  else {

    *w = fd->margin << 1;
    *h = (*w) + fd->font->h + fd->interline_space;

    while ((ch = pgstring_decode(txt,&p))) {
      if (ch=='\n') {
	*h += fd->font->h + fd->interline_space;
	if ((*w)>o_w) o_w = *w;
	*w = fd->margin << 1;
      }
      else if (ch!='\r') {
	if(fd->passwdc > 0)      /* If the font is set to a password */
	  /* We don't need to run the fd->decoder() here since fd->password
	   * is a character code, not a string.
	   */
	  outchar_fake(fd,w,fd->passwdc);
	else
	  outchar_fake(fd,w,ch);
      }
    }
    if ((*w)<o_w) *w = o_w;
    *w -= fd->interchar_space;
    *w += fd->italicw;
  }    

  VID(font_sizetext_hook)(fd,w,h,txt);
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
       (!iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,res[PGRES_DEFAULT_FONT]))) && fd)
     size = fd->font->h;
     
   e = g_malloc((void **) &fd,sizeof(struct fontdesc));
   errorcheck;
   e = mkhandle(pfh,PG_TYPE_FONTDESC,owner,fd);
   errorcheck;
   
   /* Initialize the fd */
   memset(fd,0,sizeof(struct fontdesc));
   fd->hline = -1;
   fd->style = flags;
   
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

#ifdef CONFIG_WIDGET_TERMINAL
/* The workhorse of the terminal widget. 3 params:
 * 1. buffer handle,
 * 2. buffer width and offset (0xWWWWOOOO)
 * 3. handle to textcolors array
 */
void textgrid_render(struct groprender *r, struct gropnode *n) {
  int buffersz,bufferw,bufferh;
  int celw,celh,charw,charh,offset;
  int i;
  unsigned char attr;
  u32 *textcolors;
  s16 temp_x;
  struct gropnode bn;
  struct groprender br;
  struct pgstr_iterator stri = PGSTR_I_NULL;
  struct pgstring *str;
  struct fontdesc *fd;

  if (iserror(rdhandle((void**)&str,PG_TYPE_PGSTRING,-1,
		       n->param[0])) || !str)
    return;
  if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
		       r->hfont)) || !fd)
    return;
    
  /* Set up background color node (we'll need it later) */
  memset(&bn,0,sizeof(bn));
  bn.type = PG_GROP_RECT;
  bn.flags = PG_GROPF_COLORED;
  
  /* Read textcolors parameter */
  if (iserror(rdhandle((void**)&textcolors,PG_TYPE_PALETTE,-1,
		       n->param[2])) || !textcolors)
    return;
  if (textcolors[0] < 16)    /* Make sure it's big enough */
    return;
  textcolors++;              /* Skip length entry */
  
  /* Should be fine for fixed width fonts
   * and pseudo-acceptable for others? */
  celw      = fd->font->w;  
  celh      = fd->font->h;
  
  /* n->param[1]'s low u16 is the buffer width, the high u16 is
   * an offset from the beginning of the buffer.
   */
  bufferw = n->param[1] >> 16;
  buffersz = str->num_chars - (n->param[1] & 0xFFFF);
  pgstring_seek(str,&stri, n->param[1] & 0xFFFF);
  bufferh = buffersz / bufferw;
  if (buffersz<=0) return;
  
  charw     = n->r.w/celw;
  charh     = n->r.h/celh;
  offset    = bufferw - charw;
  if (offset<0) {
    offset = 0;
    charw = bufferw;
  }
  if (charh>bufferh)
    charh = bufferh;
  
  r->orig.x = n->r.x;
  for (;charh;charh--,n->r.y+=celh,pgstring_seek(str,&stri,offset)) {
      
    /* Skip the entire line if it's clipped out */
    if (n->r.y > r->clip.y2 ||
	(n->r.y+celh) < r->clip.y1) {
      pgstring_seek(str,&stri,bufferw);
      continue;
    }
    
    for (n->r.x=r->orig.x,i=charw;i;i--) {
      void *metadata;
      u32 ch;
      
      /* Decode one character/attribute pair */
      ch = pgstring_decode_meta(str,&stri,&metadata);
      attr = (u8)(u32)metadata;
      
      /* Background color (clipped rectangle) */
      if ((attr & 0xF0)!=0) {
	br = *r;
	bn.r.x = n->r.x;
	bn.r.y = n->r.y;
	bn.r.w = celw;
	bn.r.h = celh;
	bn.param[0] = textcolors[attr>>4];
	gropnode_clip(&br,&bn);
	gropnode_draw(&br,&bn);
      }
      
      temp_x = n->r.x;
      n->r.x += celw;
      outchar(r->output, fd, &temp_x, &n->r.y, 
	      textcolors[attr & 0x0F],
	      ch, &r->clip,r->lgop, 0);
    }
  }
}
#endif

/* The End */






