/* $Id: textbox.c,v 1.6 2001/09/24 17:20:31 micahjd Exp $
 *
 * textbox.c - works along with the rendering engine to provide advanced
 * text display and editing capabilities
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

/**************************** Definitions */

/**************************** Widget handlers */

/* Set up divnodes */
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

void textbox_remove(struct widget *self) {
   if (!in_shutdown)
     r_divnode_free(self->in);
}

void textbox_resize(struct widget *self) {
}

g_error textbox_set(struct widget *self,int property, glob data) {
   return mkerror(ERRT_PASS,0);
}

glob textbox_get(struct widget *self,int property) {
   return 0;
}

void textbox_trigger(struct widget *self,long type,union trigparam *param) {
}

/**************************** Utilities */

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

   
/* The End */
