/* $Id: dvbl_font.c,v 1.1 2002/04/03 08:08:41 micahjd Exp $
 *
 * dvbl_font.c - This file is part of the Default Video Base Library,
 *               providing the basic video functionality in picogui but
 *               easily overridden by drivers.
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
 */

#include <pgserver/common.h>

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/render.h>

/* Small helper function used in outchar_fake and outchar 
 *
 * If it's an invalid character, first try replacing it with the Unicode
 * symbol for an unknown character, then if that doesn't work use a question
 * mark. If the question mark doesn't work either, use the font's default
 * character.
 */
struct fontglyph const *def_font_getglyph(struct fontdesc *fd, int ch) {
  const struct fontglyph *g, *start, *end;

  start = fd->font->glyphs;
  end = start + fd->font->numglyphs-1;

  /* Try the requested character */
  g = font_findglyph(start,end,ch);
  if (g) return g;

  /* Get the Unicode symbol for an unknown character */
  g = font_findglyph(start,end,0xFFFD);
  if (g) return g;

  /* Try an ASCII question mark */
  g = font_findglyph(start,end,'?');
  if (g) return g;

  /* The font's default character */
  g = font_findglyph(start,end,fd->font->defaultglyph);
  if (g) return g;

  /* shouldn't get here, but to avoid crashing return the 1st glyph */
  return fd->font->glyphs;
}


void def_charblit_0(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
		    s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,
		    s16 lgop) {
  int bw = w;
  int iw,hc,x;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || dest_y>clip->y2 || (dest_x+w)<clip->x1 || 
      (dest_y+h)<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y2<(dest_y+h))
      h = clip->y2-dest_y+1;
    if (clip->y1>dest_y) {
      hc = clip->y1-dest_y; /* Do it this way so skewing doesn't mess up when clipping */
      while (lines < hc && olines) {
	lines += olines;
	dest_x--;
      }
      dest_y += hc;
      chardat += hc*bw;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->x1>dest_x)
      xmin = clip->x1-dest_x;
    if (clip->x2<(dest_x+w))
      xmax = clip->x2-dest_x+1;
  }

  for (;hc<h;hc++,dest_y++) {
    if (olines && lines==hc) {
      lines += olines;
      dest_x--;
      flag=1;
    }
    for (x=dest_x,iw=bw,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x++,xpix++) {
	 if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	   (*vid->pixel) (dest,x,dest_y,c,lgop);
      }
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

/* Like charblit, but rotate 90 degrees anticlockwise whilst displaying
 *
 * As this code was mostly copied from linear8, it probably has the same
 * subtle "smudging" bug noted in linear8.c
 */
void def_charblit_90(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
		     s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,
		     s16 lgop) {
  int bw = w;
  int iw,hc,y;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || (dest_y-w)>clip->y2 || (dest_x+h)<clip->x1 || 
      dest_y<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->x2<(dest_x+h-1))
      h = clip->x2-dest_x+1;
    if (clip->x1>dest_x) {
      hc = clip->x1-dest_x; /* Do it this way so skewing doesn't mess up when clipping */
      while (lines < hc && olines) {
	lines += olines;
	dest_y++;
      }
      dest_x += hc;
      chardat += hc*bw;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->y1>dest_y-w+1)
      xmax = 1+dest_y-clip->y1;
    if (clip->y2<(dest_y))
      xmin = dest_y-clip->y2;
  }

  for (;hc<h;hc++,dest_x++) {
    if (olines && lines==hc) {
      lines += olines;
      dest_y++;
      flag=1;
    }
    for (iw=bw,y=dest_y,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,y--,xpix++) {
	 if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	   (*vid->pixel) (dest,dest_x,y,c,lgop);
      }
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

void def_charblit_180(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
		      s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,
		      s16 lgop) {
  int bw = w;
  int iw,hc,x;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x<clip->x1 || dest_y<clip->y1 || (dest_x-w)>clip->x2 || 
      (dest_y-h)>clip->y2)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y1>(dest_y-h))
      h = dest_y-clip->y1+1;
    if (clip->y2<dest_y) {
      hc = dest_y-clip->y2; /* Do it this way so skewing doesn't mess up when clipping */
      while (lines < hc && olines) {
	lines += olines;
	dest_x--;
      }
      dest_y -= hc;
      chardat += hc*bw;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->x2<dest_x)
      xmin = dest_x-clip->x2;
    if (clip->x1>(dest_x-w))
      xmax = dest_x-clip->x1+1;
  }

  for (;hc<h;hc++,dest_y--) {
    if (olines && lines==hc) {
      lines += olines;
      dest_x--;
      flag=1;
    }
    for (x=dest_x,iw=bw,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x--,xpix++) {
	 if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	   (*vid->pixel) (dest,x,dest_y,c,lgop); 
      }
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

void def_charblit_270(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
		     s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,
		     s16 lgop) {
  int bw = w;
  int iw,hc,y;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x<clip->x1 || (dest_y+w)<clip->y1 || (dest_x-h)>clip->x2 || 
      dest_y>clip->y2)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->x1>(dest_x-h-1))
      h = dest_x-clip->x1+1;
    if (clip->x2<dest_x) {
      hc = dest_x-clip->x2; /* Do it this way so skewing doesn't mess up when clipping */
      while (lines < hc && olines) {
	lines += olines;
	dest_y++;
      }
      dest_x -= hc;
      chardat += hc*bw;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->y1>dest_y)
      xmin = clip->y1-dest_y;
    if (clip->y2<(dest_y+w))
      xmax = clip->y2-dest_y+1;
  }

  for (;hc<h;hc++,dest_x--) {
    if (olines && lines==hc) {
      lines += olines;
      dest_y++;
      flag=1;
    }
    for (iw=bw,y=dest_y,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,y++,xpix++) {
	 if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	   (*vid->pixel) (dest,dest_x,y,c,lgop);
      }
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

/* A meta-charblit to select the appropriate function based on angle */
void def_charblit(hwrbitmap dest, u8 *chardat,s16 x,s16 y,s16 w,s16 h,
		  s16 lines, s16 angle, hwrcolor c, struct quad *clip,
		  s16 lgop) {

   void (*p)(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
	     s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,s16 lgop);
   
   switch (angle) {
    case 0:   p = &def_charblit_0;   break;
    case 90:  p = &def_charblit_90;  break;
    case 180: p = &def_charblit_180; break;
    case 270: p = &def_charblit_270; break;
    default:
      return;
   }

   (*p)(dest,chardat,x,y,w,h,lines,c,clip,lgop);
}

/* The End */