/* $Id: textbox_main.c,v 1.21 2001/12/14 22:56:45 micahjd Exp $
 *
 * textbox_main.c - works along with the rendering engine to provide advanced
 * text display and editing capabilities. This file handles the usual widget
 * behaviors, and user interaction.
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
#include <pgserver/widget.h>
#include <pgserver/textbox.h>

#define FLASHTIME_ON   250
#define FLASHTIME_OFF  150

struct textboxdata {
  int on,focus,flash_on;
  struct textbox_cursor c;
  handle textformat;
};
#define DATA ((struct textboxdata *)(self->data))

/* Move the cursor to the mouse location */
void textbox_move_cursor(struct widget *self, union trigparam *param);

/* Set up divnodes */
g_error textbox_install(struct widget *self) {
   g_error e;

   e = g_malloc(&self->data,           /* Allocate data structure */
		sizeof(struct textboxdata));
   errorcheck;
   memset(self->data,0,sizeof(struct textboxdata));
   e = newdiv(&self->in,self);         /* Main split */
   errorcheck;
   self->in->flags |= PG_S_TOP;
   self->out = &self->in->next;
   e = newdiv(&self->in->div,self);    /* Background fill
					* We could just put the 1st line here
					* but that would break scrolling and
					* other code that depends on the whole
					* widget being contained within
					* in->div. */
   errorcheck;
   self->in->div->build = &build_bgfill_only;
   self->in->div->state = PGTH_O_TEXTBOX;
   self->in->div->flags |= DIVNODE_SPLIT_BORDER;
   self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
   e = newdiv(&self->in->div->div,self);  /* 1st line */
   errorcheck;
   self->in->div->div->flags |= PG_S_TOP;

   /* This sets up the cursor and sets the default font */
   DATA->c.widget = self;
   e = text_nuke(&DATA->c);
   errorcheck;

   /**** Editing doesn't work yet 
   self->trigger_mask = TRIGGER_UP | TRIGGER_ACTIVATE | TRIGGER_CHAR |
     TRIGGER_DEACTIVATE | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_TIMER
     | TRIGGER_MOVE;
   */

   return success;
}

void textbox_remove(struct widget *self) {
  /* Delete our formatting stacks and associated fonts */
  textbox_delete_formatstack(self, DATA->c.f_used);
  textbox_delete_formatstack(self, DATA->c.f_top);

  g_free(self->data);
  if (!in_shutdown)
     r_divnode_free(self->in);
}

void textbox_resize(struct widget *self) {
  /* Set our margin using the theme */
  self->in->div->split = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
}

g_error textbox_set(struct widget *self,int property, glob data) {
  char *str, *fmt;
  g_error e;

  switch (property) {
    
    /* Store a string describing the text format */
  case PG_WP_TEXTFORMAT:
    if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,data))) 
      return mkerror(PG_ERRT_HANDLE,61); /* bad textformat handle */
    DATA->textformat = (handle) data;
    break;

    /* Load text in the current format */
  case PG_WP_TEXT:
    /* Load handles */
    if (DATA->textformat) {
      if (iserror(rdhandle((void **)&fmt,PG_TYPE_STRING,-1,DATA->textformat))) 
	return mkerror(PG_ERRT_HANDLE,61); /* bad textformat handle */
    }    
    else
      fmt = "TEXT";   /* Default to plaintext */
    if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,data))) 
      return mkerror(PG_ERRT_HANDLE,13); /* bad text handle */

    /* Using the '+' modifier? */
    if (*fmt == '+') {
      /* Lose the + */
      fmt++;
    }
    else {
      /* Delete everything */
      e = text_nuke(&DATA->c);
      errorcheck;
    }
   
    return text_load(&DATA->c,fmt,str,strlen(str));
    break;
    
  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob textbox_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_TEXTFORMAT:
    return (glob) DATA->textformat;

  }
  return 0;
}

/* Move the cursor to the mouse location */
void textbox_move_cursor(struct widget *self, union trigparam *param) {
  DATA->c.c_div = deepest_div_under_crsr;
  DATA->c.c_gx = param->mouse.x - deepest_div_under_crsr->x;
  DATA->c.c_gy = 0;
  DATA->c.c_gctx.current = NULL;
  text_caret_on(&DATA->c);
}

void textbox_trigger(struct widget *self,long type,union trigparam *param) {
  switch (type) {

    /* When clicked, request keyboard focus */
    
  case TRIGGER_DOWN:
    if (param->mouse.chbtn!=1)
      break;
    DATA->on=1;
    if (!DATA->focus)
      request_focus(self);
    textbox_move_cursor(self,param);
    break;

  case TRIGGER_MOVE:
    /* Drag the cursor */
    if (DATA->on)
      textbox_move_cursor(self,param);
    break;

  case TRIGGER_UP:
  case TRIGGER_RELEASE:
    if (param->mouse.chbtn==1)
      DATA->on=0;
    return;
    
    /* Update visual appearance to reflect focus or lack of focus */
    
  case TRIGGER_DEACTIVATE:
    DATA->focus = 0;
    DATA->flash_on = 0;
    text_caret_off(&DATA->c);
    break;

  case TRIGGER_ACTIVATE:
    DATA->focus = 1;
    /* No break; here! Get TRIGGER_TIMER to set up the flash timer*/

  case TRIGGER_TIMER:
    if (DATA->focus==0) break;

    if (DATA->flash_on = !DATA->flash_on)
      text_caret_on(&DATA->c);
    else
      text_caret_off(&DATA->c);

    /* Set it again... */
    install_timer(self,(DATA->flash_on ? 
			FLASHTIME_ON : FLASHTIME_OFF ));
    break;

  case TRIGGER_CHAR:    /* Keyboard input */
    switch (param->kbd.key) {

    case PGKEY_RETURN:
      text_insert_linebreak(&DATA->c);
      break;
      
    default:
      {
	char *str;
	if (iserror(g_malloc((void**)&str,2)))
	  break;
	str[0] = param->kbd.key;
	str[1] = 0;
	text_insert_string(&DATA->c,str,0);
	if (param->kbd.key == PGKEY_SPACE)
	  text_insert_wordbreak(&DATA->c);
      }
      break;
    }
    break;    

  }
}
   
/* The End */
