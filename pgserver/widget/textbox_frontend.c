/* $Id: textbox_frontend.c,v 1.8 2002/10/07 15:30:36 micahjd Exp $
 *
 * textbox_frontend.c - User and application interface for
 *                      the textbox widget. High level document handling
 *                      in implemented in textbox_document, and low level
 *                      functionality is in textbox_paragraph.
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
#include <pgserver/textbox.h>

struct textboxdata {
  struct textbox_document *doc;

  handle doc_string;    /* We have to create a new string to hold the document
			 * when PG_WP_TEXT property is get'ed. The client should probably
			 * delete this itself, but if it doesn't we'll delete it next
			 * time PG_WP_TEXT is called to keep ignorant clients from leaking
			 */
  
  handle textformat;    /* String set by PG_WP_TEXTFORMAT, this is the format
			 * loader that will be used to convert the document 
			 * to a pgstring and/or back. If this is 0, the default
			 * text format (plaintext) will be used.
			 */

  unsigned int focus : 1;
  unsigned int flash_on : 1;
  unsigned int multiline : 1;
};
#define DATA WIDGET_DATA(0,textboxdata)

#define FLASHTIME_ON   250
#define FLASHTIME_OFF  150

/* Get a pgstring for the current text format */
g_error textbox_getformat(struct widget *self, struct pgstring **fmt);

/* Find keys to ignore */
int textbox_ignorekey(struct widget *self, int key);

/********************************************** standard widget functions */

/* Set up divnodes */
g_error textbox_install(struct widget *self) {
  g_error e;

  WIDGET_ALLOC_DATA(0,textboxdata)
  DATA->multiline = 1;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;

  /* Global background and container for the document */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_TEXTBOX;
  self->out = &self->in->next;

  /* Embedded widgets get their events dispatched completely independently, 
   * so we only have to worry about events related to paragraphs.
   */
  self->trigger_mask = PG_TRIGGER_DEACTIVATE | PG_TRIGGER_ACTIVATE |
    PG_TRIGGER_TIMER | PG_TRIGGER_DOWN | PG_TRIGGER_KEYUP |
    PG_TRIGGER_KEYDOWN | PG_TRIGGER_CHAR;

  e = document_new(&DATA->doc, self->in->div);
  errorcheck;
  

  return success;
}

void textbox_remove(struct widget *self) {
  document_delete(DATA->doc);
  g_free(DATA);
  r_divnode_free(self->in);
}

void textbox_resize(struct widget *self) {
}

g_error textbox_set(struct widget *self,int property, glob data) {
  g_error e;
  struct pgstring *str;  
  struct pgstring *fmt;

  switch (property) {

  case PG_WP_MULTILINE:
    DATA->multiline = data;

  case PG_WP_TEXTFORMAT:
    DATA->textformat = data;
    break;

  case PG_WP_TEXT:
    e = textbox_getformat(self,&fmt);
    errorcheck;
    e = rdhandle((void**)&str,PG_TYPE_PGSTRING,-1,data);
    errorcheck;
    e = document_load(DATA->doc, fmt, str);
    errorcheck;
    break;

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob textbox_get(struct widget *self,int property) {
  g_error e;
  struct pgstring *str;  
  struct pgstring *fmt;

  switch (property) {

  case PG_WP_MULTILINE:
    return DATA->multiline;

  case PG_WP_TEXTFORMAT:
    return DATA->textformat;

  case PG_WP_TEXT:
    e = textbox_getformat(self,&fmt);
    errorcheck;
    e = document_save(DATA->doc, fmt, &str);
    errorcheck;

    /* Assuming nothing failed yet, we have a new string in str and maybe
     * an old string handle in DATA->textformat. Clean up the old one and
     * make a handle for the new string.
     */
    e = handle_free(self->owner,DATA->doc_string);
    errorcheck;
    e = mkhandle(&DATA->doc_string,PG_TYPE_PGSTRING, self->owner,str);
    errorcheck;

    return DATA->doc_string;

  }
  return widget_base_get(self,property);
}

void textbox_trigger(struct widget *self,s32 type,union trigparam *param) {
  struct paragraph *par;

  switch (type) {
    
     /* Update visual appearance to reflect focus or lack of focus */
    
  case PG_TRIGGER_DEACTIVATE:
    DATA->focus = 0;
    DATA->flash_on = 0;
    break;

  case PG_TRIGGER_ACTIVATE:
    DATA->focus = 1;
    DATA->flash_on = 0;
    post_event(PG_WE_FOCUS,self,1,0,NULL);
    /* No break; here! Get PG_TRIGGER_TIMER to set up the flash timer*/
    
  case PG_TRIGGER_TIMER:
    if (DATA->focus==0) break;

    if (DATA->flash_on = !DATA->flash_on)
      paragraph_show_cursor(DATA->doc->crsr);
    else
      paragraph_hide_cursor(DATA->doc->crsr);

    /* Set it again... */
    install_timer(self,(DATA->flash_on ? 
			FLASHTIME_ON : FLASHTIME_OFF ));
    break;
   
  case PG_TRIGGER_DOWN:
    if (param->mouse.chbtn != 1)
      return;
    par = document_get_div_par(param->mouse.cursor->ctx.div_under);
    if (par) {
      paragraph_hide_cursor(DATA->doc->crsr);
      DATA->doc->crsr = &par->cursor;
      paragraph_movecursor(DATA->doc->crsr, par,
			   param->mouse.x - par->div->div->x,
			   param->mouse.y - par->div->div->y);
      paragraph_show_cursor(DATA->doc->crsr);
      request_focus(self);
    }
#ifdef DEBUG_TEXTBOX
    else
      guru("Clicked on no paragraph");
#endif
    break;

  case PG_TRIGGER_KEYUP:
    if (textbox_ignorekey(self,param->kbd.key))
      return;

    if (param->kbd.flags & PG_KF_FOCUSED)
      param->kbd.consume++;
    return;   /* Skip update */

  case PG_TRIGGER_KEYDOWN:
    if (textbox_ignorekey(self,param->kbd.key))
      return;

    if (param->kbd.flags & PG_KF_FOCUSED) {
      param->kbd.consume++;
      switch (param->kbd.key) {

      case PGKEY_LEFT:
	paragraph_hide_cursor(DATA->doc->crsr);
	document_seek(DATA->doc,-1,PGSEEK_CUR);
	break;

      case PGKEY_RIGHT:
	paragraph_hide_cursor(DATA->doc->crsr);
	document_seek(DATA->doc,1,PGSEEK_CUR);
	break;

      case PGKEY_UP:
	paragraph_hide_cursor(DATA->doc->crsr);
	document_lineseek(DATA->doc,-1);
	break;
	
      case PGKEY_DOWN:
	paragraph_hide_cursor(DATA->doc->crsr);
	document_lineseek(DATA->doc,1);
	break;

      default:
	return; /* Skip update */
      }
      paragraph_show_cursor(DATA->doc->crsr);
    }
    else
      return;   /* Skip update */
    break;

  case PG_TRIGGER_CHAR:
    if (textbox_ignorekey(self,param->kbd.key))
      return;

    if (param->kbd.flags & PG_KF_FOCUSED) {
      param->kbd.consume++;
      if (param->kbd.key == PGKEY_BACKSPACE) {
	paragraph_seekcursor(DATA->doc->crsr, -1);
	if (!DATA->doc->crsr->iterator.invalid)
	  paragraph_delete_char(DATA->doc->crsr);
      }
      else if (param->kbd.key == PGKEY_DELETE) {
	document_delete_char(DATA->doc);
      }
      else if (param->kbd.key == PGKEY_RETURN && !DATA->multiline) {
	post_event(PG_WE_ACTIVATE,self,0,0,NULL);
      }
      else {
	document_insert_char(DATA->doc, param->kbd.key, NULL);
      }
      paragraph_wrap(DATA->doc->crsr->par,0);
    }
    break;

  }

  update(NULL,1);
}

/********************************************** internal functions */

/* Get a pgstring for the current text format */
g_error textbox_getformat(struct widget *self, struct pgstring **fmt) {
  static handle default_format = 0;
  handle h = DATA->textformat;
  g_error e;
  
  /* Need to use the default format? */
  if (!h) {
    /* Need to allocate a string for the default format?
     * This only happens once, when the default format is used for the
     * first time. It is a pgserver-owned handle, cleaned up at exit.
     */
    if (!default_format) {
      e = pgstring_wrap(&default_format, PGSTR_ENCODE_ASCII, "plaintext");
      errorcheck;
    }
    h = default_format;
  }

  if (iserror(rdhandle((void**)fmt,PG_TYPE_PGSTRING,-1,h)))
    return mkerror(PG_ERRT_BADPARAM, 61);  /* Bad textformat handle */
  return success;
}

/* Find keys to ignore */
int textbox_ignorekey(struct widget *self, int key) {
  /* Ignore some keys in single-line mode: */
  if (!DATA->multiline)
    switch (key) {
    case PGKEY_TAB:
    case PGKEY_UP:
    case PGKEY_DOWN:
    case PGKEY_ESCAPE:
      return 1;
    }

  return 0;
}

/* The End */
