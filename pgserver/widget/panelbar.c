/* $Id: panelbar.c,v 1.7 2002/05/20 19:18:38 micahjd Exp $
 *
 * panelbar.c - Container and draggable bar for resizing panels
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

#include <string.h>
#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/video.h>
#include <pgserver/timer.h>
#include <pgserver/input.h>
#ifdef CONFIG_DRAGSOLID
#include <pgserver/configfile.h>
#endif

#define DRAG_DELAY    20   /* Min. # of milliseconds between
			      updates while dragging */

/* The panelbar's thickness */
#define BARWIDTH      self->in->split

#define MINDRAGLEN    (BARWIDTH/2)    /* Min. # of pixels movement for a click to be
					 interpreted as a drag */

#define MAXROLLUP     (BARWIDTH/2)    /* If the user resizes the panel to this
					 or smaller, it will be interpreted
					 as a panel roll-up */

struct panelbardata {
  u32 wait_tick;    /* To limit the frame rate */
  int oldsize;                /* Size before we started dragging */
  int unrolled;               /* Size when the panel is unrolled */
  int x,y;                    /* Mouse x,y at click */
  struct sprite *s;           /* Sprite for dragging the panelbar */
  hwrbitmap sbit;             /* Sprite bitmap (no bitmask) */
  struct divnode *panelbar;   /* Panelbar itself */
  handle bindto;              /* Widget the panelbar resizes */
  unsigned int on : 1;        /* Mouse is pressed on the widget */
  unsigned int over : 1;      /* Mouse is over the widget */

#ifdef CONFIG_DRAGSOLID
  unsigned int solid : 1;
#endif
};
#define DATA ((struct panelbardata *)(self->data))

void themeify_panelbar(struct widget *self,bool force);
int panel_calcsplit(struct widget *self,int x,int y,int flags);  
int panel_effective_split(struct divnode *d);

/* Return 1 if we should skip a frame */
int panel_throttle(struct widget *self);

/* Two different versions of the trigger function */
void panelbar_trigger_sprite(struct widget *self,s32 type,union trigparam *param);
void panelbar_trigger_solid(struct widget *self,s32 type,union trigparam *param);

/**** Build and resize */

int panel_throttle(struct widget *self) {
  u32 tick;

  /* If possible, use the input driver to see when we're behind
   * and skip a frame. Otherwise, just use a timer as a throttle */
  if (events_pending())
    return 1;
  tick = getticks();
  if (tick < DATA->wait_tick)
    return 1;
  DATA->wait_tick = tick + DRAG_DELAY;  

  return 0;
}

/* Using the new position, and the stored old position, calculate the delta
 * split for the container we're attached to
 */
int panel_calcsplit(struct widget *self,int x,int y,int flags) {
   switch (flags & (~SIDEMASK)) {
    case PG_S_TOP:
      return y - DATA->y;
    case PG_S_BOTTOM:
      return  -(y - DATA->y);
    case PG_S_LEFT:
      return x - DATA->x;
    case PG_S_RIGHT:
      return -(x - DATA->x);
   }
   return 0;
}

/* Find the effective value of the divnode's "split" in pixels,
 * so we can calculate the new split relative to it.
 *
 * We can't use the real 'split' value since it may not be
 * specified in pixels, and it may be greater than the size
 * available to the widget.
 */
int panel_effective_split(struct divnode *d) {
  if (!d->div)
    return 0;
  switch (d->flags & (~SIDEMASK)) {
  case PG_S_LEFT:
  case PG_S_RIGHT:
    return d->div->calcw;
  case PG_S_TOP:
  case PG_S_BOTTOM:
    return d->div->calch;
  }
  return 0;
}

void panelbar_resize(struct widget *self) {
  /* Look up theme parameters */
  self->in->split = theme_lookup(self->in->div->state,PGTH_P_WIDTH);
  self->in->div->split = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
  self->in->div->pw = self->in->div->ph = self->in->split;
}

g_error panelbar_install(struct widget *self) {
  /* Divtree set up almost exactly like the box widget */
  g_error e;

  e = g_malloc(&self->data,sizeof(struct panelbardata));
  errorcheck;
  memset(self->data,0,sizeof(struct panelbardata));

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_BOTTOM;
  self->in->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_PANELBAR;
  DATA->panelbar = self->in->div;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | TRIGGER_DOWN |
    TRIGGER_UP | TRIGGER_RELEASE | TRIGGER_DRAG | TRIGGER_MOVE;

  /* Panelbars use auto-orientation by default */
  self->auto_orientation = 1;

#ifdef CONFIG_DRAGSOLID
  DATA->solid = get_param_int("pgserver","dragsolid",0);
#endif

  return success;
}

void panelbar_remove(struct widget *self) {
  if (DATA->s) {
    free_sprite(DATA->s);
    DATA->s = NULL;
  }
  if (DATA->sbit) {
    VID(bitmap_free) (DATA->sbit);
    DATA->sbit = NULL;
  }
  g_free(self->data);
  r_divnode_free(self->in);
}

g_error panelbar_set(struct widget *self,int property, glob data) {
  g_error e;
  struct widget *w;

  switch (property) {
    
  case PG_WP_BIND:
    e = rdhandle((void **)&w,PG_TYPE_WIDGET,self->owner,data);
    errorcheck;
    DATA->bindto = data;
    /* FIXME: this should be in pixels */
    DATA->unrolled = widget_get(w, PG_WP_SIZE);
    break;

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob panelbar_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_BIND:
    return DATA->bindto;

  }
  return 0;
}

/* Decide on either sprite or solid dragging */
void panelbar_trigger(struct widget *self,s32 type,union trigparam *param) {
#ifdef CONFIG_DRAGSOLID
  if (DATA->solid)
    panelbar_trigger_solid(self,type,param);
  else
#endif
    panelbar_trigger_sprite(self,type,param);
}

void panelbar_trigger_sprite(struct widget *self,s32 type,union trigparam *param) {
  bool force = 0;
  struct widget *boundwidget;
  int s;

  switch (type) {

  case TRIGGER_ENTER:
    DATA->over = 1;
    break;

  case TRIGGER_LEAVE:
    /* If we're dragging, the mouse didn't REALLY leave */
    if (DATA->on) return;
    DATA->over=0;
    break;

  case TRIGGER_DOWN:
    if (param->mouse.chbtn != 1) return;

    /* If we're bound to another widget (we should be) save its current size */
    if (!iserror(rdhandle((void**)&boundwidget,PG_TYPE_WIDGET,self->owner,
			  DATA->bindto)) && boundwidget) {
      DATA->oldsize = widget_get(boundwidget,PG_WP_SIZE);
    }

    DATA->on = 1;
    DATA->x = param->mouse.x;
    DATA->y = param->mouse.y;
     
    /* Update the screen now, so we have an up-to-date picture
       of the panelbar stored in DATA->s */
    themeify_panelbar(self,1);

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
    if(iserror(new_sprite(&DATA->s,DATA->panelbar->w,DATA->panelbar->h))) {
       DATA->s = NULL;
       return;
    }
    if (iserror(VID(bitmap_new) (&DATA->sbit,DATA->panelbar->w,DATA->panelbar->h,vid->bpp))) {
       free_sprite(DATA->s);
       DATA->s = NULL;
       DATA->sbit = NULL;
       return;
    }
    DATA->s->bitmap = &DATA->sbit;
    
    /* Grab a bitmap of the panelbar to use as the sprite */
    VID(blit) (DATA->sbit,0,0,DATA->panelbar->w,DATA->panelbar->h,
	       vid->display,DATA->s->x = DATA->panelbar->x,DATA->s->y = DATA->panelbar->y,
	       PG_LGOP_NONE);

    /* Clip the sprite to the travel allowed by boundwidget's parent */
    if (!iserror(rdhandle((void**)&boundwidget,PG_TYPE_WIDGET,self->owner,
			  DATA->bindto)) && boundwidget) {
      DATA->s->clip_to = boundwidget->in;
    }

    return;

  case TRIGGER_UP:
  case TRIGGER_RELEASE:
    if (!DATA->on) return;
    if (!(param->mouse.chbtn & 1)) return;

    if (!iserror(rdhandle((void**)&boundwidget,PG_TYPE_WIDGET,self->owner,
			  DATA->bindto)) && boundwidget) {

      s = panel_calcsplit(self,param->mouse.x,param->mouse.y,
			  widget_get(boundwidget,PG_WP_SIDE));
      
      if (abs(s) < MINDRAGLEN) {
	/* This was a click, not a drag */
	DATA->over = 0;
	
	widget_set(boundwidget,PG_WP_SIZEMODE,PG_SZMODE_PIXEL);
	if (DATA->oldsize > BARWIDTH)
	  /* Roll up the panel */
	  widget_set(boundwidget,PG_WP_SIZE,BARWIDTH);
	else
	  /* Unroll the panel */
	  widget_set(boundwidget,PG_WP_SIZE,DATA->unrolled);
      }
      else {
	s += panel_effective_split(boundwidget->in);

	/* Save this as the new unrolled split,
	 * Unless the user manually rolled up the panel */
	if ((s-BARWIDTH) > MAXROLLUP) 
	  DATA->unrolled = s;
	else
	  s = BARWIDTH;
	
	widget_set(boundwidget,PG_WP_SIZEMODE,PG_SZMODE_PIXEL);
	widget_set(boundwidget,PG_WP_SIZE,s);

	DATA->over = 1;
      }
    }

    VID(bitmap_free) (DATA->sbit);
    free_sprite(DATA->s);
    DATA->s = NULL;
    DATA->sbit = NULL;
    force = 1;           /* Definitely draw the new position */
     
    DATA->on = 0;
    break;

  case TRIGGER_MOVE:
  case TRIGGER_DRAG:
    if (!DATA->on) return;
     /* Ok, button 1 is dragging through our widget... */
     
    if (panel_throttle(self))
      return;
     
     /* Race condition prevention?
      * Without this, sometimes segfaults because DATA->s is NULL.
      * Possibly events_pending() triggered another event? */
     if (!DATA->s) return;
      
     /* Determine where to blit the bar to... */
     switch (self->in->flags & (~SIDEMASK)) {
     case PG_S_TOP:
     case PG_S_BOTTOM:
       DATA->s->x = DATA->panelbar->x;
       DATA->s->y = param->mouse.y - DATA->y + DATA->panelbar->y;
       break;
     case PG_S_LEFT:
     case PG_S_RIGHT:
       DATA->s->y = DATA->panelbar->y;
       DATA->s->x = param->mouse.x - DATA->x + DATA->panelbar->x;
       break;
     }

    /* Reposition sprite */
    VID(sprite_update) (DATA->s);
    return;
  }

  themeify_panelbar(self,force);
}

#ifdef CONFIG_DRAGSOLID
void panelbar_trigger_solid(struct widget *self,s32 type,union trigparam *param) {
  bool force = 0;
  struct widget *boundwidget;
  int s;

  switch (type) {

  case TRIGGER_ENTER:
    DATA->over = 1;
    break;
  case TRIGGER_LEAVE:
    DATA->over=0;
    break;

  case TRIGGER_DOWN:
    if (param->mouse.chbtn != 1) return;

    /* If we're bound to another widget (we should be) save its current size */
    if (!iserror(rdhandle((void**)&boundwidget,PG_TYPE_WIDGET,self->owner,
			  DATA->bindto)) && boundwidget) {
      DATA->oldsize = widget_get(boundwidget,PG_WP_SIZE);

      /* To make things easier during dragging,
       * get the effective split to equal the split
       */
      widget_set(boundwidget, PG_WP_SIZEMODE, PG_SZMODE_PIXEL);
      widget_set(boundwidget, PG_WP_SIZE,
		 panel_effective_split(boundwidget->in));	 
    }

    DATA->on = 1;
    DATA->x = param->mouse.x;
    DATA->y = param->mouse.y;
    break;

  case TRIGGER_UP:
  case TRIGGER_RELEASE:
    if (!DATA->on) return;
    if (!(param->mouse.chbtn & 1)) return;

    if (!iserror(rdhandle((void**)&boundwidget,PG_TYPE_WIDGET,self->owner,
			  DATA->bindto)) && boundwidget) {

      if (abs(DATA->oldsize - widget_get(boundwidget,PG_WP_SIZE)) < MINDRAGLEN) {
	/* This was a click, not a drag */
	DATA->over = 0;
	
	widget_set(boundwidget,PG_WP_SIZEMODE,PG_SZMODE_PIXEL);
	if (DATA->oldsize > BARWIDTH)
	  /* Roll up the panel */
	  widget_set(boundwidget,PG_WP_SIZE,BARWIDTH);
	else
	  /* Unroll the panel */
	  widget_set(boundwidget,PG_WP_SIZE,DATA->unrolled);

	update(NULL,1);
      }
      else {
	s = panel_effective_split(boundwidget->in);

	/* Save this as the new unrolled split,
	 * Unless the user manually rolled up the panel */
	if ((s-BARWIDTH) > MAXROLLUP) 
	  DATA->unrolled = s;
	else
	  s = BARWIDTH;
      }
    }

    DATA->on = 0;
    break;

  case TRIGGER_MOVE:
  case TRIGGER_DRAG:
     if (!DATA->on) return;
     if (panel_throttle(self))
       return;
     
     if (!iserror(rdhandle((void**)&boundwidget,PG_TYPE_WIDGET,self->owner,
			   DATA->bindto)) && boundwidget) {
     
       s = panel_calcsplit(self,param->mouse.x,param->mouse.y,
			   widget_get(boundwidget,PG_WP_SIDE));
       
       if (s) {
	 s += boundwidget->in->split;

	 /* FIXME: This isn't quite right, in solid dragging mode the panelbar
	  * can get out of sync with the cursor when this s < BARWIDTH code
	  * comes into effect.
	  */
	 if (s < BARWIDTH)
	   s = BARWIDTH;
	 else {
	   DATA->x = param->mouse.x;
	   DATA->y = param->mouse.y;
	 }

	 widget_set(boundwidget, PG_WP_SIZE,s);
	 update(NULL,1);
       }
     }
     return;
  }

  themeify_panelbar(self,force);
}
#endif /* CONFIG_DRAGSOLID */

void themeify_panelbar(struct widget *self,bool force) {
  /* Apply the current state  */
  if (DATA->on)
    div_setstate(DATA->panelbar,PGTH_O_PANELBAR_ON,force);
  else if (DATA->over)
    div_setstate(DATA->panelbar,PGTH_O_PANELBAR_HILIGHT,force);
  else
    div_setstate(DATA->panelbar,PGTH_O_PANELBAR,force);
}

/* The End */




