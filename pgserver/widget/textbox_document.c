/* $Id: textbox_document.c,v 1.33 2002/02/26 06:42:16 micahjd Exp $
 *
 * textbox_document.c - works along with the rendering engine to provide
 * advanced text display and editing capabilities. This file provides a set
 * of functions for manipulating text represented in a divtree.
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
#include <pgserver/widget.h>
#include <pgserver/font.h>
#include <pgserver/textbox.h>
#include <string.h>
#include <limits.h>           /* for INT_MAX definition portability... */

/************************* Supported format loaders */

struct txtformat text_formats[] = {

#ifdef CONFIG_FORMAT_TEXTSAVE
  { {'T','E','X','T'}, &plaintext_load, &plaintext_save },
#else
  { {'T','E','X','T'}, &plaintext_load, NULL },
#endif
#ifdef CONFIG_FORMAT_HTML
  { {'H','T','M','L'}, &html_load, NULL },
#endif

  { {0,0,0,0}, NULL, NULL }
};

/************************* Formatting */

/* Add a format node to the stack */
g_error text_format_add(struct textbox_cursor *c, struct formatnode *f) {
  f->next = c->f_top;
  c->f_top = f;
  return success;
}

/* Add a node to change the text color */
g_error text_format_color(struct textbox_cursor *c, pgcolor color) {
  struct formatnode *fn;
  g_error e;

  /* Empty formatnode */
  e = g_malloc((void**) &fn,sizeof(struct formatnode));
  errorcheck;
  memset(fn,0,sizeof(struct formatnode));

  /* Same font, new color */
  if (c->f_top)
    fn->fontdef = c->f_top->fontdef;
  fn->color = VID(color_pgtohwr)(color);

  return text_format_add(c,fn);
}

/* Add a node to change the font completely */
g_error text_format_font(struct textbox_cursor *c, handle font) {
  struct formatnode *fn;
  g_error e;

  /* Empty formatnode */
  e = g_malloc((void**) &fn,sizeof(struct formatnode));
  errorcheck;
  memset(fn,0,sizeof(struct formatnode));

  /* Same color, new font */
  if (c->f_top)
    fn->color = c->f_top->color;
  fn->fontdef = font;

  return text_format_add(c,fn);
}

/* Add a node to change font flag(s) and/or size */
g_error text_format_modifyfont(struct textbox_cursor *c,
			       u32 flags_on, u32 flags_off,s16 size_delta) {
  struct fontdesc *fd;
  const char *font_name = NULL;
  g_error e;
  int font_size = 0;
  u32 font_style = 0;
  handle font;

  /* Use previous font as a starting point */
  if (c->f_top && c->f_top->fontdef) {
    e = rdhandle((void**) &fd,PG_TYPE_FONTDESC,c->widget->owner,
		 c->f_top->fontdef);
    errorcheck;
    font_name  = fd->fs->name;
    font_size  = fd->fs->size;
    font_style = fd->style;
  }

  font_style |= flags_on;
  font_style &= ~flags_off;
  font_size += size_delta;

  /* New font */
  e = findfont(&font,c->widget->owner,font_name,font_size,font_style);
  errorcheck;

  return text_format_font(c,font);
}

/* Remove the topmost layer of formatting */
void text_unformat_top(struct textbox_cursor *c) {
  struct formatnode *n;

  if (!c->f_top)
    return;
  
  /* Remove the node from the stack */
  n = c->f_top;
  c->f_top = n->next;

  /* Is this node used? */
  if (n->font_refcnt) {
    /* Move it to the f_used list */
    n->next = c->f_used;
    c->f_used = n;
  }
  else {
    /* Delete it, and its associated handles */
    if (n->fontdef)
      handle_free(c->widget->owner,n->fontdef);
    g_free(n);
  }
}

/* Remove all formatting */
void text_unformat_all(struct textbox_cursor *c) {
  while (c->f_top)
    text_unformat_top(c);
}

/************************* Text */

/* Populates the index and width lists in the cursor */
g_error textbox_cursor_indexwidth(struct textbox_cursor *c) {
  g_error e;
  const u8 *gropstr, *chp;
  s16 cw[256], cn[256];
  int index, oindex, xoff=c->c_gx;
  struct gropnode *grop=c->c_div->div->grop;
  struct fontdesc *fd;

  while(grop && grop->type!=PG_GROP_SETFONT)
    grop=grop->next;
  if(!grop)
    return mkerror(PG_ERRT_INTERNAL, 12);	/* no font */
  e = rdhandle((void**) &fd, PG_TYPE_FONTDESC, c->widget->owner,
      grop->param[0]);
  errorcheck;
  while(grop && grop->type!=PG_GROP_TEXT)
    grop=grop->next;
  if(!grop)
    return mkerror(PG_ERRT_INTERNAL, 13);	/* no text */
  e = rdhandle((void**)&gropstr, PG_TYPE_STRING, c->widget->owner,
      grop->param[0]);
  errorcheck;
  chp=gropstr;
  oindex=0;
  while(*chp)
   {
    memset(cw, 0, sizeof(cw));
    for(index=0; index<sizeof(cw)/sizeof(*cw) && *chp; index++)
     {
      outchar_fake(fd, &cw[index], fd->decoder(&chp));
      cn[index]=chp-gropstr;
      xoff-=cw[index];
      if(xoff<0)
       {
	c->c_char=oindex+index;
	xoff=INT_MAX;
       }
     }
    e=g_realloc((void**)&c->c_cn, oindex+sizeof(*cn)*index);
    errorcheck;
    e=g_realloc((void**)&c->c_cw, oindex+sizeof(*cw)*index);
    errorcheck;
    memcpy(c->c_cn+oindex*sizeof(*c->c_cn), cn, index*sizeof(*cn));
    memcpy(c->c_cw+oindex*sizeof(*c->c_cw), cw, index*sizeof(*cw));
    oindex+=index;
   }
  if(xoff<=c->c_gx)	/* didn't match any of the characters */
    c->c_char=oindex;
  c->c_len=oindex;
  return success;
}

/* Inserts a breaking space between words at the cursor */
g_error text_insert_wordbreak(struct textbox_cursor *c) {
  g_error e;

  /* Make sure we're in a position where a word break would matter */
  if (c->c_line && c->c_div && c->c_gctx.current) {

    /* New divnode, clear grop context */
    e = newdiv(&c->c_div->next,c->widget);
    errorcheck;
    c->c_div = c->c_div->next;
    c->c_gctx.current = NULL;
    c->c_div->flags |= PG_S_LEFT | DIVNODE_AUTOWRAP;
    c->c_div->flags &= ~DIVNODE_UNDERCONSTRUCTION;

    /* A child div to actually do the rendering into */
    e = newdiv(&c->c_div->div,c->widget);
    c->c_div->div->flags &= ~DIVNODE_UNDERCONSTRUCTION;
    errorcheck;
  }

  return success;
}

/* Begin a new paragraph at the cursor */
g_error text_insert_linebreak(struct textbox_cursor *c) {
  /* Would a line break make sense here? */
  if (c->c_line) {
    struct divnode *newline;
    g_error e;

    /* Nothing in the existing line? Put a blank string in there to
     * get the height correct, otherwise the line will be invisible
     */
    if (!c->c_div) {
      e = text_insert_string(c,"",HFLAG_NFREE);
      errorcheck;
    }

    /* Construct a new line node with the same flags & size */
    e = newdiv(&newline, c->widget);
    errorcheck;
    newline->flags = c->c_line->flags;
    newline->split = c->c_line->split;

    /* Skip past any autogenerated (wordwrapped) lines */
    /* FIXME: split at c->c_div */
    while (c->c_line->nextline)
      c->c_line=c->c_line->next;
    
    /* Insert after this line, don't link nextline pointers */
    newline->next = c->c_line->next;
    c->c_line->next = newline;
    c->c_line->flags |= DIVNODE_NEED_RECALC;
    c->widget->dt->flags |= DIVTREE_NEED_RECALC;
    c->c_line = newline;

    c->c_div = NULL;
    c->c_gctx.current = NULL;
  }
  return success;
}

/* Insert the specified divnode as the only div within a line */
g_error text_insert_line_div(struct textbox_cursor *c, struct divnode *div) {
  g_error e;

  /* Make the current line empty */
  if (c->c_div) {
    e = text_insert_linebreak(c);
    errorcheck;
  }

  /* Stick the given divnode in the now-empty line */
  c->c_line->div = div;
  c->c_div = div;

  /* Get a new line so that words can't be added to this one */
  return text_insert_linebreak(c);
}

/* Insert a divnode to be wrapped along with the other words */
g_error text_insert_word_div(struct textbox_cursor *c, struct divnode *div) {
  /* TODO: implement this */
  return success;
}

/* Insert text with the current formatting at the cursor. This will not
 * generate breaking spaces. */
g_error text_insert_string(struct textbox_cursor *c, const u8 *str,
			   u32 hflag) {
  g_error e;
  struct fontdesc *fd;
  s16 tw=0, th, cw[256], cn[256];
  int index, oindex, newlen, oldlen, inspos;
  u8 **gropstr;
  handle hstr;

  /*** First thing to do is make sure we have a valid insertion point. */

  /* No line? */
  if (!c->c_line) {
    c->c_line = c->head;
    c->c_line->flags |= DIVNODE_NEED_RECALC;
    c->widget->in->flags |= DIVNODE_NEED_RECALC;
    c->widget->dt->flags |= DIVTREE_NEED_RESIZE;
  }

  /* No div? */
  if (!c->c_div) {
    e = newdiv(&c->c_line->div,c->widget);
    errorcheck;
    c->c_div = c->c_line->div;
    c->c_gctx.current = NULL;
    c->c_div->flags |= PG_S_LEFT | DIVNODE_AUTOWRAP | DIVNODE_NEED_RECALC;
    c->c_div->flags &= ~DIVNODE_UNDERCONSTRUCTION;

    /* A child div to actually do the rendering into */
    e = newdiv(&c->c_div->div,c->widget);
    c->c_div->div->flags &= ~DIVNODE_UNDERCONSTRUCTION;
    errorcheck;
    c->c_div->div->ph = th;
  }

  /* No grop context */
  if (!c->c_gctx.current) {
    gropctxt_init(&c->c_gctx,c->c_div->div);

    if(!c->c_div->div->grop) {
      /* Add font */
      if (c->f_top && c->f_top->fontdef) {
	c->f_top->font_refcnt++;
	addgrop(&c->c_gctx,PG_GROP_SETFONT);
	c->c_gctx.current->param[0] = c->f_top->fontdef;
      }
      /* Add color */
      if (c->f_top) {
	addgrop(&c->c_gctx,PG_GROP_SETCOLOR);
	c->c_gctx.current->param[0] = c->f_top->color;
      }
    }
  }

  /* Retrieve font */
  if (c->c_div->div->grop->type==PG_GROP_SETFONT)
    e = rdhandle((void**) &fd, PG_TYPE_FONTDESC, c->widget->owner,
	c->c_div->div->grop->param[0]);
  else
    e = rdhandle((void**) &fd, PG_TYPE_FONTDESC, -1, defaultfont);
  errorcheck;

  th = fd->font->ascent+fd->font->descent+fd->interline_space+fd->margin;

  /* Make sure we're at a text gropnode */
  if(c->c_gctx.current && c->c_gctx.current->type==PG_GROP_TEXT)
   {
    hstr=c->c_gctx.current->param[0];
    e=rdhandlep((void***)&gropstr, PG_TYPE_STRING, c->widget->owner, hstr);
    errorcheck;
   }
  else
   {
    /* dummy placeholder pointer - mkhandle doesn't accept NULL */
    e = mkhandle(&hstr,PG_TYPE_STRING,c->widget->owner,
	text_insert_string);
    errorcheck;
    e=rdhandlep((void***)&gropstr, PG_TYPE_STRING, c->widget->owner, hstr);
    errorcheck;
    /* set *gropstr to NULL and clear NFREE flag */
    rehandle(hstr, NULL, PG_TYPE_STRING);
    addgropsz(&c->c_gctx,PG_GROP_TEXT,0,0,1,th);
    c->c_gctx.current->param[0] = hstr;
    c->c_len=0;
    c->c_char=0;
   }

  /* Copy the text */
  newlen=strlen(str);
  oldlen=*gropstr?strlen(*gropstr):0;
  e=g_realloc((void**)gropstr, oldlen+newlen+1);
  errorcheck;
  inspos=c->c_char?c->c_cn[c->c_char-1]:0;
  memmove(*gropstr+inspos+newlen, *gropstr+inspos, oldlen-inspos);
  memcpy(*gropstr+inspos, str, newlen);
  (*gropstr)[oldlen+newlen]=0;

  /* Free the original */
  if(!(hflag&HFLAG_NFREE))
    g_free(str);

  /* Measure it */
  tw=0;		/* first sum the unchanged characters */
  for(oindex=0; oindex<c->c_char-1; oindex++)
    tw+=c->c_cw[oindex];
  if(oindex)
    str=*gropstr+c->c_cn[oindex-1];
  else
    str=*gropstr;
  c->c_char=oldlen+newlen;	/* last character */
  while(*str)
   {
    memset(cw, 0, sizeof(cw));
    for(index=0; index<sizeof(cw)/sizeof(*cw) && *str; index++)
     {
      outchar_fake(fd, &cw[index], fd->decoder(&str));
      tw+=cw[index];
      cn[index]=str-*gropstr;
      if(cn[index]>=inspos+newlen && c->c_char==oldlen+newlen)
       {
	c->c_char=oindex+index+1;	/* current position */
	c->c_gx=tw;
       }
     }
    e=g_realloc((void**)&c->c_cn, sizeof(*cn)*(index+oindex));
    errorcheck;
    e=g_realloc((void**)&c->c_cw, sizeof(*cw)*(index+oindex));
    errorcheck;
    memcpy(c->c_cn+oindex, cn, index*sizeof(*cn));
    memcpy(c->c_cw+oindex, cw, index*sizeof(*cw));
    oindex+=index;
   }
  if(c->c_char==oldlen+newlen)
    c->c_gx=tw;
  c->c_len=oindex+index;
  /* Add the width of a space to preferred width for space between words */
  outchar_fake(fd, &tw, ' ');

  printf("String width: %d, contents: \"%s\"\n", tw, *gropstr);

  /* update grop width */
  c->c_div->div->pw = tw;
  c->c_gctx.current->r.w = tw;

  c->c_div->split = c->c_div->div->pw;
  c->c_div->flags |= DIVNODE_NEED_RECALC | DIVNODE_NEED_REDRAW | 
    DIVNODE_PROPAGATE_REDRAW;
  c->widget->dt->flags |= DIVTREE_NEED_RECALC;

  return success;
}

/* Delete the character or other object after the cursor */
void text_delete_next(struct textbox_cursor *c) {
}

/* Delete the character or other object before the cursor */
void text_delete_prev(struct textbox_cursor *c) {
}

/* Combine gropnodes when possible to reduce memory consumption */
g_error text_compact(struct textbox_cursor *c) {
  return success;
}

/* Show the caret at the current cursor position */
g_error text_caret_on(struct textbox_cursor *c) {
  g_error e;
  struct gropnode *g; 

  e = text_caret_off(c);
  errorcheck;

  /* No cursor? */
  if (!c->c_div)
    return success;

  /* is the caret not at the right place? */
  if (c->caret != &c->c_div->grop) {

    /* Delete old caret? */
    if (c->caret && *c->caret) {
      g = *c->caret;
      *c->caret = (*c->caret)->next;
      gropnode_free(g);
    }

    /* Allocate new caret */
    c->caret = &c->c_div->grop;
    g = *c->caret;
    e = gropnode_alloc(c->caret);
    errorcheck;
    (*c->caret)->flags = PG_GROPF_UNIVERSAL | PG_GROPF_COLORED;
    (*c->caret)->next = g;
    (*c->caret)->r.w = 2;
    (*c->caret)->r.h = 0x7FFF;
    (*c->caret)->type = PG_GROP_RECT;
  }

  /* Make the caret visible */
  (*c->caret)->param[0] = 0x000000;
  (*c->caret)->r.x = c->c_gx;
  (*c->caret)->r.y = 0;
  c->c_div->flags |= DIVNODE_INCREMENTAL;
  c->caret_div = c->c_div;
  update(c->c_div,1);

  return success;
}

/* Hide the caret at the current cursor position */
g_error text_caret_off(struct textbox_cursor *c) {
  if (c->caret && *c->caret) {
    (*c->caret)->param[0] = 0xFFFFFF;
    c->caret_div->flags |= DIVNODE_NEED_REDRAW;
    update(c->caret_div,1);
  }
  return success;
}

/************************* Loading */
/* Delete a linked list of formatnodes */

void textbox_delete_formatstack(struct widget *self,
				struct formatnode *list) {
  struct formatnode *n, *condemn;
  n = list;
  while (n) {
    condemn = n;
    n = n->next;
    
    if (condemn->fontdef)
      handle_free(self->owner,condemn->fontdef);
    g_free(condemn);
  }
} 

g_error text_nuke(struct textbox_cursor *c) {
  g_error e;
  struct widget *w = c->widget;
  handle h;

  /* Delete our formatting stacks and associated fonts */
  textbox_delete_formatstack(c->widget, c->f_used);
  textbox_delete_formatstack(c->widget, c->f_top);

  /* character width and index lists */
  g_free(c->c_cw);
  g_free(c->c_cn);

  /* Delete divnodes */
  if (c->head) {
    r_divnode_free(c->head->div);
    r_divnode_free(c->head->next);
    c->head->div = c->head->next = NULL;
  }

  /* Reset the cursor */
  memset(c,0,sizeof(struct textbox_cursor));
  c->head = w->in->div->div;
  c->widget = w;

  /* Set up a reasonable default font */
  e = findfont(&h,w->owner,"Lucida",10,PG_FSTYLE_FLUSH);
  errorcheck;
  e = text_format_font(c,h);
  errorcheck;

  return success;
}

/* Load text of the specified format */
g_error text_load(struct textbox_cursor *c, const char *fmt_code,
		  const u8 *data, u32 datalen) {
  struct txtformat *f = text_formats;

  while (f->name[0] && strncmp(f->name,fmt_code,4))
    f++;

  if (!f->load)
    return mkerror(PG_ERRT_BADPARAM,51);  /* Unsupported text format */

  return (*f->load)(c,data,datalen);
}

/* Save text of the specified format */
g_error text_save(struct textbox_cursor *c, const char *fmt_code,
		  u8 **data, u32 *datalen) {
  struct txtformat *f = text_formats;

  while (f->name[0] && strncmp(f->name,fmt_code,4))
    f++;

  if (!f->save)
    return mkerror(PG_ERRT_BADPARAM,51);  /* Unsupported text format */

  return (*f->save)(c,data,datalen);
}

/* The End */
