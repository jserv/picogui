/* $Id: terminal_textgrid.c,v 1.6 2002/10/30 05:09:13 micahjd Exp $
 *
 * terminal.c - a character-cell-oriented display widget for terminal
 *              emulators and things.
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
#include <pgserver/terminal.h>
#define DATA WIDGET_DATA(0,terminaldata)


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
  fd->lib->getmetrics(fd,&m);
  celw      = m.charcell.w;
  celh      = m.charcell.h;
  
  /* n->param[1]'s low u16 is the buffer width, the high u16 is
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
      
      fd->lib->draw_char(fd,r->output,xy_to_pair(n->r.x,n->r.y),
			 textcolors[attr & 0x0F],
			 ch, &r->clip,r->lgop, 0);
      n->r.x += celw;
    }
  }
}


/********************************************** Textgrid-related utility functions */

/* Build an incremental gropnode from the update rectangle */
void term_realize(struct widget *self) {
   
   /* Is an update unnecessary? */
   if (!DATA->updw)
     return;
   
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
   
/*
   guru("Incremental terminal update:\nx = %d\ny = %d\nw = %d\nh = %d"
	"\nbufferw = %d\nbufferh = %d",
	DATA->updx,DATA->updy,DATA->updw,DATA->updh,
	DATA->bufferw,DATA->bufferh);
*/
 
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
  if (x<0 || y<0 || x>=DATA->bufferw || y>=DATA->bufferh)
    return;
  pgstring_seek(DATA->buffer, &p, x + y * DATA->bufferw, PGSEEK_SET);
  pgstring_encode_meta(DATA->buffer, &p, c, (void*)(u32) DATA->current.attr);
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
  int i;

  pgstring_seek(DATA->buffer, &p, fromx + fromy * DATA->bufferw, PGSEEK_SET);

  while (!p.invalid && chars--)
    pgstring_encode_meta(DATA->buffer, &p, ' ', (void*)(u32) DATA->current.attr);

  /* Add update rectangle for the effected lines */
  i = (chars+fromx) / DATA->bufferh;
  if ((chars+fromx) % DATA->bufferh)
    i++;
  term_updrect(self,0,fromy,DATA->bufferw,i);
}

/* Copy a rectangle between two text buffers */
void textblit(struct pgstring *src,struct pgstring *dest,int src_x,int src_y,int src_w,
	      int dest_x,int dest_y,int dest_w,int w,int h) {
  int src_chr,dest_chr;

  /* Top-down blit */
  if (dest_y < src_y) {
    src_chr  = src_x + src_y * src_w;
    dest_chr = dest_x + dest_y * dest_w;
    
    for (;h>0;h--,src_chr+=src_w,dest_chr+=dest_w)
      pgstring_chrcpy(dest,src,dest_chr,src_chr,w);
  }
  /* Bottom-up blit */
  else {
    src_chr  = src_x + (src_y + h - 1) * src_w;
    dest_chr = dest_x + (dest_y + h - 1) * dest_w;
 
    for (;h>0;h--,src_chr-=src_w,dest_chr-=dest_w)
      pgstring_chrcpy(dest,src,dest_chr,src_chr,w);
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

/* The End */
