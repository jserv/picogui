/* $Id: canvas.c,v 1.42 2002/05/22 09:26:33 micahjd Exp $
 *
 * canvas.c - canvas widget, allowing clients to manipulate the groplist
 * and recieve events directly, implementing graphical output or custom widgets
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

#include <string.h>
#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <picogui/canvas.h>

void canvas_command(struct widget *self, u16 command, 
		    u16 numparams, s32 *params);

struct canvasdata {
  struct gropctxt ctx;
  struct rect input_map;
  u8 input_maptype;
  handle lastfont;
  s16 gridw,gridh;
};
   
#define DATA ((struct canvasdata *)self->data)
#define CTX  (&DATA->ctx)

/*********************************** Utility */

/* This does the reverse of mappings specified in render.c */
void canvas_inputmap(struct widget *self,s16 *x,s16 *y) {
   switch (DATA->input_maptype) {
    case PG_MAP_NONE:
      return;
      
      /* self->in->div->w and self->in->div->h should never be zero here.
       * if they are, it's a bug in the input dispatch.
       */
    case PG_MAP_SCALE:
      *x = *x * DATA->input_map.w / self->in->div->w;
      *y = *y * DATA->input_map.h / self->in->div->h;
      break;

   case PG_MAP_SQUARESCALE:
    if (self->in->div->w * DATA->input_map.h - self->in->div->h * DATA->input_map.w > 0) {
      /* Centered horizontally */

      *x -= (self->in->div->w - DATA->input_map.w * self->in->div->h / DATA->input_map.h) >> 1;

      *x = *x * DATA->input_map.h / self->in->div->h;
      *y = *y * DATA->input_map.h / self->in->div->h;
    }
    else {
      /* Centered vertically */
     
      *y -= (self->in->div->h - DATA->input_map.h * self->in->div->w / DATA->input_map.w) >> 1;

      *x = *x * DATA->input_map.w / self->in->div->w;
      *y = *y * DATA->input_map.w / self->in->div->w;
    }
   }
}

/*********************************** Widget interfacing */

void build_canvas(struct gropctxt *c,
		  u16 state,struct widget *self) {
   /* Just pass this on to the app */
   post_event(PG_WE_BUILD,self,
	      (self->in->div->w << 16) | self->in->div->h,0,NULL);
}

/* Set up divnodes */
g_error canvas_install(struct widget *self) {
   g_error e;
   
   /* Allocate context */
   e = g_malloc((void**) &self->data,sizeof(struct canvasdata));
   errorcheck;
   memset(self->data,0,sizeof(struct canvasdata));

   DATA->gridw = DATA->gridh = 1;
   
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

   /* By default accept only stream commands and mouse clicks. The app can use
    * PG_WP_TRIGGERMASK to turn on mouse movement and keyboard events
    */
   self->trigger_mask = TRIGGER_STREAM | TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE;
   
   return success;
}

void canvas_remove(struct widget *self) {
   g_free(self->data);
   r_divnode_free(self->in);
}

g_error canvas_set(struct widget *self,int property, glob data) {
   return mkerror(ERRT_PASS,0);
}

glob canvas_get(struct widget *self,int property) {
   return 0;
}

void canvas_trigger(struct widget *self, s32 type, union trigparam *param) {
   int evt;
   s16 mx,my;
   
   if (type == TRIGGER_STREAM) {
      /* Accept a command from the client */
      
      struct pgcommand *cmd;
      char *buffer = param->stream.data;
      u32 remaining = param->stream.size;
      int i;
      s32 *params;
      
      while (remaining) {
	 
	 /* Out of space? */
	 if (remaining < sizeof(struct pgcommand))
	   return;
	 cmd = (struct pgcommand *) buffer;
	 cmd->command = ntohs(cmd->command);
	 cmd->numparams = ntohs(cmd->numparams);
	 
	 params = (s32 *) (buffer + sizeof(struct pgcommand));
	 
	 buffer += sizeof(struct pgcommand) + 
	   cmd->numparams * sizeof(s32);
	 remaining -= sizeof(struct pgcommand) + 
	   cmd->numparams * sizeof(s32);
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
    case TRIGGER_RELEASE:
      evt = PG_WE_PNTR_RELEASE;
      break;
    case TRIGGER_MOVE:
      evt = PG_WE_PNTR_MOVE;
      break;
    default:
      evt = 0;
   }

   /* Mouse event */
   if (evt) {

     /* Make coordinates relative to divnode */
     mx = param->mouse.x - self->in->div->x;
     my = param->mouse.y - self->in->div->y;
     
     /* Apply mapping */
     canvas_inputmap(self,&mx,&my);
     
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
		((my&0xFFF) << 12) |
		(mx & 0xFFF),
		0,NULL);

     return;
   }

   /* Keyboard event? */
   switch (type) {
    case TRIGGER_KEYUP:
      if (!(param->kbd.flags & PG_KF_FOCUSED) || param->kbd.key==PGKEY_ALPHA)
	return;
      evt = PG_WE_KBD_KEYUP;
      param->kbd.consume++;
      break;
    case TRIGGER_KEYDOWN:
      if (!(param->kbd.flags & PG_KF_FOCUSED) || param->kbd.key==PGKEY_ALPHA)
	return;
      evt = PG_WE_KBD_KEYDOWN;
      param->kbd.consume++;
      break;
    case TRIGGER_CHAR:
      if (!(param->kbd.flags & PG_KF_FOCUSED) || param->kbd.key==PGKEY_ALPHA)
	return;
      evt = PG_WE_KBD_CHAR;
      param->kbd.consume++;
      break;
   default:
      evt = 0;
   }      

   if (evt)
     {
       post_event(evt,self,
		  (evt==PG_WE_KBD_CHAR) ? param->kbd.key : 
		  (param->kbd.mods<<16)|param->kbd.key,
		  0,NULL);

       return;
     }

   /* Focus event? */
   switch (type) 
     {
     case TRIGGER_ACTIVATE:
       evt = PG_WE_ACTIVATE;
       break;
     case TRIGGER_DEACTIVATE:
       evt = PG_WE_DEACTIVATE;
       break;
     default:
       evt = 0;
     }
   
   if (evt)
     {
       post_event (evt, self, 0, 0, NULL);
     }

}

/* Extend the canvas's preferred size to include the gropnode specified */
void canvas_extendbox(struct widget *self, struct gropnode *n) {
  int i;

  if (PG_GROP_IS_NONVISUAL(n->type) || 
      PG_GROP_IS_UNPOSITIONED(n->type))
    return;

  i = n->r.x * DATA->gridw;
  if (i > self->in->div->pw) {
    self->in->div->pw = i;
    self->dt->flags |= DIVTREE_NEED_RESIZE;
  }
  i += n->r.w;
  if (i > self->in->div->pw) {
    self->in->div->pw = i;
    self->dt->flags |= DIVTREE_NEED_RESIZE;
  }
  i = n->r.y * DATA->gridh;
  if (i > self->in->div->ph) {
    self->in->div->ph = i;
    self->dt->flags |= DIVTREE_NEED_RESIZE;
  }
  i += n->r.h;
  if (i > self->in->div->ph) {
    self->in->div->ph = i;
    self->dt->flags |= DIVTREE_NEED_RESIZE;
  }
}

void canvas_resize(struct widget *self) {
   /* Calculate the preferred size based on the gropnodes in the canvas */
   
   struct gropnode *n;
   
   n = self->in->div->grop;

   self->in->div->pw = 0;
   self->in->div->ph = 0;

   while (n) {
     canvas_extendbox(self,n);
     n = n->next;
   }
}

/*********************************** Commands */
   
void canvas_command(struct widget *self, u16 command, 
		    u16 numparams, s32 *params) {
   int i;

   /* Must we fix the gropctxt's pointers? */
   if (self->in->div->flags & DIVNODE_GROPLIST_DIRTY) {
      CTX->current = self->in->div->grop;
      for (i=CTX->n-1;i && CTX->current && CTX->current->next;i--)
	CTX->current = CTX->current->next;
      CTX->n -= i;
      self->in->div->flags &= ~DIVNODE_GROPLIST_DIRTY;
   }
   
   switch (command) {

    case PGCANVAS_NUKE:
      grop_free(&self->in->div->grop);
      gropctxt_init(CTX,self->in->div);
      break;
      
    case PGCANVAS_GROP:
      if (numparams<1) return;
      if (PG_GROP_IS_UNPOSITIONED(params[0])) {
	 if (numparams>(NUMGROPPARAMS+1)) numparams = NUMGROPPARAMS+1;

	 if (params[0] == PG_GROP_SETFONT && !params[1])
	   params[1] = res[PGRES_DEFAULT_FONT];

	 addgrop(CTX,params[0]);
	 for (i=1;i<numparams;i++)
	   CTX->current->param[i-1] = params[i];

	 /* Store the current font for use in determining the size
	  * of text gropnodes.
	  *
	  * FIXME: If grops are not entered in order, this won't work.
	  *        But, considering it's not possible to enter gropnodes
	  *        out of order yet, it should be fine for now.
	  *
	  * FIXME: This also doesn't work for angled text... go figure :)
	  */
	 if (params[0] == PG_GROP_SETFONT)
	   DATA->lastfont = params[1];
      }
      else {
	 if (numparams<5) return;
	 if (numparams>(NUMGROPPARAMS+5)) numparams = NUMGROPPARAMS+5;
	 if(params[0]==PG_GROP_FPOLYGON) {
	   s32* arr;
	   s16 i,hix,lox,hiy,loy;
	   if (iserror(rdhandle((void**)&arr,PG_TYPE_ARRAY,-1,
			     params[5])) || !arr) break;
	   i=0;
	   hix=arr[1];
	   lox=arr[1];
	   hiy=arr[2];
	   loy=arr[2];
	   for(i=1;i<=arr[0];i+=2) {
	     if(arr[i]<lox)
	       lox=arr[i];
	     else if(arr[i]>hix)
	       hix=arr[i];
	     if(arr[i+1]<loy)
	       loy=arr[i+1];
	     else if(arr[i+1]>hiy)
	       hiy=arr[i+1];
	   }
	   params[1]=lox;
	   params[2]=loy;
	   params[3]=hix-lox;
	   params[4]=hiy-loy;
	 }
	 addgropsz(CTX,params[0],params[1],params[2],params[3],params[4]);
	 for (i=5;i<numparams;i++)
	   CTX->current->param[i-5] = params[i];

	 /* Determine the _real_ size of a text gropnode.
	  * Has a couple issues... see above FIXMEs 
	  */
	 if (params[0] == PG_GROP_TEXT) {
	   struct fontdesc *fd = NULL;
	   char *str = NULL;
	   rdhandle((void **) &fd,PG_TYPE_FONTDESC,-1,DATA->lastfont);
	   rdhandle((void **) &str,PG_TYPE_STRING,-1,params[5]);
	   if (fd && str)
	     sizetext(fd,&CTX->current->r.w,&CTX->current->r.h,str);
	 }

	 /* Update bounding box */
	 canvas_extendbox(self,CTX->current);
      }
      if (params[0]==PG_GROP_SETCOLOR || 
	  (CTX->current->flags & PG_GROPF_COLORED))
	CTX->current->param[0] = VID(color_pgtohwr) 
	  (CTX->current->param[0]);
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
      if (CTX->current->type==PG_GROP_SETCOLOR || 
	  (CTX->current->flags & PG_GROPF_COLORED))
	CTX->current->param[0] = VID(color_pgtohwr) 
	  (CTX->current->param[0]);
      break;
      
    case PGCANVAS_MOVEGROP:
      if (numparams<4 || !CTX->current) return;
      CTX->current->r.x = params[0];
      CTX->current->r.y = params[1];
      CTX->current->r.w = params[2];
      CTX->current->r.h = params[3];
      break;
      
    case PGCANVAS_MUTATEGROP:
      if (numparams<1 || !CTX->current) return;
      CTX->current->type = params[0];
      break;
      
    case PGCANVAS_GROPFLAGS:
      if (numparams<1 || !CTX->current) return;
      CTX->current->flags = params[0];
      break;

    case PGCANVAS_DEFAULTFLAGS:
      if (numparams<1) return;
      CTX->defaultgropflags = params[0];
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
      self->in->div->flags |= DIVNODE_SCROLL_ONLY;
      break;

    case PGCANVAS_INPUTMAPPING:
      if (numparams<5) return;
      DATA->input_map.x   = params[0];
      DATA->input_map.y   = params[1];
      DATA->input_map.w   = params[2];
      DATA->input_map.h   = params[3];
      DATA->input_maptype = params[4];
      break;

    case PGCANVAS_GRIDSIZE:
      if (numparams<2) return;
      DATA->gridw = params[0];
      DATA->gridh = params[1];
      break;

   }
}
   
/* The End */
