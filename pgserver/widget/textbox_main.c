/* $Id: textbox_main.c,v 1.5 2001/10/06 09:02:41 micahjd Exp $
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
  int focus,on,flash_on;
  struct textbox_cursor c;
};
#define DATA ((struct textboxdata *)(self->data))

/* Set up divnodes */
g_error textbox_install(struct widget *self) {
   g_error e;

   e = g_malloc(&self->data,           /* Allocate data structure */
		sizeof(struct textboxdata));
   errorcheck;
   memset(self->data,0,sizeof(struct textboxdata));
   self->rawbuild = 1;                 /* Disable normal grop rebuild */
   e = newdiv(&self->in,self);         /* Main split */
   errorcheck;
   self->in->flags |= PG_S_TOP;
   self->out = &self->in->next;
   e = newdiv(&self->in->div,self);    /* 1st line */
   errorcheck;
   self->in->div->flags |= PG_S_TOP;
   memset(&DATA->c,0,sizeof(DATA->c)); /* Set up cursor */
   DATA->c.head = self->in->div;
   DATA->c.widget = self;
   self->trigger_mask = TRIGGER_UP | TRIGGER_ACTIVATE | TRIGGER_CHAR |
     TRIGGER_DEACTIVATE | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_TIMER;

   /* Add some demo text */
   
   text_format_modifyfont(&DATA->c,PG_FSTYLE_FLUSH,0,0);
   text_insert_string(&DATA->c,"Hello ");
   text_insert_wordbreak(&DATA->c);
   text_format_color(&DATA->c,0xFFFF00);
   text_insert_string(&DATA->c,"World! ");
   text_insert_linebreak(&DATA->c);
   text_insert_string(&DATA->c,"This ");
   text_insert_wordbreak(&DATA->c);
   text_format_modifyfont(&DATA->c,PG_FSTYLE_BOLD,0,10);
   text_format_color(&DATA->c,0xFF0000);
   text_insert_string(&DATA->c,"is ");
   text_format_modifyfont(&DATA->c,PG_FSTYLE_ITALIC,PG_FSTYLE_BOLD,-20);
   text_insert_wordbreak(&DATA->c);
   text_insert_string(&DATA->c,"nifty. ");

   return sucess;
}

void textbox_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
     r_divnode_free(self->in);
}

void textbox_resize(struct widget *self) {
}

g_error textbox_set(struct widget *self,int property, glob data) {
   return mkerror(ERRT_PASS,0);
}

glob textbox_get(struct widget *self,int property) {
   return 0;
}

void textbox_trigger(struct widget *self,long type,union trigparam *param) {
  switch (type) {

    /* When clicked, request keyboard focus */
    
  case TRIGGER_DOWN:
    if (param->mouse.chbtn==1)
      DATA->on=1;
    return;

  case TRIGGER_UP:
    if (DATA->on && param->mouse.chbtn==1) {
      DATA->on=0;
      request_focus(self);
    }
    return;
    
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
    break;
    
  }
}
   
/* The End */
