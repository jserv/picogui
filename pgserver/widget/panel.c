/* $Id: panel.c,v 1.66 2001/10/09 02:06:30 micahjd Exp $
 *
 * panel.c - Holder for applications
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
#include <pgserver/video.h>
#include <pgserver/timer.h>
#include <pgserver/appmgr.h>

#define DRAG_DELAY    20   /* Min. # of milliseconds between
			      updates while dragging */

#define MINDRAGLEN    (vid->lxres/100)    /* Min. # of pixels movement for a click to be
		                     	    interpreted as a drag */

#define MAXROLLUP     (vid->lxres/50)     /* If the user resizes the panel to this
			                    or smaller, it will be interpreted
			                    as a panel roll-up */

/* the divnode making the whole (draggable) panelbar, including buttons */
#define BARDIV        self->in->div->div

/* The panelbar's thickness */
#define BARWIDTH      self->in->div->split

struct paneldata {
  int on,over;
  int grab_offset;  /* Difference between side of panel bar
		       and the point it was clicked */
  unsigned long wait_tick;    /* To limit the frame rate */

  /* Saved split value before a drag, used to
     calculate whether it should be interpreted as
     a click */
  int osplit;

  /* The split value while the panel is unrolled */
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

  /* buttons on the panelbar */
  struct widget *btn_close,*btn_rotate,*btn_zoom;

  /* The panelbar */
  struct divnode *panelbar;
};
#define DATA ((struct paneldata *)(self->data))

void themeify_panel(struct widget *self,bool force);
void panel_calcsplit(struct widget *self,int x,int y);
   
/**** Build and resize */

void panel_calcsplit(struct widget *self,int x,int y) {
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

void panel_resize(struct widget *self) {
  int s;

  /* Spacings */
  self->in->div->next->split = theme_lookup(DATA->panelbar->state,PGTH_P_MARGIN);
  BARWIDTH = theme_lookup(DATA->panelbar->state,PGTH_P_WIDTH);
   
  /* Button placement */
  s = theme_lookup(PGTH_O_CLOSEBTN,PGTH_P_SIDE);
  if ((self->in->flags & (~SIDEMASK)) & (PG_S_LEFT|PG_S_RIGHT))
    s = rotate_side(s);
  widget_set(DATA->btn_close,PG_WP_SIDE,s);  

  s = theme_lookup(PGTH_O_ROTATEBTN,PGTH_P_SIDE);
  if ((self->in->flags & (~SIDEMASK)) & (PG_S_LEFT|PG_S_RIGHT))
    s = rotate_side(s);
  widget_set(DATA->btn_rotate,PG_WP_SIDE,s);  

  s = theme_lookup(PGTH_O_ZOOMBTN,PGTH_P_SIDE);
  if ((self->in->flags & (~SIDEMASK)) & (PG_S_LEFT|PG_S_RIGHT))
    s = rotate_side(s);
  widget_set(DATA->btn_zoom,PG_WP_SIDE,s);  
}

void build_panelbar(struct gropctxt *c,unsigned short state,
		    struct widget *self) {
  struct fontdesc *fd;
  char *str;
  s16 x,y,w,h,m;
  s16 al = theme_lookup(state,PGTH_P_ALIGN);
  handle font = theme_lookup(state,PGTH_P_FONT);

  /* Background */
  exec_fillstyle(c,state,PGTH_P_BGFILL);
  
  /* Measure the exact width and height of the text and align it */
  if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,font))
      || !fd) return;
  if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,DATA->text))
      || !str) return;
  if (c->h > c->w) {
    al = mangle_align(al);
    sizetext(fd,&h,&w,str);
  }
  else
    sizetext(fd,&w,&h,str);
  if (w>c->w) w = c->w;
  if (h>c->h) h = c->h;
  align(c,al,&w,&h,&x,&y);
  if (c->h > c->w)
    y = c->h - y;

  addgrop(c,PG_GROP_SETCOLOR);
  c->current->param[0] = VID(color_pgtohwr) 
    (theme_lookup(state,PGTH_P_FGCOLOR));
  addgrop(c,PG_GROP_SETFONT);
  c->current->param[0] = theme_lookup(state,PGTH_P_FONT);
  if (c->w < c->h) {
     addgrop(c,PG_GROP_SETANGLE);
     c->current->param[0] = 90;
  }
  addgropsz(c,PG_GROP_TEXT,x,y,w,h);
  c->current->param[0] = DATA->text;
}

/**** Handlers for the panelbar buttons */

void panelbtn_close(struct widget *self,struct widget *button) {
  post_event(PG_WE_CLOSE,self,0,0,NULL);
}

void panelbtn_rotate(struct widget *self,struct widget *button) {
  switch (panel_get(self,PG_WP_SIDE)) {

  case PG_S_TOP:    widget_set(self,PG_WP_SIDE,PG_S_RIGHT); break;
  case PG_S_RIGHT:  widget_set(self,PG_WP_SIDE,PG_S_BOTTOM); break;
  case PG_S_BOTTOM: widget_set(self,PG_WP_SIDE,PG_S_LEFT); break;
  case PG_S_LEFT:   widget_set(self,PG_WP_SIDE,PG_S_TOP); break;

  }

  resizewidget(self);
  update(NULL,1);
}

void panelbtn_zoom(struct widget *self,struct widget *button) {
  struct divnode **where;
  
  /* This requires a bit of black magic, but fear not, it's not
     as bad as widget.c ;-) */
  
  /* Dereference the toolbar boundary */
  if (iserror(rdhandle((void**) &wtbboundary,PG_TYPE_WIDGET,-1,htbboundary)))
    wtbboundary = NULL;  

  /* Remove from our current position */
  *self->where = *self->out;
  if (*self->out && (*self->out)->owner)
    (*self->out)->owner->where = self->where;

  /* Where to move ourselves to */
  if (wtbboundary)
    where = wtbboundary->out;
  else
    where = &self->dt->head->next;

  /* Add again */
  *self->out = *where;
  *where = self->in;
  self->where = where;
  if (*self->out && (*self->out)->owner)
    (*self->out)->owner->where = self->out;

  /* Update other parameters related to the parent */
  if (wtbboundary)
    self->container = wtbboundary->container;

  /* tada! */
  self->dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
  self->dt->flags |= DIVTREE_NEED_RECALC;
  update(NULL,1);
}

/**** Installation */

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error panel_install(struct widget *self) {
  g_error e;

  /* Allocate data structure */
  e = g_malloc(&self->data,sizeof(struct paneldata));
  errorcheck;
  memset(self->data,0,sizeof(struct paneldata));

  /* This split determines the size of the main panel area */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags &= ~(DIVNODE_SIZE_AUTOSPLIT | DIVNODE_SIZE_RECURSIVE);
  self->in->flags |= PG_S_TOP;

  /* Split off another chunk of space for the bar */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags |= PG_S_BOTTOM;
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;

  /* This draws the panel background  */
  e = newdiv(&self->in->div->next,self);
  errorcheck;
  self->in->div->next->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->next->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->div->next->build = &build_bgfill_only;
  self->in->div->next->state = PGTH_O_PANEL;

  /* Close Button */
  e = widget_create(&DATA->btn_close,PG_WIDGET_BUTTON,self->dt,&self->in->div->div,
		    self->container,self->owner);
  errorcheck;
  customize_button(DATA->btn_close,PGTH_O_CLOSEBTN,PGTH_O_CLOSEBTN_ON,
		   PGTH_O_CLOSEBTN_HILIGHT,PGTH_O_CLOSEBTN_ON,self,
		   &panelbtn_close);

  /* Rotate Button */
  e = widget_create(&DATA->btn_rotate,PG_WIDGET_BUTTON,self->dt,DATA->btn_close->out,
		    self->container,self->owner);
  errorcheck;
  customize_button(DATA->btn_rotate,PGTH_O_ROTATEBTN,PGTH_O_ROTATEBTN_ON,
		   PGTH_O_ROTATEBTN_HILIGHT,PGTH_O_ROTATEBTN_ON,
		   self,&panelbtn_rotate);

  /* Zoom Button */
  e = widget_create(&DATA->btn_zoom,PG_WIDGET_BUTTON,self->dt,DATA->btn_rotate->out,
		    self->container,self->owner);
  errorcheck;
  customize_button(DATA->btn_zoom,PGTH_O_ZOOMBTN,PGTH_O_ZOOMBTN_ON,
		   PGTH_O_ZOOMBTN_HILIGHT,PGTH_O_ZOOMBTN_ON,
		   self,&panelbtn_zoom);

  /* And finally, the divnode that draws the panelbar */
  e = newdiv(DATA->btn_zoom->out,self);
  errorcheck;
  DATA->panelbar = *DATA->btn_zoom->out;
  DATA->panelbar->build = &build_panelbar;
  DATA->panelbar->state = PGTH_O_PANELBAR;

  self->sub = &self->in->div->next->div;
  self->out = &self->in->next;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | 
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE |
    TRIGGER_DRAG | TRIGGER_MOVE;

  return sucess;
}

/**** Properties */

void panel_remove(struct widget *self) {
  /* Kill the buttons */
  widget_remove(DATA->btn_close);
  widget_remove(DATA->btn_rotate);
  widget_remove(DATA->btn_zoom);

  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error panel_set(struct widget *self,int property, glob data) {
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

glob panel_get(struct widget *self,int property) {
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

void panel_trigger(struct widget *self,long type,union trigparam *param) {
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
       the edge shared by the panel and the
       panel bar. */
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
    themeify_panel(self,1);
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

    panel_calcsplit(self,param->mouse.x,param->mouse.y);
     
    if (abs(self->in->split - DATA->osplit) < MINDRAGLEN) {
      /* This was a click, not a drag */
      DATA->over = 0;

      if (DATA->osplit > BARWIDTH)
	/* Roll up the panel */
	self->in->split = BARWIDTH;
      else
	/* Unroll the panel */
	self->in->split = DATA->unrolled;
    }
    else {
      
      /* Save this as the new unrolled split,
       * Unless the user manually rolled up the panel */
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
	
     panel_calcsplit(self,param->mouse.x,param->mouse.y);
     self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
     self->dt->flags |= DIVTREE_NEED_RECALC;
     update(NULL,1);
     return;
     
#endif /* CONFIG_DRAGSOLID */
  }

  themeify_panel(self,force);
}

void themeify_panel(struct widget *self,bool force) {
  /* Apply the current state  */
  if (DATA->on)
    div_setstate(DATA->panelbar,PGTH_O_PANELBAR_ON,force);
  else if (DATA->over)
    div_setstate(DATA->panelbar,PGTH_O_PANELBAR_HILIGHT,force);
  else
    div_setstate(DATA->panelbar,PGTH_O_PANELBAR,force);
}

/* The End */




