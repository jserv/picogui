/* $Id: textbox.h,v 1.16 2002/10/28 01:00:21 micahjd Exp $
 *
 * textbox.h - Interface definitions for the textbox widget. This allows
 *             the main textbox widget functions and the text format loaders
 *             to access textbox utility functions.
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

#ifndef __TEXTBOX_H
#define __TEXTBOX_H

#include <pgserver/divtree.h>
#include <pgserver/render.h>
#include <pgserver/paragraph.h>

struct textbox_document {
  struct paragraph *par_list;     /* Doubly-linked list of paragraphs */
  struct divnode *container_div;  /* Divnode containing everything */
  struct paragraph_cursor *crsr;  /* Current cursor (and therefore current paragraph) */

  /* Owner-settable options */
  int password;                   /* Character to use for password hiding, if nonzero */
  unsigned int multiline : 1;
};

/* Constants for document_seek, same meaning as in fseek
*/
#define PGSEEK_SET   0   /* Seek from beginning */
#define PGSEEK_CUR   1   /* Seek from cursor    */
#define PGSEEK_END   2   /* Seek from the end   */

/******************************************************** Public Methods **/

g_error document_new(struct textbox_document **doc, struct divnode *container_div);
void document_delete(struct textbox_document *doc);

/************************* Editing */

/* Note that textbox_frontend provides all interactive editing. These functions
 * don't bother doing anything with the cursor or user interaction. This interface
 * is used under textbox_frontend, and in the format loaders.
 *
 * Note that it's fine for textbox_frontend to muck with the paragraph object directly
 * because it must manage visual things like cursor blink, mouse interaction, and that
 * can easily be done independent of the entire document's structure. However,
 * format loaders should only use this interface.
 */

/* Insert a character/string at the current cursor location.
 * Metadata if any is copied (the document will not reference
 * metadata nodes in the original string). The cursor location
 * afterward will be after the inserted string.
 */
g_error document_insert_char(struct textbox_document *doc, u32 ch, void *metadata);
g_error document_insert_string(struct textbox_document *doc, struct pgstring *str);

/* Seek in the document, same interface as fseek.
 * If this seeks past the end of the stream, document_eof will return true
 */
void document_seek(struct textbox_document *doc, s32 offset, int whence);

/* Seek up/down in the document, snapping the cursor to the nearest character */
void document_lineseek(struct textbox_document *doc, s32 offset);

/* Return true if the current cursor location is not valid */
int document_eof(struct textbox_document *doc);

/* Delete the character after the cursor. If there's no cursor to delete,
 * document_eof() should be true
 */
void document_delete_char(struct textbox_document *doc);

/* Retrieve the paragraph associated with a divnode */
struct paragraph *document_get_div_par(struct divnode *div);

/************************* Text format loaders */

struct txtformat {
  const char *name;
  g_error (*load)(struct textbox_document *doc, struct pgstring *str);
  g_error (*save)(struct textbox_document *doc, struct pgstring **str);
};

/* Load text of the specified format. */
g_error document_load(struct textbox_document *doc, const struct pgstring *format,
		      struct pgstring *str);
g_error document_save(struct textbox_document *doc, const struct pgstring *format,
		      struct pgstring **str);

/* Clear out the existing data and reset the cursor */
g_error document_nuke(struct textbox_document *doc);

extern struct txtformat text_formats[];

/* Format loader functions */
g_error plaintext_load(struct textbox_document *doc, struct pgstring *str);
g_error plaintext_save(struct textbox_document *doc, struct pgstring **str);
g_error html_load(struct textbox_document *doc, struct pgstring *str);
g_error html_save(struct textbox_document *doc, struct pgstring **str);

#endif /* __H_TEXTBOX */   

/* The End */


