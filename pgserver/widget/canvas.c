/* $Id: canvas.c,v 1.5 2001/01/24 00:34:11 micahjd Exp $
 *
 * canvas.c - canvas widget, allowing clients to manipulate the groplist
 * and recieve events directly, implementing graphical output or custom widgets
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/widget.h>
#include <picogui/canvas.h>

void canvas_command(struct widget *self, unsigned short command, 
		    unsigned short numparams,signed long *params);

/* Use the data pointer to store a grop context */
#define CTX  ((struct gropctxt *)self->data)

/*********************************** Widget interfacing */

void build_canvas(struct gropctxt *c,
		  unsigned short state,struct widget *self) {
   /* Just pass this on to the app */
   post_event(PG_WE_BUILD,self,
	      (self->in->div->w << 16) | self->in->div->h,0,NULL);
}

/* Set up divnodes */
g_error canvas_install(struct widget *self) {
   g_error e;
   
   /* Allocate context */
   e = g_malloc((void**) &self->data,sizeof(struct gropctxt));
   errorcheck;
   
   /* Main split */
   e = newdiv(&self->in,self);
   errorcheck;
   self->in->flags |= PG_S_ALL;
   self->out = &self->in->next;
   
   /* Visible node */
   e = newdiv(&self->in->div,self);
   errorcheck;
   self->in->div->build = &build_canvas;

   /* Init grop context */
   gropctxt_init(CTX,self->in->div);
   self->rawbuild = 1;
   
   self->trigger_mask = TRIGGER_STREAM | TRIGGER_UP | TRIGGER_DOWN;
   
   return sucess;
}

void canvas_remove(struct widget *self) {
   g_free(self->data);
   
   if (!in_shutdown)
     r_divnode_free(self->in);
}

g_error canvas_set(struct widget *self,int property, glob data) {

  switch (property) {

  case PG_WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,31);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    redraw_bg(self);
    break;

   default:
    return mkerror(PG_ERRT_BADPARAM,37);
  }
  return sucess;
}

glob canvas_get(struct widget *self,int property) {
   return 0;
}

void canvas_trigger(struct widget *self,long type,union trigparam *param) {
   int evt;
   
   if (type == TRIGGER_STREAM) {
      /* Accept a command from the client */
      
      struct pgcommand *cmd;
      char *buffer = param->stream.data;
      unsigned long remaining = param->stream.size;
      int i;
      signed long *params;
      
      while (remaining) {
	 
	 /* Out of space? */
	 if (remaining < sizeof(struct pgcommand))
	   return;
	 cmd = (struct pgcommand *) buffer;
	 cmd->command = ntohs(cmd->command);
	 cmd->numparams = ntohs(cmd->numparams);
	 
	 params = (signed long *) (buffer + sizeof(struct pgcommand));
	 
	 buffer += sizeof(struct pgcommand) + 
	   cmd->numparams * sizeof(signed long);
	 remaining -= sizeof(struct pgcommand) + 
	   cmd->numparams * sizeof(signed long);
	 if (remaining < 0)
	   return;
	 
	 /* Convert parameters */
	 for (i=0;i<cmd->numparams;i++)
	   params[i] = ntohl(params[i]);
	 
	 canvas_command(self,cmd->command,cmd->numparams,params);
	 
      }
      return;
   }

   /* Nope, it was some sort of event to pass on to the app */
   
   switch (type) {
    case TRIGGER_UP:
      evt = PG_WE_PNTR_UP;
      break;
    case TRIGGER_DOWN:
      evt = PG_WE_PNTR_DOWN;
      break;
   }

   /* Same mouse event packing used for pointer grabbing in widget.c:  
    *
    * Squeeze all the mouse params into a long, as follows.
    * Note that if PicoGUI is to support screens bigger than
    * 4096x4096 this won't work!
    * 
    * Bits 31-28:  buttons
    * Bits 27-24:  changed buttons
    * Bits 23-12:  Y
    * Bits 11-0 :  X
    */
      
   post_event(evt,self,
	      (param->mouse.btn << 28) |
	      (param->mouse.chbtn << 24) |
	      ((param->mouse.y-self->in->div->y) << 12) |
	      param->mouse.x - self->in->div->x,
	      0,NULL);
}

/*********************************** Commands */
   
void canvas_command(struct widget *self, unsigned short command, 
		    unsigned short numparams,signed long *params) {
   int i;
   switch (command) {

    case PGCANVAS_NUKE:
      grop_free(&self->in->div->grop);
      gropctxt_init(CTX,self->in->div);
      break;
      
    case PGCANVAS_GROP:
      if (numparams<5) return;
      addgrop(CTX,params[0],params[1],params[2],params[3],params[4]);
      break;
      
    case PGCANVAS_EXECFILL:
      if (numparams<6) return;
      /* Fudge the coordinates */
      CTX->x = params[2];
      CTX->y = params[3];
      CTX->w = params[4];
      CTX->h = params[5];
      exec_fillstyle(CTX,params[0],params[1]);
      break;
      
    case PGCANVAS_FINDGROP:
      if (numparams<1) return;
      CTX->current = self->in->div->grop;
      for (i=params[0];i && CTX->current;i--)
	CTX->current = CTX->current->next;
      break;
      
    case PGCANVAS_SETGROP:
      if (!CTX->current) return;
      if (numparams>NUMGROPPARAMS) numparams = NUMGROPPARAMS;
      for (i=0;i<numparams;i++)
        CTX->current->param[i] = params[i];
      break;
      
    case PGCANVAS_MOVEGROP:
      if (numparams<4 || !CTX->current) return;
      CTX->current->x = params[0];
      CTX->current->y = params[1];
      CTX->current->w = params[2];
      CTX->current->h = params[3];
      break;
      
    case PGCANVAS_MUTATEGROP:
      
    case PGCANVAS_COLORCONV:
      if (numparams<1 || !CTX->current) return;
      for (i=0;i<NUMGROPPARAMS;i++,params[0]>>1)
	if (params[0] & 1)
	  CTX->current->param[i] = (*vid->color_pgtohwr)(CTX->current->param[i]);
      break;
      
    case PGCANVAS_GROPFLAGS:
      if (numparams<1 || !CTX->current) return;
      CTX->current->flags = params[0];
      break;
      
    case PGCANVAS_REDRAW:
      self->in->div->flags |= DIVNODE_NEED_REDRAW;
      self->dt->flags |= DIVTREE_NEED_REDRAW;
      break;
      
    case PGCANVAS_INCREMENTAL:
      self->in->div->flags |= DIVNODE_INCREMENTAL;
      break;
      
    case PGCANVAS_SCROLL:
      if (numparams<2) return;
      self->in->div->tx = params[0];
      self->in->div->ty = params[1];
      self->in->div->flags = DIVNODE_SCROLL_ONLY;
      break;
      
   }
}
   
/* The End */
