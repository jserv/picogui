/* $Id: textbox_main.c,v 1.2 2001/10/02 08:47:50 micahjd Exp $
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

/* Set up divnodes */
g_error textbox_install(struct widget *self) {
   g_error e;
   struct textbox_cursor crsr;

   self->rawbuild = 1;                /* Disable normal grop rebuild */
   e = newdiv(&self->in,self);        /* Main split */
   errorcheck;
   self->in->flags |= PG_S_TOP;
   self->out = &self->in->next;
   e = newdiv(&self->in->div,self);   /* 1st line */
   errorcheck;
   self->in->div->flags |= PG_S_TOP;
   memset(&crsr,0,sizeof(crsr));      /* Set up cursor */
   crsr.head = self->in->div;
   crsr.widget = self;

   /* Add some demo text */
   
   text_format_font_flags(&crsr,PG_FSTYLE_FLUSH,0);
   text_insert_string(&crsr,"Hello ");
   text_insert_wordbreak(&crsr);
   text_insert_string(&crsr,"World! ");
   text_insert_linebreak(&crsr);
   text_insert_string(&crsr,"This ");
   text_insert_wordbreak(&crsr);
   text_format_font_flags(&crsr,PG_FSTYLE_BOLD,0);
   text_insert_string(&crsr,"is ");
   text_format_font_flags(&crsr,PG_FSTYLE_ITALIC,PG_FSTYLE_BOLD);
   text_insert_wordbreak(&crsr);
   text_insert_string(&crsr,"nifty. ");

   return sucess;
}

void textbox_remove(struct widget *self) {
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
}

   
/* The End */
