/* $Id$
 *
 * textbox_frontend.c - User and application interface for
 *                      the textbox widget. High level document handling
 *                      in implemented in textbox_document, and low level
 *                      functionality is in textbox_paragraph.
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
#include <pgserver/widget.h>
#include <pgserver/textbox.h>

void textbox_command(struct widget *self, u32 command, 
		    u32 numparams, s32 *params);

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

  int insertmode;       /* PG_INSERT_* constant, set by PG_WP_INSERTMODE */

  u32 update_time;      /* Timer to make the cursor only flash when inactive */
  u32 time_on;          /* Timers, defined in the theme */
  u32 time_off;
  u32 time_delay;

  unsigned int focus : 1;
  unsigned int flash_on : 1;
  unsigned int readonly : 1;
  unsigned int has_unreported_edit : 1;

  /* Mask of extended events to send (meaning: not handle internally)
   * and other flags*/
  int extdevents;
};
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(textboxdata)

/* Get a pgstring for the current text format */
g_error textbox_getformat(struct widget *self, struct pgstring **fmt);

/* Add text in the current text format and insertion mode */
g_error textbox_write(struct widget *self, struct pgstring *str);

/* Find keys to ignore */
int textbox_ignorekey(struct widget *self, int key);

void textbox_reset_inactivity(struct widget *self);

/********************************************** standard widget functions */

/* Set up divnodes */
g_error textbox_install(struct widget *self) {
  g_error e;

  WIDGET_ALLOC_DATA(textboxdata);

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;

  /* Global background and container for the document */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER | DIVNODE_HOTSPOT;
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_TEXTBOX;
  self->out = &self->in->next;

  /* Embedded widgets get their events dispatched completely independently, 
   * so we only have to worry about events related to paragraphs.
   */
  self->trigger_mask = PG_TRIGGER_DEACTIVATE | PG_TRIGGER_ACTIVATE |
    PG_TRIGGER_TIMER | PG_TRIGGER_DOWN | PG_TRIGGER_KEYUP |
    PG_TRIGGER_KEYDOWN | PG_TRIGGER_CHAR | PG_TRIGGER_STREAM | PG_TRIGGER_COMMAND;

  e = document_new(&DATA->doc, self->in->div);
  errorcheck;
  
  return success;
}

void textbox_remove(struct widget *self) {
  handle_free(self->owner,DATA->doc_string);
  document_delete(DATA->doc);
  g_free(DATA);
  r_divnode_free(self->in);
}

void textbox_resize(struct widget *self) {
  self->in->div->split = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
  DATA->time_on = theme_lookup(self->in->div->state,PGTH_P_TIME_ON);
  DATA->time_off = theme_lookup(self->in->div->state,PGTH_P_TIME_OFF);
  DATA->time_delay = theme_lookup(self->in->div->state,PGTH_P_TIME_DELAY);
}

g_error textbox_set(struct widget *self,int property, glob data) {
  g_error e;
  struct pgstring *str;
  int i;
  struct paragraph *p;

  switch (property) {

  case PG_WP_AUTOSCROLL:
    DATA->doc->autoscroll = data;
    paragraph_show_cursor(DATA->doc->crsr);   /* Scroll to the cursor now */
    break;

  case PG_WP_READONLY:
    DATA->readonly = data;
    paragraph_hide_cursor(DATA->doc->crsr);
    break;

  case PG_WP_EXTDEVENTS:
    DATA->extdevents = data;
    break;

  case PG_WP_PASSWORD:
    DATA->doc->password = data;
    resizewidget(self);
    break;

  case PG_WP_MULTILINE:
    DATA->doc->multiline = data;
    
    /* Single-line textboxen scroll horizontally */
    if (DATA->doc->multiline)
      self->in->div->flags &= ~DIVNODE_EXTEND_HEIGHT;
    else
      self->in->div->flags |= DIVNODE_EXTEND_HEIGHT | DIVNODE_DIVSCROLL;
    break;

  case PG_WP_TEXTFORMAT:
    DATA->textformat = data;
    break;

  case PG_WP_INSERTMODE:
    if (data < 0 || data > PG_INSERTMAX)
      return mkerror(PG_ERRT_BADPARAM,58);   /* Bad insertion mode */
    DATA->insertmode = data;
    break;

  case PG_WP_TEXT:
    e = rdhandle((void**)&str,PG_TYPE_PGSTRING,-1,data);
    errorcheck;
    e = textbox_write(self,str);
    errorcheck;
    paragraph_scroll_to_cursor(DATA->doc->crsr);
    break;

  case PG_WP_CURSOR_POSITION:
    paragraph_hide_cursor(DATA->doc->crsr);
    i = data >> 16;
    for (p=DATA->doc->par_list;i && p;p=p->next) {
      i--;
    }
    DATA->doc->crsr = &p->cursor;
    DATA->doc->crsr->iterator.offset = (data & 0xff);
    paragraph_show_cursor(DATA->doc->crsr);
    paragraph_scroll_to_cursor(DATA->doc->crsr);
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
  int i;
  struct paragraph *p;

  switch (property) {

  case PG_WP_AUTOSCROLL:
    return DATA->doc->autoscroll;

  case PG_WP_READONLY:
    return DATA->readonly;

  case PG_WP_EXTDEVENTS:
    return (glob) DATA->extdevents;

  case PG_WP_PASSWORD:
    return DATA->doc->password;

  case PG_WP_MULTILINE:
    return DATA->doc->multiline;

  case PG_WP_TEXTFORMAT:
    return DATA->textformat;

  case PG_WP_INSERTMODE:
    return DATA->insertmode;

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

  case PG_WP_CURSOR_POSITION:
    i = 0;
    for (p=DATA->doc->par_list;p;p=p->next) {
      if(p == DATA->doc->crsr->par)
	break;
      i++;
    }
    return (i<<16) + DATA->doc->crsr->iterator.offset;
  }
  return widget_base_get(self,property);
}

void textbox_trigger(struct widget *self,s32 type,union trigparam *param) {
  int seek_amount;
  struct pgstring *str; /* used for streaming */
  g_error e;
  
  if ((type != PG_TRIGGER_COMMAND) && (DATA->readonly))
    return;
  
  switch (type) {
    
    /* Accept a command from the client */
    
  case PG_TRIGGER_COMMAND:
    textbox_command(self,param->command.command,param->command.numparams,param->command.data);
    textbox_reset_inactivity(self);
    return;
    
    /* Receive streamed data from client */ 
  case PG_TRIGGER_STREAM:
    
    if (!iserror(pgstring_new(&str, PGSTR_ENCODE_UTF8, param->stream.size, param->stream.data))) {
      textbox_write(self,str);
      pgstring_delete(str);
      paragraph_scroll_to_cursor(DATA->doc->crsr);
    }
    return;

     /* Update visual appearance to reflect focus or lack of focus */
    
  case PG_TRIGGER_DEACTIVATE:
    DATA->focus = 0;
    DATA->flash_on = 0;
    paragraph_hide_cursor(DATA->doc->crsr);
    break;

  case PG_TRIGGER_ACTIVATE:
    DATA->focus = 1;
    DATA->flash_on = 0;
    post_event(PG_WE_FOCUS,self,1,0,NULL);
    /* No break; here! Get PG_TRIGGER_TIMER to set up the flash timer*/
    
  case PG_TRIGGER_TIMER:
    if (os_getticks() > DATA->update_time + DATA->time_delay) {

      if (DATA->has_unreported_edit) {
	DATA->has_unreported_edit = 0;
	post_event(PG_WE_CHANGED,self,0,0,NULL);
      }

      if (DATA->focus) {
	if (DATA->flash_on = !DATA->flash_on)
	  paragraph_show_cursor(DATA->doc->crsr);
	else
	  paragraph_hide_cursor(DATA->doc->crsr);
      }
    }

    /* Set it again... */
    install_timer(self,(DATA->flash_on ? DATA->time_on : DATA->time_off));
    break;
   
  case PG_TRIGGER_DOWN:
    if (param->mouse.chbtn != 1)
      return;
    paragraph_hide_cursor(DATA->doc->crsr);
    document_mouseseek(DATA->doc, &param->mouse);
    paragraph_show_cursor(DATA->doc->crsr);
    textbox_reset_inactivity(self);
    request_focus(self);
    paragraph_scroll_to_cursor(DATA->doc->crsr);
    break;

  case PG_TRIGGER_KEYUP:
    if (!(param->kbd.flags & PG_KF_FOCUSED))
      return;

    if (textbox_ignorekey(self,param->kbd.key))
      return;
    param->kbd.consume++;

    if (DATA->extdevents & PG_EXEV_KEY) {
      /* client asked us to not handle keyup */
      post_event(PG_WE_KBD_KEYUP,self,(param->kbd.mods<<16)|param->kbd.key,0,NULL);
      return;
    }

    textbox_reset_inactivity(self);

    switch (param->kbd.key) {
      /* don't post bound keys */
    case PGKEY_LEFT:
    case PGKEY_RIGHT:
    case PGKEY_UP:
    case PGKEY_DOWN:
    case PGKEY_HOME:
    case PGKEY_END:
      break;
    
    default:
      post_event(PG_WE_KBD_KEYUP,self,(param->kbd.mods<<16)|param->kbd.key,0,NULL);
    }
    return;   /* Skip update */

  case PG_TRIGGER_KEYDOWN:
    if (!(param->kbd.flags & PG_KF_FOCUSED))
      return;

    if (textbox_ignorekey(self,param->kbd.key))
      return;

    if (DATA->extdevents & PG_EXEV_KEY) {
      /* client asked us to not handle keydown */
      post_event(PG_WE_KBD_KEYDOWN,self,(param->kbd.mods<<16)|param->kbd.key,0,NULL);
      param->kbd.consume++;
      return;
    }

    textbox_reset_inactivity(self);
    seek_amount = 0;

    switch (param->kbd.key) {
      
    case PGKEY_LEFT:
      paragraph_hide_cursor(DATA->doc->crsr);
      if (param->kbd.mods & PGMOD_CTRL)
        seek_amount = document_bounded_seek(DATA->doc,-10,PGSEEK_CUR);
      else
        seek_amount = document_bounded_seek(DATA->doc,-1,PGSEEK_CUR);
      break;
      
    case PGKEY_RIGHT:
      paragraph_hide_cursor(DATA->doc->crsr);
      if (param->kbd.mods & PGMOD_CTRL)
        seek_amount = document_bounded_seek(DATA->doc,10,PGSEEK_CUR);
      else
        seek_amount = document_bounded_seek(DATA->doc,1,PGSEEK_CUR);
      break;
      
    case PGKEY_UP:
      paragraph_hide_cursor(DATA->doc->crsr);
      if (param->kbd.mods & PGMOD_CTRL)
        seek_amount = document_lineseek(DATA->doc,-10);
      else
        seek_amount = document_lineseek(DATA->doc,-1);
      break;
      
    case PGKEY_DOWN:
      paragraph_hide_cursor(DATA->doc->crsr);
      if (param->kbd.mods & PGMOD_CTRL)
        seek_amount = document_lineseek(DATA->doc,10);
      else
        seek_amount = document_lineseek(DATA->doc,1);
      break;
      
    case PGKEY_HOME:
      paragraph_hide_cursor(DATA->doc->crsr);
      if (param->kbd.mods & PGMOD_CTRL)
        seek_amount = document_bounded_seek(DATA->doc,0,PGSEEK_SET);
      else {
        paragraph_seekcursor(DATA->doc->crsr,0,PGSEEK_SET);
	seek_amount = 1;
      }
      break;
      
    case PGKEY_END:
      paragraph_hide_cursor(DATA->doc->crsr);
      if (param->kbd.mods & PGMOD_CTRL)
        seek_amount = document_bounded_seek(DATA->doc,0,PGSEEK_END);
      else {
	paragraph_seekcursor(DATA->doc->crsr,0,PGSEEK_END);
	seek_amount = 1;
      }
      break;
      
    default:
      /* unbound - send to client */
      post_event(PG_WE_KBD_KEYDOWN,self,(param->kbd.mods<<16)|param->kbd.key,0,NULL);
      param->kbd.consume++;
      return; /* Skip update */
    }

    /* If we didn't actually get anywhere, let the event pass */
    if (seek_amount)
      param->kbd.consume++;

    paragraph_show_cursor(DATA->doc->crsr);
    paragraph_scroll_to_cursor(DATA->doc->crsr);
    break;

  case PG_TRIGGER_CHAR:
    if (!(param->kbd.flags & PG_KF_FOCUSED))
      return;

    if (textbox_ignorekey(self,param->kbd.key)) {
      if (param->kbd.key == PGKEY_RETURN && !DATA->doc->multiline) {
	param->kbd.consume++;
	textbox_reset_inactivity(self);
	post_event(PG_WE_ACTIVATE,self,0,0,NULL);
	break;
      }
      return;
    }

    if (DATA->extdevents & PG_EXEV_CHAR) {
      /* client asked us to not handle chars */
      post_event(PG_WE_KBD_CHAR,self,param->kbd.key,0,NULL);
      param->kbd.consume++;
      return;
    }

    param->kbd.consume++;
    textbox_reset_inactivity(self);
    
    if (param->kbd.key == PGKEY_BACKSPACE)
      document_backspace_char(DATA->doc);
    else if (param->kbd.key == PGKEY_DELETE)
      document_delete_char(DATA->doc);
    else
      document_insert_char(DATA->doc, param->kbd.key, NULL);
    paragraph_wrap(DATA->doc->crsr->par,0);

    DATA->has_unreported_edit = 1;

    /* The cursor might have been hidden if we added a paragraph */
    paragraph_show_cursor(DATA->doc->crsr);
    paragraph_scroll_to_cursor(DATA->doc->crsr);

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
  if (!DATA->doc->multiline)
    switch (key) {
    case PGKEY_TAB:
    case PGKEY_UP:
    case PGKEY_DOWN:
    case PGKEY_ESCAPE:
    case PGKEY_RETURN:
      return 1;
    }
  
  return 0;
}

void textbox_reset_inactivity(struct widget *self) {
  if (!DATA->flash_on) {
    DATA->flash_on = 1;
    paragraph_show_cursor(DATA->doc->crsr);
  }
  DATA->update_time = os_getticks();
}

/* Add text in the current text format and insertion mode */
g_error textbox_write(struct widget *self, struct pgstring *str) {
  struct pgstring *fmt;
  g_error e;

  switch (DATA->insertmode) {
    
  case PG_INSERT_OVERWRITE:
    document_nuke(DATA->doc);
    break;
    
  case PG_INSERT_ATCURSOR:
    break;
    
  case PG_INSERT_PREPEND:
    document_seek(DATA->doc, 0, PGSEEK_SET);
    break;
    
  case PG_INSERT_APPEND:
    document_seek(DATA->doc, 0, PGSEEK_END);
    break;
    
  }
  
  e = textbox_getformat(self,&fmt);
  errorcheck;
  
  e = document_load(DATA->doc, fmt, str);
  errorcheck;
  
  paragraph_wrap(DATA->doc->crsr->par,0);
  return success;
}

/*********************************** Commands */
   
void textbox_command(struct widget *self, u32 command, 
		     u32 numparams, s32 *params) {
   int i;
   
   switch (command) {

    case 1:
      document_nuke(DATA->doc);
      break;
      
   }
}

/* The End */
