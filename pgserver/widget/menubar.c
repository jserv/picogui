/* $Id: menubar.c,v 1.1 2001/12/12 15:23:34 epchristi Exp $
 *
 * menubar.c - Holder for applications
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 * Copyright (C) 2001 RidgeRun Inc.
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
 * Eric Christianson
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/video.h>
#include <pgserver/timer.h>
#include <pgserver/appmgr.h>
#include <picogui/menu.h>
/*  #define DEBUG_WIDGET */
#ifdef DEBUG_WIDGET
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>


#define DRAG_DELAY    20   /* Min. # of milliseconds between
			      updates while dragging */

#define MINDRAGLEN    (vid->lxres/100)    /* Min. # of pixels movement for a click to be
		                     	    interpreted as a drag */

#define MAXROLLUP     (vid->lxres/50)     /* If the user resizes the menubar to this
			                    or smaller, it will be interpreted
			                    as a menubar roll-up */

/* the divnode making the whole (draggable) panelbar, including buttons */
#define BARDIV        self->in->div->div

/* The panelbar's thickness */
#define BARWIDTH      self->in->div->split

struct menubardata {
  int on,over;
  int grab_offset;  /* Difference between side of menubar bar
		       and the point it was clicked */
  unsigned long wait_tick;    /* To limit the frame rate */

  /* Saved split value before a drag, used to
     calculate whether it should be interpreted as
     a click */
  int osplit;

  /* The split value while the menubar is unrolled */
  int unrolled;

   /* Because the divnode coordinates change when dragging in solid mode,
    * the x,y coordinate must be saved before starting */
  int x,y;
   
#ifndef CONFIG_DRAGSOLID
  /* Sprite for dragging the panelbar */
  struct sprite *s;
  hwrbitmap sbit;
#endif

  /* Text on the panelbar */
  handle text;

  /* popup to be used for placing menu items */
  struct widget *btn_box;
  struct widget **wlist;
  int numwidgets;
  
  /* The panelbar */
  struct divnode *panelbar;
};
#define DATA ((struct menubardata *)(self->data))

void themeify_menubar(struct widget *self,bool force);
void menubar_calcsplit(struct widget *self,int x,int y);
   
/**** Build and resize */

void menubar_calcsplit(struct widget *self,int x,int y) {
   /* Now use grab_offset to calculate a new split value */
   switch (self->in->flags & (~SIDEMASK)) {
    case PG_S_TOP:
      self->in->split =  y - DATA->grab_offset - DATA->y;
      if (self->in->split + BARDIV->h > self->in->h)
	self->in->split = self->in->h - BARDIV->h;
      break;
    case PG_S_BOTTOM:
      self->in->split = DATA->y + self->in->h - 1 -
	y - DATA->grab_offset;
      if (self->in->split + BARDIV->h > self->in->h)
	self->in->split = self->in->h - BARDIV->h;
      break;
    case PG_S_LEFT:
      self->in->split =  x - DATA->grab_offset - DATA->x;
      if (self->in->split + BARDIV->w > self->in->w)
	self->in->split = self->in->w - BARDIV->w;
      break;
    case PG_S_RIGHT:
      self->in->split = DATA->x + self->in->w - 1 -
	x - DATA->grab_offset;
      if (self->in->split + BARDIV->w > self->in->w)
	self->in->split = self->in->w - BARDIV->w;
      break;
   }
   if (self->in->split < 0) self->in->split = 0;
   self->in->split += BARWIDTH;    /* Account for panelbar height */
}

void menubar_resize(struct widget *self) {
  int s;

  /* Spacings */
  self->in->div->next->split = theme_lookup(DATA->panelbar->state,PGTH_P_MARGIN);
  BARWIDTH = theme_lookup(DATA->panelbar->state,PGTH_P_WIDTH);
   
  /* Button placement */
  s = theme_lookup(PGTH_O_MENUBUTTON,PGTH_P_SIDE);
  if ((self->in->flags & (~SIDEMASK)) & (PG_S_LEFT|PG_S_RIGHT))
    s = rotate_side(s);
  /* TODO: */
/*    widget_set(DATA->btn_menu,PG_WP_SIDE,s);    */
  /* This needs to do something with all the buttons likely */
}

void build_menubar(struct gropctxt *c,unsigned short state,
		    struct widget *self) {
  struct fontdesc *fd;
  char *str;
  s16 x,y,w,h,m;
  handle font = theme_lookup(state,PGTH_P_FONT);

  s16 text_align = PG_A_RIGHT; /* theme_lookup(state,PGTH_O_MENUBAR_TEXT_ALIGN); */
  handle text_font = font; /*theme_lookup(state,PGTH_O_MENUBAR_TEXT_FONT);*/

  s16 title_align = PG_A_LEFT; /*  theme_lookup(state,PGTH_O_MENUBAR_TITLE_ALIGN); */
  handle title_font = font; /*theme_lookup(state,PGTH_O_MENUBAR_TITLE_FONT);*/

  /* Background */
  exec_fillstyle(c,state,PGTH_P_BGFILL);

  DBG(":Fill Background\n");
  
  /* Measure the exact width and height of the text and align it */
  if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,text_font))
      || !fd) return;
  if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,DATA->text))
      || !str) return;
  if (c->h > c->w) {
    text_align = mangle_align(text_align);
    sizetext(fd,&h,&w,str);
  }
  else
    sizetext(fd,&w,&h,str);
  if (w>c->w) w = c->w;
  if (h>c->h) h = c->h;
  align(c,text_align,&w,&h,&x,&y);

  DBG(":align_text w=%d h=%d x=%d y=%d\n",w,h,x,y);

  if (c->h > c->w)
    y = c->h - y;

  DBG(":align_text y (adjusted)=%d\n",y);

  addgrop(c,PG_GROP_SETCOLOR);
  c->current->param[0] = VID(color_pgtohwr) 
    (theme_lookup(state,PGTH_P_FGCOLOR));
  addgrop(c,PG_GROP_SETFONT);
  c->current->param[0] = text_font;
  if (c->w < c->h) {
     addgrop(c,PG_GROP_SETANGLE);
     c->current->param[0] = 90;
  }
  addgropsz(c,PG_GROP_TEXT,x,y,w,h);
  c->current->param[0] = DATA->text;

  /* align title */
  if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,title_font))
      || !fd) {
    DBG(" no font\n");
    return;
  }
  if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,self->name))
      || !str) {
    DBG(" no string\n");
    return;
  } 
  if (c->h > c->w) {
    title_align = mangle_align(title_align);
    sizetext(fd,&h,&w,str);
  }
  else
    sizetext(fd,&w,&h,str);
  if (w>c->w) w = c->w;
  if (h>c->h) h = c->h;
  align(c,title_align,&w,&h,&x,&y);

  DBG( ":align_title w=%d h=%d x=%d y=%d\n",w,h,x,y);

  if (c->h > c->w)
    y = c->h - y;

  DBG(":align_title y (adjusted)=%d\n",y);

  addgrop(c,PG_GROP_SETCOLOR);
  c->current->param[0] = VID(color_pgtohwr) 
    (theme_lookup(state,PGTH_P_FGCOLOR));
  addgrop(c,PG_GROP_SETFONT);
  c->current->param[0] = title_font;
  if (c->w < c->h) {
     addgrop(c,PG_GROP_SETANGLE);
     c->current->param[0] = 90;
  }
  addgropsz(c,PG_GROP_TEXT,x,y,w,h);
  c->current->param[0] = self->name;

}

/**** Installation */

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error menubar_install(struct widget *self) {
  g_error e;
  char *buf;

  DBG("\n");

  /* Allocate data structure */
  e = g_malloc(&self->data,sizeof(struct menubardata));
  errorcheck;
  memset(self->data,0,sizeof(struct menubardata));

  /* initialize data */
  DATA->wlist=0;
  DATA->numwidgets=0;

  /* This split determines the size of the main menubar area */
  e = newdiv(&self->in,self);
  errorcheck;
  DBG("newdiv = %x\n",self->in);
  self->in->flags &= ~(DIVNODE_SIZE_AUTOSPLIT | DIVNODE_SIZE_RECURSIVE);
  self->in->flags |= PG_S_TOP;

  /* Split off another chunk of space for the bar */
  e = newdiv(&self->in->div,self);
  errorcheck;
  DBG("newdiv = %x\n",self->in->div);
  self->in->div->flags |= PG_S_BOTTOM;
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;

  /* This draws the menubar background  */
  e = newdiv(&self->in->div->next,self);
  errorcheck;
  DBG("newdiv = %x\n",self->in->div->next);

  self->in->div->next->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->next->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->div->next->build = &build_bgfill_only;
  self->in->div->next->state = PGTH_O_PANEL;

  /* Menu Button Box */
  e = newdiv(&self->in->div->div,self);
  errorcheck;
  DBG("newdiv = %x\n",self->in->div->div);

  e = widget_create(&DATA->btn_box, PG_WIDGET_BOX, self->dt, self->container, self->owner);
  errorcheck;
  e = widget_attach(DATA->btn_box, self->dt,&self->in->div->div, self->container,self->owner);
  errorcheck;

  /* And finally, the divnode that draws the panelbar */
  e = newdiv(DATA->btn_box->out,self);
  errorcheck;

  DATA->panelbar = *DATA->btn_box->out; 
  DATA->panelbar->build = &build_menubar;
  DATA->panelbar->state = PGTH_O_PANELBAR;

  self->sub = &self->in->div->next->div;
  self->out = &self->in->next;

  self->trigger_mask = TRIGGER_STREAM | TRIGGER_ENTER | TRIGGER_LEAVE | 
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE |
    TRIGGER_DRAG | TRIGGER_MOVE;

  return sucess;
}

/**** Properties */

void menubar_remove(struct widget *self) {
  int i;
  DBG("\n");

  /* remove widgets attached */
  for(i=0; i<DATA->numwidgets; i++) {
    DBG("remove widget 0x%x from position %d\n",DATA->wlist[i],i);
    widget_remove((struct widget *) DATA->wlist[i]);
  }

  /* kill the box */
  DBG("remove box\n");
  widget_remove(DATA->btn_box); 


  DBG("g_free\n");
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);

  DBG("EXIT\n");

}

g_error menubar_set(struct widget *self,int property, glob data) {
  char *str;

  switch (property) {

  case PG_WP_SIDE:
     /* Set extra flags for the panelbar */
     self->in->div->flags &= SIDEMASK;
     switch (data) {    /* Invert the side for the panelbar */
      case PG_S_TOP:    self->in->div->flags |= PG_S_BOTTOM; break;
      case PG_S_BOTTOM: self->in->div->flags |= PG_S_TOP; break;
      case PG_S_LEFT:   self->in->div->flags |= PG_S_RIGHT; break;
      case PG_S_RIGHT:  self->in->div->flags |= PG_S_LEFT; break;
     }
     return mkerror(ERRT_PASS,0);

  case PG_WP_SIZE:
    if (data<0)
      data = 0;
    self->in->split = data;
    if (data>0)
      DATA->unrolled = data;
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_TEXT:
    if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,data)) || !str) 
      return mkerror(PG_ERRT_HANDLE,13);
    DATA->text = (handle) data;
    resizewidget(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_THOBJ:
     self->in->div->next->state = data;
     resizewidget(self);
     self->in->flags |= DIVNODE_NEED_RECALC;
     self->dt->flags |= DIVTREE_NEED_RECALC;
     break;

  default:
    return mkerror(ERRT_PASS,0);

  }
  return sucess;
}

glob menubar_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case PG_WP_SIZE:
    return self->in->split;

  case PG_WP_TEXT:
    return DATA->text;

  case PG_WP_THOBJ:
    return self->in->div->next->state;;
    
  }
  return 0;
}

static g_error insert_widget(struct widget *self, struct widget *w)
{
  g_error e;

  DBG("type=%d parent=%x\n",w->type,DATA->btn_box,DATA->btn_box);

  e = widget_derive(&w, w->type, DATA->btn_box, DATA->btn_box->container, PG_DERIVE_BEFORE, self->owner);
  errorcheck;

  /* realloc space in the list of added widgets for later removal */
  DBG("adding widget 0x%x to position %x\n",w,DATA->numwidgets);
  DATA->wlist = (struct widget **) realloc((struct widget **)DATA->wlist, sizeof(struct widget)*(1+DATA->numwidgets));
  DATA->wlist[DATA->numwidgets] = w;
  DATA->numwidgets++;

  /* now fix up the divtree */
  w->out = &DATA->panelbar; 

  return sucess;

}

static g_error menu_command(struct widget *self, unsigned short command, 
			    unsigned short numparams,signed long *params) {
   int i;
   g_error e;
   struct widget *w;

   DBG("command = %d\n",command);

   switch (command) {

   case PGMENU_ADD :
     e = rdhandle((void **) &w, PG_TYPE_WIDGET, -1, *params);
     errorcheck;
     
     DBG("PGMENU_ADD w=0x%x type=%d handle=%x\n",w,w->type,params[0]);
     /* traverse list, and then attach and reconstruct */
     e = insert_widget(self,w);
     errorcheck;

     update(NULL,1);
     break;


   case PGMENU_REMOVE :
     /* TODO: Remove widget */
     break;

   default:
     return mkerror(ERRT_PASS,0);
   }

   return 0;
}


void menubar_trigger(struct widget *self,long type,union trigparam *param) {
  unsigned long tick;
  int tmpover;
  g_error e;
  bool force = 0;
   
  switch (type) {

  case TRIGGER_ENTER:
  case TRIGGER_MOVE:
    /* Handle entering/exiting the node */
    tmpover = div_under_crsr == DATA->panelbar;
    if (DATA->over == tmpover) return;
    DATA->over = tmpover;
    break;

  case TRIGGER_LEAVE:
    /* If we're dragging, the mouse didn't REALLY leave */
    if (DATA->on) return;

    if (!DATA->over) return;  /* Don't bother redrawing */

    DATA->over=0;
    break;

  case TRIGGER_DOWN:
    if (param->mouse.chbtn != 1) return;
    if (DATA->panelbar != div_under_crsr) return;

    /* Calculate grab_offset with respect to
       the edge shared by the menubar and the
       menubar bar. */
    switch (self->in->flags & (~SIDEMASK)) {
    case PG_S_TOP:
      DATA->grab_offset = param->mouse.y - DATA->panelbar->y;
      break;
    case PG_S_BOTTOM:
      DATA->grab_offset = DATA->panelbar->y + DATA->panelbar->h - 1 - param->mouse.y;
      break;
    case PG_S_LEFT:
      DATA->grab_offset = param->mouse.x - DATA->panelbar->x;
      break;
    case PG_S_RIGHT:
      DATA->grab_offset = DATA->panelbar->x + DATA->panelbar->w - 1 - param->mouse.x;
      break;
    }

    DATA->osplit = self->in->split;
    DATA->on = 1;
    DATA->x = self->in->x;
    DATA->y = self->in->y;
     
#ifndef CONFIG_DRAGSOLID
    /* Sprite dragging code */
     
    /* Update the screen now, so we have an up-to-date picture
       of the panelbar stored in DATA->bar */
    themeify_menubar(self,1);
    VID(sprite_hideall) ();   /* This line combined with the zero flag on */
    update(NULL,0);             /*  the next gets a clean spriteless grab */

    /* In case there was no release trigger (bug in input driver) */
    if (DATA->s) {
      free_sprite(DATA->s);
      DATA->s = NULL;
    }
    if (DATA->sbit) {
      VID(bitmap_free) (DATA->sbit);
      DATA->sbit = NULL;
    }
     
    /* Allocate the new sprite */
    if(iserror(new_sprite(&DATA->s,BARDIV->w,BARDIV->h))) {
      DATA->s = NULL;
      return;
    }
    if (iserror(VID(bitmap_new) (&DATA->sbit,BARDIV->w,BARDIV->h))) {
      free_sprite(DATA->s);
      DATA->s = NULL;
      DATA->sbit = NULL;
      return;
    }
    DATA->s->bitmap = &DATA->sbit;
    
    /* Grab a bitmap of the panelbar to use as the sprite */
    VID(blit) (DATA->sbit,0,0,BARDIV->w,BARDIV->h,
	       vid->display,DATA->s->x = BARDIV->x,DATA->s->y = BARDIV->y,
	       PG_LGOP_NONE);
    DATA->s->clip_to = self->in;

    return;
#else
    break;
#endif /* CONFIG_DRAGSOLID */

  case TRIGGER_UP:
  case TRIGGER_RELEASE:
    if (!DATA->on) return;
    if (!(param->mouse.chbtn & 1)) return;

    menubar_calcsplit(self,param->mouse.x,param->mouse.y);
     
    if (abs(self->in->split - DATA->osplit) < MINDRAGLEN) {
      /* This was a click, not a drag */
      DATA->over = 0;

      if (DATA->osplit > BARWIDTH)
	/* Roll up the menubar */
	self->in->split = BARWIDTH;
      else
	/* Unroll the menubar */
	self->in->split = DATA->unrolled;
    }
    else {
      
      /* Save this as the new unrolled split,
       * Unless the user manually rolled up the menubar */
      if ((self->in->split-BARWIDTH) > MAXROLLUP) 
	DATA->unrolled = self->in->split;
      else
	self->in->split = BARWIDTH;

      DATA->over = 1;
    }

    /* Do a recalc, because we just changed everything's size */
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;

#ifndef CONFIG_DRAGSOLID
    VID(bitmap_free) (DATA->sbit);
    free_sprite(DATA->s);
    DATA->s = NULL;
    DATA->sbit = NULL;
    force = 1;           /* Definitely draw the new position */
#endif
     
    DATA->on = 0;
    break;

#ifndef CONFIG_DRAGSOLID
    /* Sprite dragging code */

  case TRIGGER_DRAG:
    if (!DATA->on) return;
    /* Ok, button 1 is dragging through our widget... */
     
    /* If possible, use the input driver to see when we're behind
     * and skip a frame. Otherwise, just use a timer as a throttle */
    if (events_pending())
      return;
    tick = getticks();
    if (tick < DATA->wait_tick) return;
    DATA->wait_tick = tick + DRAG_DELAY;
     
    /* Race condition prevention?
     * Without this, sometimes segfaults because DATA->s is NULL.
     * Possibly events_pending() triggered another event? */
    if (!DATA->s) return;
      
    /* Determine where to blit the bar to... */
    switch (self->in->flags & (~SIDEMASK)) {
    case PG_S_TOP:
      DATA->s->x = BARDIV->x;
      DATA->s->y = param->mouse.y - DATA->grab_offset;
      break;
    case PG_S_BOTTOM:
      DATA->s->x = BARDIV->x;
      DATA->s->y = param->mouse.y + DATA->grab_offset - BARDIV->h;
      break;
    case PG_S_LEFT:
      DATA->s->y = BARDIV->y;
      DATA->s->x = param->mouse.x - DATA->grab_offset;
      break;
    case PG_S_RIGHT:
      DATA->s->y = BARDIV->y;
      DATA->s->x = param->mouse.x + DATA->grab_offset - BARDIV->w;
      break;
    }

    /* Reposition sprite */
    VID(sprite_update) (DATA->s);
    return;

#else /* CONFIG_DRAGSOLID */     
    /* Solid dragging code, abuse the CPU */
     
  case TRIGGER_DRAG:
    if (!DATA->on) return;

    /* If possible, use the input driver to see when we're behind
     * and skip a frame. Otherwise, just use a timer as a throttle */
    if (events_pending())
      return;
    tick = getticks();
    if (tick < DATA->wait_tick) return;
    DATA->wait_tick = tick + DRAG_DELAY;
	
    menubar_calcsplit(self,param->mouse.x,param->mouse.y);
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    update(NULL,1);
    return;
     
#endif /* CONFIG_DRAGSOLID */

  case TRIGGER_STREAM:
    {
      struct pgcommand *cmd;
      char *buffer = param->stream.data;
      unsigned long remaining = param->stream.size;
      int i;
      signed long *params;
      
      DBG(": TRIGGER_STREAM - param.data = 0x%x, param.size = %d\n", param->stream.data, param->stream.size);
      //
      // Process all the commands received.
      //
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

	//
	// Finished processing?
	//
	if (remaining < 0)
	  break;
	 
	/* Convert parameters */
	for (i=0;i<cmd->numparams;i++)
	  params[i] = ntohl(params[i]);
	 
	menu_command(self,cmd->command,cmd->numparams,params);
	 
      } // End of remaining
    }
    break;

  }

  themeify_menubar(self,force);
}

void themeify_menubar(struct widget *self,bool force) {
  /* Apply the current state  */
  if (DATA->on)
    div_setstate(DATA->panelbar,PGTH_O_PANELBAR_ON,force);
  else if (DATA->over)
    div_setstate(DATA->panelbar,PGTH_O_PANELBAR_HILIGHT,force);
  else
    div_setstate(DATA->panelbar,PGTH_O_PANELBAR,force);
}

/* The End */




