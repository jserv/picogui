/* $Id: plaintext.c,v 1.7 2002/02/03 16:07:58 lonetech Exp $
 *
 * plaintext.c - Load plain text into the textbox widget
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
#ifdef CONFIG_FORMAT_TEXTSAVE
#include <pgserver/divtree.h>
#include <pgserver/widget.h>
#endif

#include <ctype.h>
#include <string.h>

g_error plaintext_word(struct textbox_cursor *c, const u8 *start,
		       const u8 *end);

#ifdef CONFIG_FORMAT_TEXTSAVE
g_error plaintext_save(struct textbox_cursor *c, u8 **data, u32 *datalen)
 {
  g_error e;
  const u8 *str;
  struct divnode *line, *word;
  struct gropctxt gctx;
  int wordlen;

  *datalen=0;
  e = g_malloc((void**)data, 1);
  errorcheck;
  (*data)[0]=0;
  line=c->head;
  while(line)
   {
    word=line->div;
    while(word)
     {
      gropctxt_init(&gctx, word->div);
      if(gctx.current && gctx.current->type==PG_GROP_TEXT)
       {
	e = rdhandle((void**)&str, PG_TYPE_STRING, c->widget->owner,
	    gctx.current->param[0]);
	errorcheck;
	wordlen=strlen(str);
	e = g_realloc((void**)data, *datalen+wordlen+2);
	errorcheck;
	strcpy(*data+*datalen, str);
	*datalen+=wordlen;
	(*data)[(*datalen)++]=' ';
	(*data)[*datalen]=0;
       }
      word=word->next;
     }
    e = g_realloc((void**)data, *datalen+2);
    errorcheck;
    (*data)[(*datalen)++]='\n';
    (*data)[*datalen]=0;
    line=line->next;
   }
  return success;
 }
#endif

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

  return success;
}

g_error plaintext_word(struct textbox_cursor *c, const u8 *start,
		       const u8 *end) {
  int length;
  char *str;
  g_error e;

  /* Allocate a new string */
  length = end-start+1;
  if (!length)
    return success;
  e = g_malloc((void**)&str, length+1);
  errorcheck;
  str[length] = 0;  

  strncpy(str,start,length);
  
  /* Send it to the textbox */
  return text_insert_string(c,str,0);
}

/* The End */





