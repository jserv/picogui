/* $Id: textbox.h,v 1.5 2001/10/07 07:01:24 micahjd Exp $
 *
 * textbox.h - Interface definitions for the textbox widget. This allows
 *             the main textbox widget functions and the text format loaders
 *             to access textbox utility functions.
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

#ifndef __TEXTBOX_H
#define __TEXTBOX_H

#include <pgserver/divtree.h>
#include <pgserver/render.h>

/************************* Data structures */

/* Formatting is stored on a stack. Each node records format _changes_ */
struct formatnode {
  handle fontdef;            /* Font definition for this formatting */
  int font_refcnt;           /* Number of divnodes using the font */
  hwrcolor color;            /* Text foreground color */
  struct formatnode *next;
};

/* A description of the cursor position and formatting */
struct textbox_cursor {
  struct widget *widget;     /* Widget owning this text */
  struct divnode *head;      /* First line in the document */
  struct divnode *c_line;    /* Line the cursor is in */
  struct divnode *c_div;     /* Divnode the cursor is in */
  struct gropctxt c_gctx;    /* Gropnode context the cursor is in */
  s16 c_gx, c_gy;            /* Cursor location within gropnode */
  struct formatnode *f_top;  /* Top of formatting stack */
  struct formatnode *f_used; /* Formats not on stack with refcnt != 0 */
  struct gropnode **caret;   /* Gropnode for the caret, if it is on */
  struct divnode *caret_div; /* Divnode containing the caret */
};

/************************* Formatting */

/* Add a format node to the stack */
g_error text_format_add(struct textbox_cursor *c, struct formatnode *f);
/* Add a node to change the text color */
g_error text_format_color(struct textbox_cursor *c, pgcolor color);
/* Add a node to change the font completely */
g_error text_format_font(struct textbox_cursor *c, handle font);
/* Add a node to change font flag(s) */
g_error text_format_modifyfont(struct textbox_cursor *c,
			       u32 flags_on, u32 flags_off,s16 size_delta);
/* Remove the topmost layer of formatting */
void text_unformat_top(struct textbox_cursor *c);
/* Remove all formatting */
void text_unformat_all(struct textbox_cursor *c);

/************************* Text */

/* Inserts a breaking space between words at the cursor */
g_error text_insert_wordbreak(struct textbox_cursor *c);
/* Begin a new paragraph at the cursor */
g_error text_insert_linebreak(struct textbox_cursor *c);
/* Insert text with the current formatting at the cursor. This will not
 * generate breaking spaces. */
g_error text_insert_string(struct textbox_cursor *c, const char *str);

/************************* Editing */

/* Delete the character or other object after the cursor */
void text_delete_next(struct textbox_cursor *c);
/* Delete the character or other object before the cursor */
void text_delete_prev(struct textbox_cursor *c);
/* Combine gropnodes when possible to reduce memory consumption */
g_error text_compact(struct textbox_cursor *c);
/* Show the caret at the current cursor position */
g_error text_caret_on(struct textbox_cursor *c);
/* Hide the caret at the current cursor position */
g_error text_caret_off(struct textbox_cursor *c);

#endif /* __H_TEXTBOX */   

/* The End */


