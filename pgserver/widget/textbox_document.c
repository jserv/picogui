/* $Id: textbox_document.c,v 1.41 2002/10/11 11:58:45 micahjd Exp $
 *
 * textbox_document.c - High-level interface for managing documents
 *                      with multiple paragraphs, formatting, and
 *                      other embedded objects.
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

  /* Initial paragraph, stick the cursor in it */
  e = textbox_new_par_div(&(*doc)->par_list, &container_div->div, container_div);
  errorcheck;
  (*doc)->crsr = &(*doc)->par_list->cursor;

  return success;
}

void document_delete(struct textbox_document *doc) {
  struct paragraph *p, *condemn;

  /* Free all paragraphs we have */
  p = doc->par_list;
  while (p) {
    condemn = p;
    p = p->next;
    handle_free(-1,hlookup(condemn,NULL));
  }
  
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
    int was_visible = doc->crsr->visible;

    e = textbox_new_par_div(&doc->crsr->par->next, &doc->crsr->par->div->next,
			    doc->crsr->par->background);
    errorcheck;
    paragraph_hide_cursor(doc->crsr);
    doc->crsr->par->div->owner->dt->flags |= DIVTREE_NEED_RESIZE;
    doc->crsr = &doc->crsr->par->next->cursor;
    if (was_visible)
      paragraph_show_cursor(doc->crsr);
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
  struct pgstr_iterator i = PGSTR_I_NULL;
  u32 ch;
  void *meta;
  g_error e;

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
  paragraph_seekcursor(doc->crsr,offset);
}

/* Seek up/down in the document, snapping the cursor to the nearest character */
void document_lineseek(struct textbox_document *doc, s32 offset) {
  struct pgstr_iterator i;

  /* Repeat the below code for offsets other than -1 or 1 */
  if (offset < -1)
    while (offset++)
      document_lineseek(doc,-1);
  else if (offset > 1)
    while (offset--)
      document_lineseek(doc,1);
  else if (!offset)
    return;

  /* Find the line (and paragraph) we want */
  /* FIXME: finish this function */
}

/* Return true if the current cursor location is not valid */
int document_eof(struct textbox_document *doc) {
  return doc->crsr->iterator.invalid;
}

/* Delete the character after the cursor. If there's no cursor to delete,
 * document_eof() should be true
 */
void document_delete_char(struct textbox_document *doc) {

  /* No character to delete? Should we merge this paragraph and the next paragraph? */
  if (doc->crsr->iterator.invalid) {
    
  }

  /* Yep, just delete the next character */
  else {
    paragraph_delete_char(doc->crsr);
  }
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
      c->r.x,c->r.y,c->r.w,c->r.h,c->owner->pw,c->owner->ph);
}

/* The End */
