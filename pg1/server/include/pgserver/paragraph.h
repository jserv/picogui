/* $Id$
 *
 * paragraph.h - Build upon the text storage capabilities of pgstring, adding word
 *               wrapping, formatting, and UI.
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

#ifndef __PARAGRAPH_H
#define __PARAGRAPH_H

#include <pgserver/pgstring.h>
#include <pgserver/divtree.h>

/* Types of metadata for paragraph_metadata */
#define PAR_META_FONT            1
#define PAR_META_COLOR           2
#define PAR_META_EMBED_WIDGET    3

struct textbox_document;

/* This structure is embedded in the metadata of the paragraph's pgstring,
 * and controls formatting and embedded objects.
 */
struct paragraph_metadata {
  int refcount;

  int type;          /* Selects which member of the union to use */
  union {
    struct font_descriptor *fd;
    hwrcolor color;
    struct widget *embedwidget;
  } u;

  /* Links in the paragraph's metadata list */
  struct paragraph_metadata *prev, *next;
};

/* Represent all the formatting necessary to display text in the
 * paragraph. This data can be altered by metadata nodes.
 */
struct paragraph_formatting {
  struct font_descriptor *fd;
  hwrcolor color;
};

/* Metadata for one line in the paragraph */
struct paragraph_line {
  /* If this line has been wrapped, wrapped will be 1,
   * char_width will be the number of characters on the line,
   * height will be the height of the line in pixels.
   */
  int char_width;
  int height;
  unsigned int wrapped : 1;

  /* This flag is set when the line is rewrapped, and cleared when
   * the line is rendered either incrementally or normally.
   */
  unsigned int wrap_need_render : 1;

  struct {
    /* The following information is only valid if the 'valid' flags
     * for this line _and all before it_ are nonzero
     */
    struct pgstr_iterator iterator;
    struct paragraph_formatting fmt;
    unsigned int valid : 1;
  } cache;
  
  struct paragraph_line *prev, *next;
};

struct paragraph;

/* Information about the cursor location in the paragraph */
struct paragraph_cursor {
  struct paragraph *par;
  struct paragraph_line *line;
  struct pgstr_iterator iterator; /* The selected cursor, after the insertion point */

  /* The appearance of the cursor. Right now only solid rectangular cursors
   * are supported. This could be extended to using fillstyles if needed.
   */
  int width;
  hwrcolor color;

  /* The location this cursor was most recently rendered
   * at, in absolute logical coordinates. 
   */
  struct pgrect last_rect;

  unsigned int visible : 1;
};

struct paragraph {
  struct pgstring *content;
  struct paragraph_line *lines;
  struct divnode *background;
  int width;                      /* This is set before wrapping, by the owner */
  int height;                     /* Calculated during wrapping */
  struct paragraph_metadata *metanodes;   /* List of metadata nodes for this paragraph */  
  struct divnode *div;            /* Divnode for formatting this paragraph relative
				   * to other paragraphs. The contents are
				   * rendered to the divnode div->div. 

  /* Information about the most recent change, for incremental rendering.
   */
  struct {
    struct paragraph_line *line;
    struct pgstr_iterator start;
    int nchars;                  /* Number of changed characters, or 0 to keep rendering
				  * as long as the scrolling was changed.
				  */
  } last_change;

  /* NOTE: It is possible to use other cursors than this one in most places,
   * but this cursor is the only one that may be used for interactive
   * editing. This is because incremental redraws rely on this cursor having
   * a correct 'line' member, so the cursor must be updated when it wraps
   * onto a different line.
   */
  struct paragraph_cursor cursor;

  /* Document this paragraph is contained in if any, or NULL.
   * This is needed for document-wide settings, like password rendering
   * or line wrapping options.
   */
  struct textbox_document *doc;

  /* Previous and next paragraphs in cursor navigation order */
  struct paragraph *prev, *next;
};

/******************************************************** Public Methods **/

/* div is the divnode used for formatting, div->div is what the paragraph
 * uses for all its sizing and rendering. The paragraph doesn't need to
 * know about div, but storing it in the paragraph makes things much easier
 * for textbox_document.
 */
g_error paragraph_new(struct paragraph **par, struct divnode *div);
void paragraph_delete(struct paragraph *par);

/* Draw a gropnode containing a PG_GROP_PARAGRAPH */
void paragraph_render(struct groprender *r, struct gropnode *n);

/* Draw the PG_GROP_PARAGRAPH_INC gropnode, only drawing changed areas */
void paragraph_render_inc(struct groprender *r, struct gropnode *n);

/* Move the specified cursor as close as possible to the location x,y
 * relative to the specified paragraph
 */
void paragraph_movecursor(struct paragraph_cursor *crsr, 
			  struct paragraph *par, int x, int y);

/* Move the cursor in the paragraph, works like fseek() */
void paragraph_seekcursor(struct paragraph_cursor *crsr, int offset, int whence);

/* Delete a character at the cursor */
g_error paragraph_delete_char(struct paragraph_cursor *crsr);

/* Insert a character at the cursor */
g_error paragraph_insert_char(struct paragraph_cursor *crsr, u32 ch, 
			      struct paragraph_metadata *meta);

/* Hide/show the cursor */
void paragraph_hide_cursor(struct paragraph_cursor *crsr);
void paragraph_show_cursor(struct paragraph_cursor *crsr);

/* Make sure the cursor is visible in the scrolled area
 * if autoscrolling is on and this is possible, otherwise do nothing.
 */
void paragraph_scroll_to_cursor(struct paragraph_cursor *crsr);

/* Set this cursor as the last change location, for incremental redraws */
void paragraph_set_last_change(struct paragraph_cursor *crsr);

#endif /* __PARAGRAPH_H */
/* The End */
