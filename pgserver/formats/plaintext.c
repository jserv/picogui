/* $Id: plaintext.c,v 1.9 2002/09/15 10:51:46 micahjd Exp $
 *
 * plaintext.c - Load and save plain unformatted text into the textbox widget
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

/* Trivial to add a plaintext string to the document... 
 * FIXME: it shouldn't be possible to add strings with metadata, this could be
 *        a security hole if there was a way to put metadata in strings
 *        coming from the client side.
 */
g_error plaintext_load(struct textbox_document *doc, struct pgstring *str) {
  return document_insert_string(doc,str);
}

/* Create a new pgstring, and traverse the list of paragraphs adding each one */
g_error plaintext_save(struct textbox_document *doc, struct pgstring **str) {
  struct paragraph *p;
  g_error e;
  struct pgstr_iterator i = PGSTR_I_NULL;
  
  e = pgstring_new(str, PGSTR_ENCODE_UTF8, 0, NULL);
  errorcheck;

  for (p=doc->par_list;p;p=p->next) {
    e = pgstring_insert_string(*str,&i,p->content);
    errorcheck;
    e = pgstring_insert_char(*str,&i,'\n',NULL);
    errorcheck;
  }

  return success;
}


/* The End */





