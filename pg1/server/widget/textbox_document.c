/* $Id$
 *
 * textbox_document.c - High-level interface for managing documents
 *                      with multiple paragraphs, formatting, and
 *                      other embedded objects.
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
#include <pgserver/textbox.h>
#include <pgserver/paragraph.h>
#include <pgserver/render.h>
#include <pgserver/widget.h>

#ifdef DEBUG_TEXTBOX
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

/* Create a paragraph and pair of divnodes tied to each other */
g_error textbox_new_par_div(struct paragraph **par, struct divnode **div,
			    struct divnode *background);

/* Rebuild function for paragraph divnodes, issued by textbox_new_par_div */
void textbox_build_par_div(struct gropctxt *c, u16 state, struct widget *self);

void textbox_delete_parlist(struct paragraph *list);

struct txtformat text_formats[] = {
  { "plaintext", &plaintext_load, &plaintext_save },
#ifdef CONFIG_FORMAT_HTML
  /* HTML not working yet, but plaintext is better than nothing :) */
  // { "html", &html_load, &html_save },
  { "html", &plaintext_load, &plaintext_save },
#endif
  { NULL, NULL, NULL }
};


/******************************************************** Public Methods */

g_error document_new(struct textbox_document **doc, struct divnode *container_div) {
  g_error e;
  struct widget *w;
  s16 thobj;
  handle h;

  /* Initialize a new document struct */
  e = g_malloc((void**)doc, sizeof(struct textbox_document));
  errorcheck;
  memset(*doc, 0, sizeof(struct textbox_document));
  (*doc)->container_div = container_div;
  (*doc)->multiline = 1;
  (*doc)->autoscroll = 1;

  /* Initialize the paragraps */
  return document_nuke(*doc);
}

void document_delete(struct textbox_document *doc) {
  textbox_delete_parlist(doc->par_list);
  doc->container_div->div = NULL;
  g_free(doc);
}

g_error document_load(struct textbox_document *doc, const struct pgstring *format,
		      struct pgstring *str) {
  struct txtformat *f = text_formats;
  
  while (f->name && pgstring_cmp(pgstring_tmpwrap(f->name),format))
    f++;

  if (!f->load)
    return mkerror(PG_ERRT_BADPARAM,51);  /* Unsupported text format */

  return (*f->load)(doc,str);
}

g_error document_save(struct textbox_document *doc, const struct pgstring *format,
		      struct pgstring **str) {
  struct txtformat *f = text_formats;
  
  while (f->name && pgstring_cmp(pgstring_tmpwrap(f->name),format))
    f++;

  if (!f->save)
    return mkerror(PG_ERRT_BADPARAM,51);  /* Unsupported text format */

  return (*f->save)(doc,str);
}

/* Clear out the existing data and reset the cursor */
g_error document_nuke(struct textbox_document *doc) {
  g_error e;
  int was_visible = doc->crsr && doc->crsr->visible;
  
  if (doc->crsr)
    paragraph_hide_cursor(doc->crsr);
  textbox_delete_parlist(doc->par_list);
  doc->par_list = NULL;
  doc->container_div->div = NULL;

  /* Initial paragraph, stick the cursor in it */
  e = textbox_new_par_div(&doc->par_list, &doc->container_div->div, doc->container_div);
  errorcheck;
  doc->par_list->doc = doc;
  doc->crsr = &doc->par_list->cursor;
  doc->container_div->flags |= DIVNODE_NEED_RECALC | DIVNODE_NEED_REDRAW;
  doc->container_div->owner->dt->flags |= DIVTREE_NEED_RESIZE | 
    DIVTREE_NEED_RECALC | DIVTREE_NEED_REDRAW;
  if (was_visible)
    paragraph_show_cursor(doc->crsr);

  return success;
}

/* Insert a character/string at the current cursor location.
 * Metadata if any is copied (the document will not reference
 * metadata nodes in the original string). The cursor location
 * afterward will be after the inserted string.
 */
g_error document_insert_char(struct textbox_document *doc, u32 ch, void *metadata) {
  g_error e;

  /* Insert us the paragraph! */
  if (ch == '\n' || ch == '\r') {
    struct paragraph *oldpar, *newpar;
    struct pgstr_iterator i;
    
    /* Force an immediate cursor hide, since soon the cursor will be replaced */
    paragraph_hide_cursor(doc->crsr);

    /* Create a new paragraph, linked into the paragraph list and divtree */
    e = document_insert_paragraph(doc);
    errorcheck;
    oldpar = doc->crsr->par;
    newpar = oldpar->next;

    /* Split the section of the paragraph after the cursor into the new paragraph */
    pgstring_seek(newpar->content, &i, 0, PGSEEK_SET);
    e = pgstring_move(newpar->content,oldpar->content,
		      &i,&oldpar->cursor.iterator,-1);
    errorcheck;
    paragraph_wrap(oldpar,1);

    /* Set the cursor at the beginning of the new paragraph */
    doc->crsr = &newpar->cursor;
    paragraph_seekcursor(doc->crsr,0,PGSEEK_SET);
  }
  
  /* Normal character */
  else {
    e = paragraph_insert_char(doc->crsr,ch,metadata);
    errorcheck;
  }

  return success;
}

/* No special way to insert strings yet */
g_error document_insert_string(struct textbox_document *doc, struct pgstring *str) {
  struct pgstr_iterator i;
  u32 ch;
  void *meta;
  g_error e;

  pgstring_seek(str,&i,0,PGSEEK_SET);
  while ((ch = pgstring_decode_meta(str,&i,&meta))) {
    e = document_insert_char(doc,ch,meta);
    errorcheck;
  }
  return success;
}

/* Seek in the document, same interface as fseek.
 * If this seeks past the end of the stream, document_eof will return true
 */
void document_seek(struct textbox_document *doc, s32 offset, int whence) {
  struct paragraph *par;

  /* Seek to the beginning or end of the document first if necessary */
  switch (whence) {

  case PGSEEK_SET:
    doc->crsr = &doc->par_list->cursor;
    paragraph_seekcursor(doc->crsr,0,PGSEEK_SET);
    break;

  case PGSEEK_END:
    par = doc->par_list;
    while (par->next)
      par = par->next;
    doc->crsr = &par->cursor;
    paragraph_seekcursor(doc->crsr,0,PGSEEK_END);
    break;
  }

  /* Now keep seeking through paragraphs as necessary */
  while (offset) {
    paragraph_seekcursor(doc->crsr,offset,whence);
    offset = document_eof(doc);

    if (offset > 0 && doc->crsr->par->next) {
      doc->crsr = &doc->crsr->par->next->cursor;
      paragraph_seekcursor(doc->crsr,0,PGSEEK_SET);
      offset--;
    }

    else if (offset < 0 && doc->crsr->par->prev) {
      doc->crsr = &doc->crsr->par->prev->cursor;
      paragraph_seekcursor(doc->crsr,0,PGSEEK_END);
      offset++;
    }
    
    else
      break;
  }

}

/* Seek up/down in the document, snapping the cursor to the nearest character 
 *
 * This works by finding the proper paragraph, Then using the
 * last known cursor location to move directly vertical.
 * Returns the number of lines actually seeked (positive or negative)
 */
int document_lineseek(struct textbox_document *doc, s32 offset) {
  struct pgstr_iterator i;
  struct paragraph *par;
  struct paragraph_cursor *oldcrsr;
  int seeked = 0;

  /* Up */
  while (offset<0) {
    par = doc->crsr->par;
    if (!doc->crsr->line->prev)
      par = par->prev;
    if (!par)
      return seeked;
    oldcrsr = doc->crsr;
    doc->crsr = &par->cursor;
    paragraph_movecursor(doc->crsr, par,
			 oldcrsr->last_rect.x - par->div->div->r.x,
			 oldcrsr->last_rect.y - par->div->div->r.y - 1);
    offset++;
    seeked--;
  }
    
  /* Down */
  while (offset>0) {
    par = doc->crsr->par;
    if (!doc->crsr->line->next)
      par = par->next;
    if (!par)
      return seeked;
    oldcrsr = doc->crsr;
    doc->crsr = &par->cursor;
    paragraph_movecursor(doc->crsr, par,
			 oldcrsr->last_rect.x - par->div->div->r.x,
			 oldcrsr->last_rect.y + oldcrsr->line->height + 1 - par->div->div->r.y);
    offset--;
    seeked++;
  }

  return seeked;
}


/* Return 0 if the cursor is still inside the document,
 * If the cursor is before the beginning of the document return
 * a negative number equal to the number of characters before,
 * likewise return a positive number indicating the number
 * of characters after if the cursor is after the end of the doc.
 */
int document_eof(struct textbox_document *doc) {
  int i = pgstring_eof(doc->crsr->par->content, &doc->crsr->iterator);

  /* Allow one extra character past the end to be valid, since
   * we need a way to put the cursor at the end of the document.
   */
  if (i > 0)
    i--;

  DBG("returning %d\n",i);
  return i;
}

/* Delete the character after the cursor. If there's no cursor to delete,
 * document_eof() should be true
 */
g_error document_delete_char(struct textbox_document *doc) {
  g_error e;

  /* No character to delete? Merge this paragraph and the next paragraph */
  if (pgstring_eof(doc->crsr->par->content, &doc->crsr->iterator) > 0) {
    if (doc->crsr->par->next) {
      /* Save the iterator, since we don't want to advance the cursor */
      struct pgstr_iterator saved_iterator = doc->crsr->iterator;
      e = document_insert_string(doc, doc->crsr->par->next->content);
      errorcheck;
      doc->crsr->iterator = saved_iterator;
      paragraph_delete(doc->crsr->par->next);
    }
  }

  /* Yep, just delete the next character */
  else
    paragraph_delete_char(doc->crsr);

  return success;
}

/* Retrieve the paragraph associated with a divnode */
struct paragraph *document_get_div_par(struct divnode *div) {
  struct paragraph *par;
  
  /* The second gropnode refers to the paragraph, extract the handle from it */
  if (!div->grop) return NULL;
  if (!div->grop->next) return NULL;
  
  if (iserror(rdhandle((void**)&par,PG_TYPE_PARAGRAPH,-1,div->grop->next->param[0])))
    return NULL;
  
  return par;
}

/* Seek the cursor to the mouse location */
void document_mouseseek(struct textbox_document *doc, struct trigparam_mouse *m) {
  struct paragraph *par;
  
  par = document_get_div_par(m->cursor->ctx.div_under);
  if (par) {
    doc->crsr = &par->cursor;
    paragraph_movecursor(doc->crsr, par,
			 m->x - par->div->div->r.x,
			 m->y - par->div->div->r.y);
  }
  else {
    /* They clicked outside of all paragraphs. Assume this means
     * they clicked after the end of the document, and position the
     * cursor at the document's end.
     * FIXME: This could also mean a click within the document's margin!
     */
    document_seek(doc,0,PGSEEK_END);
  }
}

/* Delete the cursor before the cursor */
g_error document_backspace_char(struct textbox_document *doc) {
  document_seek(doc,-1,PGSEEK_CUR);
  if (document_eof(doc))
    document_seek(doc,1,PGSEEK_CUR);
  else
    return document_delete_char(doc);
  return success;
}

/* Like document_seek, but bound it at the edges of the document.
 * document_eof() will never be set after calling this.
 */
int document_bounded_seek(struct textbox_document *doc, s32 offset, int whence) {
  int correction;
  document_seek(doc,offset,whence);
  correction = -document_eof(doc);
  document_seek(doc,correction,PGSEEK_CUR);
  return offset + correction;
}

/* Insert a new paragraph after the one the cursor is on.
 */
g_error document_insert_paragraph(struct textbox_document *doc) {
  struct paragraph *oldpar = doc->crsr->par, *newpar;
  struct pgstr_iterator i;
  g_error e;
  
  e = textbox_new_par_div(&newpar, &oldpar->div->next, oldpar->background);
  errorcheck;

  if (oldpar->next)
    oldpar->next->prev = newpar;
  newpar->next = oldpar->next;
  newpar->prev = oldpar;
  oldpar->next = newpar;

  newpar->doc = doc;
  oldpar->div->owner->dt->flags |= DIVTREE_NEED_RESIZE;

  return success;
}


/******************************************************** Internal functions */

/* Create a paragraph and divnode tied to each other */
g_error textbox_new_par_div(struct paragraph **par, struct divnode **div,
			    struct divnode *background) {
  g_error e;
  handle hpar;
  struct gropctxt c;
  struct paragraph *old_par = *par;
  struct divnode *old_div = *div;

  /* Top-level divnode for this paragraph is used for formatting, it's
   * child is where the paragraph renders to.
   */
  e = newdiv(div,background->owner);
  errorcheck;
  (*div)->flags |= DIVNODE_SPLIT_TOP;
  (*div)->flags &= ~DIVNODE_UNDERCONSTRUCTION;

  /* We want to prevent the normal groplist clearing in div_rebuild
   * because the only way our build function has to know the
   * paragraph handle is by reading the previous groplist.
   */
  e = newdiv(&(*div)->div,background->owner);
  errorcheck;
  (*div)->div->build = &textbox_build_par_div;
  (*div)->div->flags |= DIVNODE_RAW_BUILD;
  (*div)->div->flags &= ~DIVNODE_UNDERCONSTRUCTION;

  /* New paragraph with associated handle */
  e = paragraph_new(par, *div);
  errorcheck;
  e = mkhandle(&hpar, PG_TYPE_PARAGRAPH, -1, *par);
  errorcheck;
  (*par)->background = background;

  /* build the initial groplist-
   * one incremental paragraph divnode, one normal one.
   * The size and theme related params will be filled in later.
   */
  gropctxt_init(&c,(*div)->div);
  addgrop(&c,PG_GROP_SETCOLOR);
  addgrop(&c,PG_GROP_PARAGRAPH);
  c.current->param[0] = hpar;
  addgrop(&c,PG_GROP_PARAGRAPH_INC);
  c.current->param[0] = hpar;
  c.current->flags |= PG_GROPF_INCREMENTAL;

  /* Relink the portion of the paragraph and divnode lists after this node */
  (*par)->next = old_par;
  (*div)->next = old_div;

  return success;
}

/* Rebuild function for paragraph divnodes, issued by textbox_new_par_div */
void textbox_build_par_div(struct gropctxt *c, u16 state, struct widget *self) {
  struct paragraph *par;
  struct gropnode *grops = *c->headpp;

  /* Retrieve the paragraph pointer from the existing gropnodes */
  if (iserror(rdhandle((void**)&par,PG_TYPE_PARAGRAPH,-1,grops->next->param[0])) || !par)
    return;

  /* Text color, cursor color, cursor width */
  grops->param[0] = VID(color_pgtohwr)(theme_lookup(state,PGTH_P_FGCOLOR));
  par->cursor.color = grops->param[0];
  par->cursor.width = theme_lookup(state,PGTH_P_CURSOR_WIDTH);

  /* If we're displaying to a different width, rewrap it */
  if (par->width != c->r.w) {
    par->width = c->r.w;
    paragraph_wrap(par,1);
  }

  /* Set gropnode sizes */
  grops->next->r.x = c->r.x;
  grops->next->r.y = c->r.y;
  grops->next->r.w = c->r.w;
  grops->next->r.h = c->r.h;
  grops->next->next->r = grops->next->r;

  DBG("Build, size: %d,%d,%d,%d preferred: %d,%d\n",
      c->r.x,c->r.y,c->r.w,c->r.h,c->owner->preferred.w,c->owner->preferred.h);
}

void textbox_delete_parlist(struct paragraph *list) {
  struct paragraph *condemn;
  while (list) {
    condemn = list;
    list = list->next;
    paragraph_delete(condemn);
  }
}

/* The End */
