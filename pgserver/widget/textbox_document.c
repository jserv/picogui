/* $Id: textbox_document.c,v 1.1 2001/10/02 05:57:46 micahjd Exp $
 *
 * textbox_document.c - works along with the rendering engine to provide
 * advanced text display and editing capabilities. This file provides a set
 * of functions for manipulating text represented in a divtree.
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
#include <pgserver/widget.h>
#include <pgserver/textbox.h>

/************************* Formatting */

/* Add a format node to the stack */
g_error text_format_add(struct textbox_cursor *c, struct formatnode *f) {
  return sucess;
}

/* Add a node to change the text color */
g_error text_format_color(struct textbox_cursor *c, pgcolor color) {
  return sucess;
}

/* Add a node to change the font completely */
g_error text_format_font(struct textbox_cursor *c, handle font) {
  return sucess;
}

/* Add a node to change font flag(s) */
g_error text_format_font_flags(struct textbox_cursor *c,u32 on, u32 off) {
  return sucess;
}

/* Add a node to change font size relative to the current size */
g_error text_format_font_addsize(struct textbox_cursor *c,int size) {
  return sucess;
}

/* Remove the topmost layer of formatting */
void text_unformat_top(struct textbox_cursor *c) {
}

/* Remove all formatting */
void text_unformat_all(struct textbox_cursor *c) {
}

/************************* Text */

/* Inserts a breaking space between words at the cursor */
g_error text_insert_wordbreak(struct textbox_cursor *c) {
  g_error e;

  /* Make sure we're in a position where a word break would matter */
  if (c->c_line && c->c_div) {

    /* New divnode, clear grop context */
    e = newdiv(&c->c_div->next,c->widget);
    errorcheck;
    c->c_div = c->c_div->next;
    c->c_gctx.current = NULL;
    c->c_div->flags |= PG_S_LEFT | DIVNODE_AUTOWRAP;
  }

  return sucess;
}

/* Begin a new paragraph at the cursor */
g_error text_insert_linebreak(struct textbox_cursor *c) {
  return sucess;
}

/* Insert text with the current formatting at the cursor. This will not
 * generate breaking spaces. */
g_error text_insert_string(struct textbox_cursor *c, const char *str) {
  g_error e;
  struct fontdesc *fd;
  s16 tw,th;
  handle hstr;
  
  /*** First thing to do is make sure we have a valid insertion point. */

  /* No line? */
  if (!c->c_line)
    c->c_line = c->head;

  /* No div? */
  if (!c->c_div) {
    e = newdiv(&c->c_line->div,c->widget);
    errorcheck;
    c->c_div = c->c_line->div;
    c->c_gctx.current = NULL;
    c->c_div->flags |= PG_S_LEFT | DIVNODE_AUTOWRAP;
  }

  /* No grop? */
  if (!c->c_gctx.current) {
    gropctxt_init(&c->c_gctx,c->c_div);
    c->c_gx = c->c_gy = 0;
   
    /* Add font */
    if (c->f_top && c->f_top->fontdef) {
      c->f_top->font_refcnt++;
      addgrop(&c->c_gctx,PG_GROP_SETFONT);
      c->c_gctx.current->param[0] = c->f_top->fontdef;
    }
    /* Add color */
    if (c->f_top && c->f_top->color) {
      addgrop(&c->c_gctx,PG_GROP_SETCOLOR);
      c->c_gctx.current->param[0] = c->f_top->color;
    }
  }
  
  /* Measure the text */
  if (c->f_top && c->f_top->fontdef)
    e = rdhandle((void**) &fd,PG_TYPE_FONTDESC,c->widget->owner,
		 c->f_top->fontdef);
  else
    e = rdhandle((void**) &fd,PG_TYPE_FONTDESC,-1,defaultfont);
  errorcheck;
  sizetext(fd,&tw,&th,str);
  
  /* Add a text gropnode at the cursor */
  e = mkhandle(&hstr,PG_TYPE_STRING,c->widget->owner,str);
  errorcheck;
  addgropsz(&c->c_gctx,PG_GROP_TEXT,c->c_gx,c->c_gy,1,1);
  c->c_gctx.current->param[0] = hstr;

  /* Update cursor and preferred size */
  c->c_gx += tw;
  c->c_div->pw = c->c_gx;
  if (th > c->c_div->ph)
    c->c_div->ph = th;
  c->c_div->split = c->c_div->pw;

  return sucess;
}

/* Delete the character or other object after the cursor */
void text_delete_next(struct textbox_cursor *c) {
}

/* Delete the character or other object before the cursor */
void text_delete_prev(struct textbox_cursor *c) {
}

/* Combine gropnodes when possible to reduce memory consumption */
g_error text_compact(struct textbox_cursor *c) {
  return sucess;
}

/********************************** CRUFT BARRIER ***********************/
#if 0

/* Create a divnode containing a piece of text */
g_error text_div(struct divnode **where, struct widget *self,
		 char *str, pgcolor c, handle font) {
  g_error e;
  handle h;
  struct gropctxt ctx;
  struct fontdesc *fd;

  /* Init divnode and context */
  e = newdiv(where,self);
  errorcheck;
  gropctxt_init(&ctx,*where);

  /* Make a string handle */
  e = mkhandle(&h,PG_TYPE_STRING | HFLAG_NFREE,self->owner,str);
  errorcheck;

  /* color node */
  if (c) {
    addgrop(&ctx,PG_GROP_SETCOLOR);
    ctx.current->param[0] = VID(color_pgtohwr)(c);
  }

  /* font node */
  if (font) {
    addgrop(&ctx,PG_GROP_SETFONT);
    ctx.current->param[0] = font;
  }

  /* Text node */
  addgropsz(&ctx,PG_GROP_TEXT,0,0,1,1);
  ctx.current->param[0] = h;

  /* Lookup font */
  if (font)
    e = rdhandle((void**) &fd,PG_TYPE_FONTDESC,self->owner,font);
  else
    e = rdhandle((void**) &fd,PG_TYPE_FONTDESC,-1,defaultfont);
  errorcheck;

  /* Preferred size */
  sizetext(fd,&(*where)->pw,&(*where)->ph,str);
  (*where)->flags |= PG_S_LEFT | DIVNODE_AUTOWRAP;
  (*where)->split = (*where)->pw;
 
  /* For debugging, show the extent of the divnode */
  //  addgropsz(&ctx,PG_GROP_FRAME,0,0,(*where)->pw,(*where)->ph);

  return sucess;
}

g_error textbox_install(struct widget *self) {
   g_error e;
   struct divnode **w;
   int i;
   char *p,*q;

   static char teststr[] = 
     "This is a test of text layout capability in PicoGUI. Right now "
     "it's just tokenizing this string into a bunch of divnodes with "
     "the DIVNODE_AUTOWRAP flag. The nifty thing is, with all these "
     "divnode flags we've accumulated (28!) it shouldn't be too hard "
     "to convert HTML, RTF, or whatever directly into a PicoGUI divtree. "
     "Tables, images, weird formatting, embedded widgets- you name it, "
     "this thing should support it. But for any of that to happen, I "
     "have to use this string to debug the DIVNODE_AUTOWRAP flag :)";

   /* Disable the usual method of building groplists. */
   self->rawbuild = 1;
   
   /* Main split */
   e = newdiv(&self->in,self);
   errorcheck;
   self->in->flags |= PG_S_TOP;
   self->out = &self->in->next;
   
   /* 1st line */
   e = newdiv(&self->in->div,self);
   errorcheck;
   self->in->div->flags |= PG_S_TOP;

   /* Start at 1st line */
   w = &self->in->div->div;   

   /* Hackishly tokenize a string into text divnodes */
   p = teststr;
   while (p) {
     q = strchr(p,' ');
     if (q)
       *q = 0;

     e = text_div(w,self,p,0,0);
     errorcheck;

     /* Tell it where our second line is */
     (*w)->nextline = self->in->div->next;
     /* Move 'where' pointer to the next position */
     w = &((*w)->next);

     p = q;
     if (p)
       p++;
   }

   return sucess;
}

#endif
   
/* The End */
