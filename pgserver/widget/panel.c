/* $Id: panel.c,v 1.31 2000/10/29 01:45:35 micahjd Exp $
 *
 * panel.c - Holder for applications
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
#include <pgserver/video.h>
#include <pgserver/timer.h>

#define PANELBAR_SIZE 15

#define DRAG_DELAY    20   /* Min. # of milliseconds between
			      updates while dragging */

#define MINDRAGLEN    4    /* Min. # of pixels movement for a click to be
			      interpreted as a drag */

#define MAXROLLUP     10   /* If the user resizes the panel to this
			      or smaller, it will be interpreted
			      as a panel roll-up */

/* A shortcut... */
#define PANELBAR_DIV self->in->next->div

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

  /* Sprite for dragging the panelbar */
  struct sprite *s;
};
#define DATA ((struct paneldata *)(self->data))

void themeify_panel(struct widget *self);

void resize_panel(struct widget *self) {
  self->in->div->split = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
  self->in->next->split = theme_lookup(self->in->next->div->state,PGTH_P_WIDTH);
}

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
  self->in->flags |= PG_S_TOP;

  /* This draws the panel background  */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_PANEL;

  /* Split off another chunk of space for the bar */
  e = newdiv(&self->in->next,self);
  errorcheck;
  self->in->next->flags |= PG_S_TOP;

  /* And finally, the divnode that draws the panelbar */
  e = newdiv(&self->in->next->div,self);
  errorcheck;
  self->in->next->div->build = &build_bgfill_only;
  self->in->next->div->state = PGTH_O_PANELBAR;

  self->sub = &self->in->div->div;
  self->out = &self->in->next->next;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | 
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE |
    TRIGGER_DRAG | TRIGGER_MOVE;

  self->resize = &resize_panel;

  return sucess;
}

void panel_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error panel_set(struct widget *self,int property, glob data) {
  switch (property) {

  case PG_WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,38);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    self->in->next->flags &= SIDEMASK;
    self->in->next->flags |= ((sidet)data);
    return sucess;

  case PG_WP_SIZE:
    if (data<0) data = 0;
    self->in->split = data;
    if (data>0)
      DATA->unrolled = data;
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  default:
    return mkerror(PG_ERRT_BADPARAM,39);

  }
  return sucess;
}

glob panel_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case PG_WP_SIZE:
    return self->in->split;
    
  }
  return 0;
}

void panel_trigger(struct widget *self,long type,union trigparam *param) {
  unsigned long tick;
  g_error e;
  int tmpover;

  switch (type) {

  case TRIGGER_ENTER:

    /* Only set DATA->over if the mouse is in the panelbar */
    if (param->mouse.x < PANELBAR_DIV->x ||
	param->mouse.y < PANELBAR_DIV->y ||
	param->mouse.x >= (PANELBAR_DIV->x+PANELBAR_DIV->w) ||
	param->mouse.y >= (PANELBAR_DIV->y+PANELBAR_DIV->h))
      return;
    DATA->over = 1;
    break;

  case TRIGGER_LEAVE:
    /* If we're dragging, the mouse didn't REALLY leave */
    if (DATA->on) return;

    if (!DATA->over) return;  /* Don't bother redrawing */

    DATA->over=0;
    break;

  case TRIGGER_DOWN:
    if (param->mouse.chbtn != 1) return;

    /* Calculate grab_offset with respect to
       the edge shared by the panel and the
       panel bar. */
    switch (self->in->flags & (~SIDEMASK)) {
    case PG_S_TOP:
      DATA->grab_offset = param->mouse.y - PANELBAR_DIV->y;
      break;
    case PG_S_BOTTOM:
      DATA->grab_offset = PANELBAR_DIV->y + PANELBAR_DIV->h - 1 - param->mouse.y;
      break;
    case PG_S_LEFT:
      DATA->grab_offset = param->mouse.x - PANELBAR_DIV->x;
      break;
    case PG_S_RIGHT:
      DATA->grab_offset = PANELBAR_DIV->x + PANELBAR_DIV->w - 1 - param->mouse.x;
      break;
    }

    /* Ignore if it's not in the panelbar */
    if (DATA->grab_offset<0) return;

    DATA->osplit = self->in->split;
    DATA->on = 1;

    /* Update the screen now, so we have an up-to-date picture
       of the panelbar stored in DATA->bar */
    themeify_panel(self);
    update_nosprite();

    /* Allocate the new sprite */
    if(iserror(new_sprite(&DATA->s,PANELBAR_DIV->w,PANELBAR_DIV->h)))
      return;
    if (iserror((*vid->bitmap_new)(&DATA->s->bitmap,PANELBAR_DIV->w,PANELBAR_DIV->h))) {
      free_sprite(DATA->s);
      return;
    }
    
    /* Grab a bitmap of the panelbar to use as the sprite */
    (*vid->clip_off)();
    (*vid->unblit)(DATA->s->x = PANELBAR_DIV->x,DATA->s->y = PANELBAR_DIV->y,
		   DATA->s->bitmap,0,0,
		   PANELBAR_DIV->w,PANELBAR_DIV->h);
    DATA->s->clip_to = self->in;

    break;

  case TRIGGER_UP:
  case TRIGGER_RELEASE:
    if (!DATA->on) return;
    if (param->mouse.chbtn != 1) return;

    /* Now use grab_offset to calculate a new split value */
    switch (self->in->flags & (~SIDEMASK)) {
    case PG_S_TOP:
      self->in->split =  param->mouse.y - DATA->grab_offset - self->in->y;
      if (self->in->split + PANELBAR_DIV->h > self->in->h)
	self->in->split = self->in->h - PANELBAR_DIV->h;
      break;
    case PG_S_BOTTOM:
      self->in->split = self->in->y + self->in->h - 1 -
	param->mouse.y - DATA->grab_offset;
      if (self->in->split + PANELBAR_DIV->h > self->in->h)
	self->in->split = self->in->h - PANELBAR_DIV->h;
      break;
    case PG_S_LEFT:
      self->in->split =  param->mouse.x - DATA->grab_offset - self->in->x;
      if (self->in->split + PANELBAR_DIV->w > self->in->w)
	self->in->split = self->in->w - PANELBAR_DIV->w;
      break;
    case PG_S_RIGHT:
      self->in->split = self->in->x + self->in->w - 1 -
	param->mouse.x - DATA->grab_offset;
      if (self->in->split + PANELBAR_DIV->w > self->in->w)
	self->in->split = self->in->w - PANELBAR_DIV->w;
      break;
    }
    if (self->in->split < 0) self->in->split = 0;

    if (abs(self->in->split - DATA->osplit) < MINDRAGLEN) {
      /* This was a click, not a drag */

      if (DATA->osplit > 0) {
	/* Roll up the panel */
	self->in->split = 0;

	DATA->over = 0;
      }
      else {
	/* Unroll the panel */
	self->in->split = DATA->unrolled;

	DATA->over = 0;
      }
    }
    else {
      
      /* Save this as the new unrolled split,
       * Unless the user manually rolled up the panel */
      if (self->in->split > MAXROLLUP) 
	DATA->unrolled = self->in->split;
      else
	self->in->split = 0;
    }

    /* Do a recalc, because we just changed everything's size */
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;

    free_sprite(DATA->s);

    DATA->on = 0;
    break;

  case TRIGGER_MOVE:
    /* We're not dragging the bar, but see if the mouse is 
       entering or exiting the bar */
    tmpover = (param->mouse.x >= PANELBAR_DIV->x &&
	       param->mouse.y >= PANELBAR_DIV->y &&
	       param->mouse.x < (PANELBAR_DIV->x+PANELBAR_DIV->w) &&
	       param->mouse.y < (PANELBAR_DIV->y+PANELBAR_DIV->h));
    if (tmpover == DATA->over) return;
    DATA->over = tmpover;
    break;
    
  case TRIGGER_DRAG:
    if (!DATA->on) return;
    /* Ok, button 1 is dragging through our widget... */

    /* If we haven't waited long enough since the last update,
       go away */
    tick = getticks();
    if (tick < DATA->wait_tick) return;
    DATA->wait_tick = tick + DRAG_DELAY;

    /* Determine where to blit the bar to... */
    switch (self->in->flags & (~SIDEMASK)) {
    case PG_S_TOP:
      DATA->s->x = PANELBAR_DIV->x;
      DATA->s->y = param->mouse.y - DATA->grab_offset;
      break;
    case PG_S_BOTTOM:
      DATA->s->x = PANELBAR_DIV->x;
      DATA->s->y = param->mouse.y + DATA->grab_offset - PANELBAR_DIV->h;
      break;
    case PG_S_LEFT:
      DATA->s->y = PANELBAR_DIV->y;
      DATA->s->x = param->mouse.x - DATA->grab_offset;
      break;
    case PG_S_RIGHT:
      DATA->s->y = PANELBAR_DIV->y;
      DATA->s->x = param->mouse.x + DATA->grab_offset - PANELBAR_DIV->w;
      break;
    }

    /* Reposition sprite */
    (*vid->sprite_update)(DATA->s);

    return;

  }

  themeify_panel(self);
}

void themeify_panel(struct widget *self) {
  /* Apply the current state  */
  if (DATA->on)
    div_setstate(self->in->next->div,PGTH_O_PANELBAR_ON);
  else if (DATA->over)
    div_setstate(self->in->next->div,PGTH_O_PANELBAR_HILIGHT);
  else
    div_setstate(self->in->next->div,PGTH_O_PANELBAR);
}

/* The End */




