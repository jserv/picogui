/* $Id: plaintext.c,v 1.1 2001/11/05 04:06:38 micahjd Exp $
 *
 * plaintext.c - Load plain text into the textbox widget
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
#include <pgserver/textbox.h>

g_error plaintext_word(struct textbox_cursor *c, const u8 *start,
		       const u8 *end);

g_error plaintext_load(struct textbox_cursor *c, const u8 *data, u32 datalen) {
  const u8 *start = data;
  const u8 *end = data+datalen-1;
  const u8 *fragment;
  g_error e;

  for (fragment = NULL;start <= end;start++) {
    if (fragment) {
      /* Look for the end */

      if (isspace(*start)) {
	e = plaintext_word(c,fragment,start-1);
	errorcheck;
	fragment = NULL;

	if (*start == '\n')
	  e = text_insert_linebreak(c);
	else
	  e = text_insert_wordbreak(c);
	errorcheck;
      }
    }
    else {
      /* Look for the beginning */

      if (isspace(*start)) {
	if (*start == '\n')
	  e = text_insert_linebreak(c);
	else
	  e = text_insert_wordbreak(c);
	errorcheck;
      }
      else
	fragment = start;
    }
  }
  if (fragment) {
    e = plaintext_word(c,fragment,start-1);
    errorcheck;
  }

  return sucess;
}

g_error plaintext_word(struct textbox_cursor *c, const u8 *start,
		       const u8 *end) {
  int length;
  char *str;
  g_error e;

  /* Allocate a new string */
  length = end-start+1;
  e = g_malloc((void**)&str, length+1);
  errorcheck;
  str[length] = 0;  

  strncpy(str,start,length);
  
  /* Send it to the textbox */
  return text_insert_string(c,str,0);
}

/* The End */





