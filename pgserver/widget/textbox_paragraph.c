/* $Id: textbox_paragraph.c,v 1.2 2002/09/15 10:51:50 micahjd Exp $
 *
 * textbox_paragraph.c - Build upon the text storage capabilities
 *                       of pgstring, adding word wrapping, formatting,
 *                       and UI. This code isn't specific to the textbox
 *                       widget, but it should only be used by the textbox
 *                       since textbox_document provides the code to manage
 *                       collections of paragraphs together.
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
#include <pgserver/pgstring.h>
#include <pgserver/paragraph.h>
#include <pgserver/render.h>
#include <pgserver/appmgr.h>
#include <pgserver/widget.h>
#include <ctype.h>

#ifdef DEBUG_TEXTBOX
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

/* Wrap one line (longer description below) */
g_error paragraph_wrap_line(struct paragraph *par, struct paragraph_line **line,
			    int *cache_valid, int force);

/* Wrap the entire paragraph, forcing a rewrap of every line if 'force' is nonzero  */
g_error paragraph_wrap(struct paragraph *par, int force);

/* Determine the amount of vertical space to reserve for text in a given font */
int paragraph_font_height(struct fontdesc *fd);

/* Skip from one line to the next, updating the metadata and iterator cache. 
 * - On the first call, *line should equal par->line and *valid should be 1.
 * - On entering this function, (*line)->cache->valid must be 1.
 * - On exiting this function, *line will point to the next line, and (*line)->cache->valid
 *   will be 1.
 */
void paragraph_line_skip(struct paragraph *par, struct paragraph_line **line, int *valid);

/* Apply a metadata node's formatting */
void paragraph_apply_metadata(struct paragraph_metadata *meta,
			      struct paragraph_formatting *fmt);

/* Rerender a line or part of a line, clearing it with the paragraph's background first */
void paragraph_rerender_line(struct groprender *r, struct gropnode *n, 
			     struct paragraph *par, struct paragraph_line *line,
			     s16 *y, struct pgstr_iterator *skip_to, int *nchars);

/* Set this cursor as the last change location, for incremental redraws */
void paragraph_set_last_change(struct paragraph_cursor *crsr);

/* Draw the cursor at the given location */
void paragraph_render_cursor(struct groprender *r, struct paragraph_cursor *crsr,
			     s16 x, s16 y);

/* Mark the cursor and the area overlapping it for rerendering */
void paragraph_update_cursor(struct paragraph_cursor *crsr);

/******************************************************** Public Methods **/

g_error paragraph_new(struct paragraph **par, struct divnode *div) {
  g_error e;

  e = g_malloc((void**)par, sizeof(struct paragraph));
  errorcheck;
  memset(*par, 0, sizeof(struct paragraph));

  (*par)->div = div;

  /* Make an empty string */
  e = pgstring_new(&(*par)->content,PGSTR_ENCODE_ASCII,0,NULL);
  errorcheck;

  /* Create the first line */ 
  e = g_malloc((void**)&(*par)->lines,sizeof(struct paragraph_line));
  errorcheck;
  memset((*par)->lines,0,sizeof(struct paragraph_line));
 
  /* Give it default formatting */
  e = rdhandle((void**)&(*par)->lines->cache.fmt.fd,PG_TYPE_FONTDESC,-1,res[PGRES_DEFAULT_FONT]);
  errorcheck;
  (*par)->lines->cache.fmt.color = VID(color_pgtohwr)(theme_lookup(PGTH_O_DEFAULT, PGTH_P_FGCOLOR));
  (*par)->lines->cache.valid = 1;

  /* Set the first line height to the font height. This will be overridden 
   * as soon as there's text in here, but until then this is necessary for
   * sizing the cursor and this paragraph's surroundings properly.
   */
  (*par)->div->div->ph = (*par)->height = (*par)->lines->height = 
    paragraph_font_height((*par)->lines->cache.fmt.fd);

  /* Initially hide the cursor and put it at the beginning */
  (*par)->cursor.visible = 0;
  paragraph_movecursor(&(*par)->cursor,*par,0,0);

  return success;
}

void paragraph_delete(struct paragraph *par) {
  struct paragraph_line *p, *dead;

  /* Delete linked list of lines */
  p = par->lines;
  while (p) {
    dead = p;
    p = p->next;
    g_free(dead);
  }

  pgstring_delete(par->content);
  g_free(par);
}

/* Draw a gropnode containing a PG_GROP_PARAGRAPH */
void paragraph_render(struct groprender *r, struct gropnode *n) {
  struct paragraph *par;
  struct pgstr_iterator p = PGSTR_I_NULL;
  u32 ch;
  s16 x,y;
  struct paragraph_metadata *meta;
  struct paragraph_line *line;
  struct paragraph_formatting fmt;
  int i, old_x, draw_cursor;
  int valid = 1;

  if (iserror(rdhandle((void**)&par,PG_TYPE_PARAGRAPH,-1,n->param[0])) || !par)
    return;

  line = par->lines;
  x = n->r.x;
  y = n->r.y;
  if (!line)
    return;
  fmt = line->cache.fmt;

  DBG("Normal render\n");
  
  /* Line rendering loop */
  while (line) {
    line->wrap_need_render = 0;

    /* Skip lines completely off the top of our clipping rectangle */
    if (line->height + y < r->clip.y1) {
      y += line->height;
      paragraph_line_skip(par, &line, &valid);
      if (!line)
	return;
      fmt = line->cache.fmt;
      p = line->cache.iterator;
    }

    /* Quit if we're past the bottom of the clipping rectangle */
    else if (y > r->clip.y2) {
      return;
    }

    /* Render the characters and/or metadata in this line */
    else {
      for (i=line->char_width;i;i--) {
	draw_cursor = par->cursor.visible && 
	  !pgstring_iteratorcmp(&p, &par->cursor.iterator);
	ch = pgstring_decode_meta(par->content, &p, (void**) &meta);
	if (meta) 
	  paragraph_apply_metadata(meta, &fmt);
	old_x = x;
	if (ch)
	  outchar(r->output, fmt.fd, &x, &y, fmt.color, ch, &r->clip, r->lgop, 0);
	if (draw_cursor)
	  paragraph_render_cursor(r,&par->cursor,old_x,y);
      }

      /* Drat, there's a special case: if the cursor is at the very end of the
       * paragraph. (We don't have this problem at the end of lines, since lines
       * always break on a whitespace- the cursor is just positioned before that
       * whitespace)
       */
      if ((!line->next) && par->cursor.visible &&
	  !pgstring_iteratorcmp(&p, &par->cursor.iterator))
	  paragraph_render_cursor(r,&par->cursor,x,y);

      /* Next line */
      y += line->height;
      x = n->r.x;
      line = line->next;
    }
  }
}

/* Draw a gropnode containing a PG_GROP_PARAGRAPH_INC,
 * only draw the area indicated in the "last_change" member
 * of the paragraph.
 */
void paragraph_render_inc(struct groprender *r, struct gropnode *n) {
  struct paragraph *par;
  struct paragraph_line *line;
  s16 y;
  int valid = 1;

  DBG("Incremental render\n");

  if (iserror(rdhandle((void**)&par,PG_TYPE_PARAGRAPH,-1,n->param[0])) || !par)
    return;
  line = par->lines;
  y = n->r.y;
  if (!line)
    return;

  /* Skip lines until we get to the first one we want to render */
  while (line != par->last_change.line) {
    y += line->height;
    paragraph_line_skip(par, &line, &valid);
    if (!line)
      return;
  }
  
  /* Render the portion of this line including and after the change */
  paragraph_rerender_line(r,n,par,line,&y,&par->last_change.start,&par->last_change.nchars);
  y += line->height;
  paragraph_line_skip(par, &line, &valid);

  /* Render all the lines afterwards that need rendering because
   * they were rewrapped.
   */
  while (line) {
    /* Quit if we're past the bottom of the clipping rectangle */
    if (y > r->clip.y2)
      break;

    if (line->wrap_need_render)
      paragraph_rerender_line(r,n,par,line,&y,NULL,NULL);
    line->wrap_need_render = 0;

    y += line->height;
    paragraph_line_skip(par, &line, &valid);
  }
}

/* Move a paragraph's cursor as close as possible to the given coordinates */
void paragraph_movecursor(struct paragraph_cursor *crsr,
			  struct paragraph *par, int x, int y) {
  int line_y = 0, i;
  s16 line_x = 0;
  int cache_valid = 1;
  struct paragraph_formatting fmt;
  u32 ch;
  struct paragraph_metadata *meta;

  crsr->par = par;
  crsr->line = par->lines;
  if (!crsr->line)
    return;

  /* Before the first line? */
  if (y < 0) {
    crsr->iterator = crsr->line->cache.iterator;
    /* Point the cursor before the first character */
    pgstring_seek(par->content, &crsr->iterator, 0);
  }
  else {

    /* First find the line */
    while (y > line_y + crsr->line->height) {
      if (!crsr->line->next) {
	/* We've run past the last line.. position the cursor at the end */
	crsr->iterator = crsr->line->cache.iterator;
	while (pgstring_decode(par->content, &crsr->iterator));
	DBG("Moved cursor to END at %d,%p,%d\n",
	    crsr->iterator.offset,crsr->iterator.buffer,crsr->iterator.invalid);
	return;
      }
      line_y += crsr->line->height;
      paragraph_line_skip(par,&crsr->line,&cache_valid);
    }

    /* Now the character */
    crsr->iterator = crsr->line->cache.iterator;
    fmt = crsr->line->cache.fmt;
    for (i=crsr->line->char_width;i;i--) {
      ch = pgstring_decode_meta(par->content, &crsr->iterator, (void**) &meta);
      if (meta)
	paragraph_apply_metadata(meta,&fmt);
      outchar_fake(fmt.fd,&line_x,ch);
      if (line_x > x) {
	/* We just passed the character that was clicked */
	pgstring_seek(par->content, &crsr->iterator, -1);
	break;
      }
    }

    /* If we reached the end of the line and the last character was a space,
     * go back one so the cursor is positioned at that space
     */
    if (crsr->line->char_width && isspace(ch) && !i)
      pgstring_seek(par->content, &crsr->iterator, -1);
  }
  DBG("Moved cursor to %d,%p,%d\n",
      crsr->iterator.offset,crsr->iterator.buffer,crsr->iterator.invalid);
}

/* Move the cursor back or forward by some number of characters. This handles
 * moving across paragraph boundaries correctly.
 */
void paragraph_seekcursor(struct paragraph_cursor *crsr, int num_chars) {
  pgstring_seek(crsr->par->content, &crsr->iterator, num_chars);
}

/* Delete a character at the cursor */
g_error paragraph_delete_char(struct paragraph_cursor *crsr) {
  paragraph_set_last_change(crsr);
  return pgstring_delete_char(crsr->par->content, &crsr->iterator);
}

/* Insert a character at the cursor */
g_error paragraph_insert_char(struct paragraph_cursor *crsr, u32 ch, 
			      struct paragraph_metadata *meta) {
  paragraph_set_last_change(crsr);
  return pgstring_insert_char(crsr->par->content,&crsr->iterator,ch,meta);
}

void paragraph_update_cursor(struct paragraph_cursor *crsr) {
  /* Mark the changed region as only the character the cursor is sitting at */
  paragraph_set_last_change(crsr);
  crsr->par->last_change.nchars = 1;
}

void paragraph_hide_cursor(struct paragraph_cursor *crsr) {
  /* Nothing to hide? */
  if (!crsr->par)
    return;

  crsr->visible = 0;
  paragraph_update_cursor(crsr);

  /* To avoid having to store this as a separate discontinuous update region,
   * just update the area now */
  update(crsr->par->div->div,1);
}

void paragraph_show_cursor(struct paragraph_cursor *crsr) {
  /* Make the cursor visible, and take it's color from the default foreground for now */
  crsr->color = VID(color_pgtohwr)(theme_lookup(PGTH_O_DEFAULT, PGTH_P_FGCOLOR));
  crsr->visible = 1;
  paragraph_update_cursor(crsr);
}

/******************************************************** Internal Methods **/

/* Wrap one line of the paragraph to the width specified in the paragraph.
 *
 * Inputs:
 *   '*line' should be an existing line in the paragraph structure, with a valid
 *           metadata cache.
 *   'par' should be the paragraph containing the line to wrap
 *   'cache_valid' is a flag used to remember whether line caches are valid,
 *      it should be "1" for the first line
 *   'force' forces the line to rewrap if it's nonzero
 *
 * Outputs:
 *   '*line' will point to the next line that needs wrapping, or NULL if done
 */
g_error paragraph_wrap_line(struct paragraph *par, struct paragraph_line **line,
			    int *cache_valid, int force) {
  struct paragraph_metadata *meta;
  g_error e;
  s16 x;
  u32 ch;
  int spaces;
  int old_height;
  struct paragraph_line *deadline;
  struct pgstr_iterator i;
  int old_char_width;
  struct fontdesc *fd;

  /* If this line doesn't need wrapping, skip it */
  if ((*line)->wrapped && !force) {
    paragraph_line_skip(par,line,cache_valid);
    return success;
  }

  DBG("In paragraph_wrap_line, line %p, wrapped=%d, valid=%d, force=%d\n",
	 (*line),(*line)->wrapped,(*line)->cache.valid,force);

  /* Reset this line */
  old_char_width = (*line)->char_width;
  (*line)->char_width = 0;
  old_height = (*line)->height;
  (*line)->height = paragraph_font_height((*line)->cache.fmt.fd);
  x = 0;
  spaces = 0;
  fd = (*line)->cache.fmt.fd;
  i = (*line)->cache.iterator;
  (*line)->wrapped = 1;

  /* Now accumulate characters in the line until we reach our wrapping point */
  for (;;) {
    /* If we're at the cursor position, set the cursor line.
     * This ensures that the cursor tracks lines as they wrap.
     */
    if (!pgstring_iteratorcmp(&i,&par->cursor.iterator)) {
#ifdef DEBUG_FILE
      if (par->cursor.line != *line) {
	DBG("Cursor line is wrong, %p != %p\n",par->cursor.line,*line);
      }
#endif
      par->cursor.line = *line;
    }    

    ch = pgstring_decode_meta(par->content, &i, (void**) &meta);

    if (meta && meta->type == PAR_META_FONT) {
      int h;
      fd = meta->u.fd;
      h = paragraph_font_height(fd);
      if (h > (*line)->height)
	(*line)->height = h;
    }
    if (ch) {
      if (isspace(ch))
	spaces++;
      outchar_fake(fd,&x,ch);
      (*line)->char_width++;
      
      /* Over our limit yet? */
      if (x >= par->width && spaces) {

	/* Step back until we get to a breaking space */
	pgstring_seek(par->content,&i,-1);
	for (;;) {
	  ch = pgstring_decode_meta(par->content, &i, (void**)&meta);
	  
	  /* Reapply font changes in reverse order */
	  if (meta && meta->type == PAR_META_FONT) {
	    fd = meta->u.fd;
	    break;
	  }
	  
	  /* Keep going until we hit a space */
	  if (isspace(ch))
	    break;
	  
	  /* Reverse our previous step */
	  (*line)->char_width--;
	  pgstring_seek(par->content,&i,-2);
	}

	/* Apply changes to this line height */
	par->height += (*line)->height - old_height;

	/* Next line.. create it if necessary */
	if (!(*line)->next) {
	  e = g_malloc((void**)&(*line)->next, sizeof(struct paragraph_line));
	  errorcheck;
	  memset((*line)->next,0,sizeof(struct paragraph_line));
	  (*line)->next->prev = *line;
	}

	/* The next line needs rendering now
	 * FIXME: This needs to be more efficient, but the problem of determining
	 *        if the line should be rendered isn't as simple as comparing pointers
	 *        or widths, since inserting or deleting text will change pointers
	 *        and comparing width isn't reliable.
	 */
	(*line)->next->wrapped = 0;
	(*line)->next->cache.valid = 0;
	
	/* Also make a note that this line needs redrawing now */
	(*line)->wrap_need_render = 1;

	paragraph_line_skip(par,line,cache_valid);
	return success;
      }
    }
    else  /* if (!ch) */
      break;
  }

  /* Apply changes to this line height */
  par->height += (*line)->height - old_height;

  /* Redraw this line if it's changed */
  /* FIXME: This doesn't test whether anything's changed, see above FIXME */
  (*line)->wrap_need_render = 1;

  /* All done. If there were more lines after this, delete them all */
  deadline = *line;
  *line = (*line)->next;
  deadline->next = NULL;
  while (*line) {
    deadline = *line;
    *line = (*line)->next;
    
    /* Delete the height the line we're about to delete had */
    par->height -= deadline->height;

    g_free(deadline);
  }

  return success;
}

/* Wrap the entire paragraph  */
g_error paragraph_wrap(struct paragraph *par, int force) {
  struct paragraph_line *line;
  int valid = 1;
  g_error e;

  /* If there's no text, don't worry about the wrapping 
   * (we don't insert or delete the first line)
   */
  if (!par->content->num_chars)
    return success;

  /* Wrap each line until done */
  line = par->lines;
  while (line) {
    e = paragraph_wrap_line(par,&line,&valid,force);
    errorcheck;
  }

  /* Update the layout engine with our new preferred size */
  if (par->div->div->ph != par->height) {
    par->div->div->ph = par->height;
    par->div->div->owner->dt->flags |= DIVTREE_NEED_RESIZE;
  }

  return success;
}

/* Determine the amount of vertical space to reserve for text in a given font */
int paragraph_font_height(struct fontdesc *fd) {
  return fd->font->h + fd->interline_space + fd->font->descent;
}

/* Skip from one line to the next, updating the metadata and iterator cache. 
 * - On the first call, *line should equal par->line and *valid should be 1.
 * - On entering this function, (*line)->cache->valid must be 1.
 * - On exiting this function, *line will point to the next line, and (*line)->cache->valid
 *   will be 1.
 */
void paragraph_line_skip(struct paragraph *par, struct paragraph_line **line, int *valid) {
  struct paragraph_line *thisline = *line;
  struct paragraph_line *nextline = thisline->next;
  int i;
  struct paragraph_metadata *meta;

  /* Is there another line to propagate cache to? */
  if (nextline) {
    
    /* We're assuming on entry that the current line is valid. Our task is to propagate
     * cache to the next line if necessary. If the metadata for the next line and all before
     * it is marked valid, we don't need to.
     */
    *valid &= nextline->cache.valid;
  
    /* Do we still need to make the cache for the next line? */
    if (!*valid) {
      /* Get a starting point for iteration, and set the 'valid' flag to 1 */
      nextline->cache = thisline->cache;

      /* Update the iterator and metadata as we traverse the line */
      for (i=thisline->char_width;i;i--) {
	pgstring_decode_meta(par->content, &nextline->cache.iterator, (void**) &meta);
	if (meta)
	  paragraph_apply_metadata(meta, &nextline->cache.fmt);
      }
    }
  }

  /* Traverse to the next line */
  *line = nextline;
}

/* Apply a metadata node's formatting */
void paragraph_apply_metadata(struct paragraph_metadata *meta,
			      struct paragraph_formatting *fmt) {
  switch (meta->type) {

  case PAR_META_FONT:
    fmt->fd = meta->u.fd;
    break;

  case PAR_META_COLOR:
    fmt->color = meta->u.color;
    break;

  }
}

/* Rerender a line or part of a line, clearing it with the paragraph's background first */
void paragraph_rerender_line(struct groprender *r, struct gropnode *n, 
			     struct paragraph *par, struct paragraph_line *line,
			     s16 *y, struct pgstr_iterator *skip_to, int *nchars) {
  u32 ch;
  s16 old_x;
  s16 x = n->r.x;
  int i = line->char_width;
  struct paragraph_metadata *meta;
  struct pgstr_iterator p = line->cache.iterator;
  struct paragraph_formatting fmt = line->cache.fmt;
  struct quad clip;
  int draw_cursor;

  /* If nchars starts out 0, there's no limit to the number of characters we should
   * draw after the cursor.
   */
  if (nchars && !*nchars)
    nchars = NULL;

  DBG("Rerender line\n");

  /* Skip until we get to the beginning of the change */
  if (skip_to) {
    while (pgstring_iteratorcmp(&p,skip_to)) {
      ch = pgstring_decode_meta(par->content, &p, (void**) &meta);
      if (meta) 
	paragraph_apply_metadata(meta, &fmt);
      if (!ch)
	return;
      outchar_fake(fmt.fd, &x, ch);
      i--;
    }
  }

  DBG("skipped to beginning of the change\n");

  /* Render the background behind the portion of the line after the change
   * We only do it in one big chunk if we're rendering all the characters on the line.
   * Otherwise do it behind each character. Usually this only gets an nchar of 1,
   * so adding up the space needed beforehand wouldn't necessarily be faster.
   */
  if (!nchars) {
    clip.x1 = x;
    clip.y1 = *y;
    clip.x2 = n->r.x + n->r.w - 1;
    clip.y2 = *y + line->height - 1;
    grop_render(par->background, &clip);
  }

#if 0 
  /* If this is the last line, also clear any old lines below this one */
  if (!line->next) {
    /* FIXME: This makes editing on the last line very slow... */
    clip.x1 = n->r.x;
    clip.y1 = *y + line->height;
    clip.x2 = n->r.x + n->r.w - 1;
    clip.y2 = r->clip.y2;
    grop_render(par->background, &clip);
  }
#endif

  /* Now render the remainder of the changed line over our freshly clean background.
   * If we're redrawing where the cursor is, put the cursor back where it was.
   */
  DBG("Entering loop with i=%d, iterator at %d,%p,%d, cursor at %d,%p,%d\n",
      i,p.offset,p.buffer,p.invalid,par->cursor.iterator.offset, par->cursor.iterator.buffer,
      par->cursor.iterator.invalid);

  /* The >= here instead of > means we're going to look at the first character
   * in the next line too. We need this to draw cursors at the end of the line.
   */
  for (;i>=0;i--) {

    draw_cursor = par->cursor.visible && 
      !pgstring_iteratorcmp(&p, &par->cursor.iterator);
   
    ch = pgstring_decode_meta(par->content, &p, (void**) &meta);
    if (meta) 
      paragraph_apply_metadata(meta, &fmt);
    old_x = x;

    if (nchars) {
      /* If we're counting characters here, draw the background
       * on each character separately 
       */
      outchar_fake(fmt.fd, &x, ch);
      clip.x1 = old_x;
      clip.y1 = *y;
      clip.x2 = x - 1;
      clip.y2 = *y + line->height - 1;
      grop_render(par->background, &clip);
      x = old_x;
    }

    if (ch && i)
      outchar(r->output, fmt.fd, &x, y, fmt.color, ch, &r->clip, r->lgop, 0);    

    if (draw_cursor) {
      DBG("Draw_cursor at %d,%d\n",old_x,*y);
      paragraph_render_cursor(r,&par->cursor,old_x,*y);
    }

    if (nchars && !--*nchars)
      return;
  }
  DBG("Exited loop with i=%d\n",i);
}

/* Set this cursor as the last change location, for incremental redraws */
void paragraph_set_last_change(struct paragraph_cursor *crsr) {
  crsr->par->last_change.line = crsr->line;
  crsr->par->last_change.start = crsr->iterator;
  if (crsr->line->prev)
    crsr->line->prev->wrapped = 0;
  crsr->line->wrapped = 0;
  crsr->par->last_change.start.invalid = 0;
  crsr->par->last_change.nchars = 0;
  
  /* Schedule an incremental redraw */
  crsr->par->div->div->flags |= DIVNODE_INCREMENTAL;
  crsr->par->div->div->owner->dt->flags |= DIVTREE_NEED_REDRAW;
}

/* Draw the cursor at the given location */
void paragraph_render_cursor(struct groprender *r, struct paragraph_cursor *crsr,
			     s16 x, s16 y) {
  struct gropnode n;
  n.type = PG_GROP_RECT;
  n.flags = PG_GROPF_COLORED;
  n.r.x = x;
  n.r.y = y;
  n.r.w = crsr->width;
  n.r.h = crsr->line->height;
  n.param[0] = crsr->color;

  gropnode_clip(r,&n);
  gropnode_draw(r,&n);
}

/* The End */