/* $Id$
 *
 * terminal.c - a character-cell-oriented display widget for terminal
 *              emulators and things.
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
#include <pgserver/terminal.h>
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(terminaldata)

#ifdef DEBUG_TERMINAL_TEXTGRID
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>


/********************************************** Textgrid implementation */

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
  struct pgstr_iterator stri;
  struct pgstring *str;
  struct font_descriptor *fd;
  struct font_metrics m;

  DBG("rendering buffer 0x%08X with width %d, offset %d, and palette 0x%08X\n",
      n->param[0], n->param[1]>>16, n->param[1]&0xFFFF, n->param[2]);

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
  
  fd->lib->getmetrics(fd,&m);
  celw      = m.charcell.w;
  celh      = m.charcell.h;
  
  /* Protection against SIGFPE if our font engine is hosed */
  if (celw < 1) celw = 1;
  if (celh < 1) celh = 1;

  /* n->param[1]'s high u16 is the buffer width, the low u16 is
   * an offset from the beginning of the buffer.
   */
  bufferw = n->param[1] >> 16;
  buffersz = str->num_chars - (n->param[1] & 0xFFFF);
  pgstring_seek(str,&stri, n->param[1] & 0xFFFF, PGSEEK_SET);
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
  for (;charh;charh--,n->r.y+=celh,pgstring_seek(str,&stri,offset,PGSEEK_CUR)) {
      
    /* Skip the entire line if it's clipped out */
    if (n->r.y > r->clip.y2 ||
	(n->r.y+celh) < r->clip.y1) {
      pgstring_seek(str,&stri,bufferw,PGSEEK_CUR);
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

      /* If the character has the high bit set, check whether it's a line drawing character.
       * If not, and it's not blank, render it normally.
       */
      if ((!(ch & 0x80)) || !term_linedraw(r->output, n->r.x, n->r.y, 
					   celw, celh, ch, textcolors[attr & 0x0F], r->lgop)) {
	if (ch != ' ')
	  fd->lib->draw_char(fd,r->output,xy_to_pair(n->r.x,n->r.y),
			     textcolors[attr & 0x0F],
			     ch, &r->clip,r->lgop, 0);
      }
      n->r.x += celw;
    }
  }
}

/* Handle drawing line-drawing characters that were mapped into
 * the character set where ISO Latin-1 normally is :)
 * Return 1 if the character is handled by this function.
 */
int term_linedraw(hwrbitmap dest, int x, int y, int w, int h,
		  unsigned char ch, hwrcolor color, int lgop) {
  switch (ch) {

  case 'a' | 0x80:   /* Stippled box */
    VID(rect)(dest, x, y, w, h, color, PG_LGOP_STIPPLE);
    return 1;

  case 'j' | 0x80:   /* Bottom-right corner */
    VID(slab)(dest, x, y+(h>>1), 1+(w>>1), color, lgop);
    VID(bar)(dest, x+(w>>1), y, 1+(h>>1), color, lgop);
    return 1;

  case 'k' | 0x80:   /* Top-right corner */
    VID(slab)(dest, x, y+(h>>1), 1+(w>>1), color, lgop);
    VID(bar)(dest, x+(w>>1), y+(h>>1), 1+(h>>1), color, lgop);
    return 1;

  case 'l' | 0x80:   /* Top-left corner */
    VID(slab)(dest, x+(w>>1), y+(h>>1), 1+(w>>1), color, lgop);
    VID(bar)(dest, x+(w>>1), y+(h>>1), 1+(h>>1), color, lgop);
    return 1;

  case 'm' | 0x80:   /* Bottom-left corner */
    VID(slab)(dest, x+(w>>1), y+(h>>1), 1+(w>>1), color, lgop);
    VID(bar)(dest, x+(w>>1), y, h>>1, color, lgop);
    return 1;

  case 'n' | 0x80:   /* Cross */
    VID(slab)(dest, x, y+(h>>1), w, color, lgop);
    VID(bar)(dest, x+(w>>1), y, h, color, lgop);
    return 1;

  case 'o' | 0x80:   /* Top horizontal line */
    VID(slab)(dest, x, y, w, color, lgop);
    return 1;

  case 'p' | 0x80:   /* Top-middle horizontal line */
    VID(slab)(dest, x, y+(h>>2), w, color, lgop);
    return 1;

  case 'q' | 0x80:   /* Middle horizontal line */
    VID(slab)(dest, x, y+(h>>1), w, color, lgop);
    return 1;

  case 'r' | 0x80:   /* Bottom-middle horizontal line */
    VID(slab)(dest, x, y+(h>>1)+(h>>2), w, color, lgop);
    return 1;

  case 's' | 0x80:   /* Bottom horizontal line */
    VID(slab)(dest, x, y+h-1, w, color, lgop);
    return 1;

  case 't' | 0x80:   /* Right tee */
    VID(bar)(dest, x+(w>>1), y, h, color, lgop);
    VID(slab)(dest, x+(w>>1), y+(h>>1), 1+(w>>1), color, lgop);
    return 1;

  case 'u' | 0x80:   /* Left tee */
    VID(bar)(dest, x+(w>>1), y, h, color, lgop);
    VID(slab)(dest, x, y+(h>>1), 1+(w>>1), color, lgop);
    return 1;

  case 'v' | 0x80:   /* Top tee */
    VID(slab)(dest, x, y+(h>>1), w, color, lgop);
    VID(bar)(dest, x+(w>>1), y, 1+(h>>1), color, lgop);
    return 1;

  case 'w' | 0x80:   /* Bottom tee */
    VID(slab)(dest, x, y+(h>>1), w, color, lgop);
    VID(bar)(dest, x+(w>>1), y+(h>>1), 1+(h>>1), color, lgop);
    return 1;

  case 'x' | 0x80:   /* Vertical line */
    VID(bar)(dest, x+(w>>1), y, h, color, lgop);
    return 1;

  }
  return 0;
}


/********************************************** Textgrid-related utility functions */

/* Build an incremental gropnode from the update rectangle */
void term_realize(struct widget *self) {
  /* Is an update unnecessary? */
  if (!DATA->updw)
    return;
  
  DBG("updating %d,%d,%d,%d\n", DATA->updx, DATA->updy, DATA->updw, DATA->updh);
  
  /* Go ahead and set the master redraw flag */
  self->dt->flags |= DIVTREE_NEED_REDRAW;
  
  /* Are we forced to use a full update? (first time) */
  if (!DATA->inc) {
    self->in->div->flags |= DIVNODE_NEED_REDRAW;
    return;
  }
  
  /**** Set up an incremental update for the update rectangle */
  
  /* If this is more than one line, load the buffer width */  
  if (DATA->updh > 1)  
    DATA->inc->param[1] = DATA->bufferw << 16;
  else  
    DATA->inc->param[1] = DATA->updw << 16;  
 
  /* Set the buffer offset */
  DATA->inc->param[1] |= DATA->updx + DATA->updy * DATA->bufferw;
   
  /* Gropnode position (background and text) */
  DATA->bginc->r.x = DATA->inc->r.x = DATA->x + DATA->updx * DATA->celw;
  DATA->bginc->r.y = DATA->inc->r.y = DATA->y + DATA->updy * DATA->celh;
  DATA->bginc->r.w = DATA->inc->r.w = DATA->updw * DATA->celw;
  DATA->bginc->r.h = DATA->inc->r.h = DATA->updh * DATA->celh;
  DATA->bgsrc->r.x = DATA->bginc->r.x;
  DATA->bgsrc->r.y = DATA->bginc->r.y;

  /* Set the incremental update flag */
  self->in->div->flags |= DIVNODE_INCREMENTAL;
}

/* Add an update rectangle in */
void term_updrect(struct widget *self,int x,int y,int w,int h) {
  if (DATA->updw) {
    if (x < DATA->updx) {
      DATA->updw += DATA->updx - x;
      DATA->updx = x;
    }
    if (y < DATA->updy) {
      DATA->updh += DATA->updy - y;
      DATA->updy = y;
    }
    if ((w+x) > (DATA->updx+DATA->updw))
      DATA->updw = w+x-DATA->updx;
    if ((h+y) > (DATA->updy+DATA->updh))
      DATA->updh = h+y-DATA->updy;
  }
  else {
    DATA->updx = x;
    DATA->updy = y;
    DATA->updw = w;
    DATA->updh = h;
  }
}

/* Plot a character at an x,y position */
void term_plot(struct widget *self,int x,int y,u8 c) {
  struct pgstr_iterator p;
  u8 attr;
  if (x<0 || y<0 || x>=DATA->bufferw || y>=DATA->bufferh)
    return;

  if (DATA->current.reverse_video)
    attr = (DATA->current.attr >> 4) | (DATA->current.attr << 4);
  else
    attr = DATA->current.attr;

  pgstring_seek(DATA->buffer, &p, x + y * DATA->bufferw, PGSEEK_SET);
  pgstring_encode_meta(DATA->buffer, &p, c, (void*)(u32) attr);
  term_updrect(self,x,y,1,1);
}

/* Change attribute at an x,y position */
u8 term_chattr(struct widget *self,int x,int y,u8 c) {
  struct pgstr_iterator i,j;
  u32 ch;
  void *metadata;
  pgstring_seek(DATA->buffer, &i, x + y * DATA->bufferw, PGSEEK_SET);

  j = i;
  ch = pgstring_decode_meta(DATA->buffer, &j, &metadata);

  pgstring_encode_meta(DATA->buffer, &i, ch, (void*)(u32) c);
  term_updrect(self,x,y,1,1);

  return (u8)(u32)metadata;
}

/* Hide/show cursor */
void term_setcursor(struct widget *self,int flag) {
  if (DATA->current.cursor_hidden)
    flag = 0;

  if (flag == DATA->cursor_on)
    return;
  
  if (flag)
    /* Show cursor */
    DATA->attr_under_crsr = term_chattr(self,DATA->current.crsrx==DATA->bufferw?
		    			DATA->current.crsrx-1:DATA->current.crsrx,
					DATA->current.crsry,DATA->attr_cursor);
  else
    /* Hide cursor */
    term_chattr(self,DATA->current.crsrx==DATA->bufferw?DATA->current.crsrx-1:DATA->current.crsrx,
		DATA->current.crsry,DATA->attr_under_crsr);
  
  DATA->cursor_on = flag;
}

/* Prepare for adding more update rectangles.
 * This prevents losing updates- if the divnode still has its
 * incremental flag, don't clear the update rectangle */
void term_rectprepare(struct widget *self) {
  if (!(self->in->div->flags & DIVNODE_INCREMENTAL))
    DATA->updw = 0;
}

/* Clear a chunk of buffer */
void term_clearbuf(struct widget *self,int fromx,int fromy,int chars) {
  struct pgstr_iterator p;

  /* Add update rectangle for the effected lines */
  term_updrect(self,0,fromy,DATA->bufferw,(chars+fromx+1) / DATA->bufferh);

  pgstring_seek(DATA->buffer, &p, fromx + fromy * DATA->bufferw, PGSEEK_SET);

  while (!p.invalid && chars--)
    pgstring_encode_meta(DATA->buffer, &p, ' ', (void*)(u32) DATA->current.attr);
}

/* Copy a rectangle between two text buffers */
void textblit(struct pgstring *src,struct pgstring *dest,int src_x,int src_y,int src_w,
	      int dest_x,int dest_y,int dest_w,int w,int h) {
  struct pgstr_iterator src_i,dest_i;

  /* Top-down blit */
  if (src!=dest || dest_y < src_y) {
    pgstring_seek(src,&src_i,src_x + src_y * src_w,PGSEEK_SET);
    pgstring_seek(dest,&dest_i,dest_x + dest_y * dest_w,PGSEEK_SET);
    
    for (;h>0;h--) {
      pgstring_copy(dest,src,&dest_i,&src_i,w);
      pgstring_seek(src,&src_i,src_w,PGSEEK_CUR);
      pgstring_seek(dest,&dest_i,dest_w,PGSEEK_CUR);
    }
  }
  /* Bottom-up blit */
  else {
    pgstring_seek(src,&src_i,src_x + (src_y + h - 1) * src_w,PGSEEK_SET);
    pgstring_seek(dest,&dest_i,dest_x + (dest_y + h - 1) * dest_w,PGSEEK_SET);
 
    for (;h>0;h--) {
      pgstring_copy(dest,src,&dest_i,&src_i,w);
      pgstring_seek(src,&src_i,-src_w,PGSEEK_CUR);
      pgstring_seek(dest,&dest_i,-dest_w,PGSEEK_CUR);
    }
  }
}

/* Scroll the specified region up/down by some number of lines,
 * clearing the newly exposed region. ('lines' is + for down, - for up)
 */
void term_scroll(struct widget *self, int top_y, int bottom_y, int lines) {

  top_y = max(top_y, 0);
  bottom_y = min(bottom_y, DATA->bufferh - 1);
  if (bottom_y < top_y)
    return;
  if (max(lines,-lines) >= bottom_y - top_y)
    return;

  if (lines > 0) {
    textblit(DATA->buffer, DATA->buffer, 0, top_y, DATA->bufferw, 
	     0, top_y + lines, DATA->bufferw, DATA->bufferw, bottom_y - top_y + 1 - lines);    
    term_clearbuf(self, 0, top_y, DATA->bufferw * lines);
  }

  else if (lines < 0) {
    textblit(DATA->buffer, DATA->buffer, 0, top_y - lines, DATA->bufferw, 
	     0, top_y, DATA->bufferw, DATA->bufferw, bottom_y - top_y + 1 + lines);
    term_clearbuf(self, 0, bottom_y+1+lines, DATA->bufferw * -lines);
  }

  term_updrect(self,0,top_y,DATA->bufferw,bottom_y - top_y + 1);
}

/* Shift all the text at and after the cursor right by 'n' characters */
void term_insert(struct widget *self, int n) {

  /* Clamp to the amount of space not left of the cursor */
  if (n > DATA->bufferw - DATA->current.crsrx)
    n = DATA->bufferw - DATA->current.crsrx;

  /* Shift the characters over */
  textblit(DATA->buffer, DATA->buffer, 
	   DATA->current.crsrx, DATA->current.crsry, DATA->bufferw,
	   DATA->current.crsrx + n, DATA->current.crsry, DATA->bufferw,
	   DATA->bufferw - DATA->current.crsrx - n, 1);

  /* Blank out the inserted characters */
  term_clearbuf(self, DATA->current.crsrx, DATA->current.crsry, n);
  
  /* Update this changed region of the screen */
  term_updrect(self, DATA->current.crsrx, DATA->current.crsry,
	       DATA->bufferw - DATA->current.crsrx, 1);
}

/* Shift all the text after the cursor left by 'n' characters. */
void term_delete(struct widget *self, int n) {

  /* Clamp to the amount of space not left of the cursor */
  if (n > DATA->bufferw - DATA->current.crsrx)
    n = DATA->bufferw - DATA->current.crsrx;

  /* Shift the characters over */
  textblit(DATA->buffer, DATA->buffer, 
	   DATA->current.crsrx + n, DATA->current.crsry, DATA->bufferw,
	   DATA->current.crsrx, DATA->current.crsry, DATA->bufferw,
	   DATA->bufferw - DATA->current.crsrx - n, 1);

  /* Blank out the deleted characters */
  term_clearbuf(self, DATA->bufferw - n, DATA->current.crsry, n);
  
  /* Update this changed region of the screen */
  term_updrect(self, DATA->current.crsrx, DATA->current.crsry,
	       DATA->bufferw - DATA->current.crsrx, 1);
}

void term_setpalette(struct widget *self, int n, pgcolor color) {
  int *palette;

  /* Create a mutable copy of the theme's palette if we haven't already */
  if (!DATA->htextcolors)
    if (iserror(handle_dup(&DATA->htextcolors, -1, 
			   theme_lookup(self->in->div->state,PGTH_P_TEXTCOLORS))))
      return;

  if (iserror(rdhandle((void **) &palette, PG_TYPE_PALETTE, -1, DATA->htextcolors)))
    return;

  if (n<palette[0]) {
    color = VID(color_pgtohwr)(color);
    if (palette[n+1] != color) {
      if (n==0)
	set_widget_rebuild(self);       /* Rebuild necessary to set the background color */
      else
	term_updrect(self, 0, 0, DATA->bufferw, DATA->bufferh);
    }
    palette[n+1] = color;
  }
}

/* The End */
