/* $Id: panel.c,v 1.26 2000/08/27 05:54:28 micahjd Exp $
 *
 * panel.c - Holder for applications
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <widget.h>
#include <divtree.h>
#include <g_malloc.h>
#include <theme.h>
#include <timer.h>

#define PANELMARGIN (HWG_MARGIN<<1)

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

void themeify_panel(struct divnode *d);

struct paneldata {
  int on,over;
  int grab_offset;  /* Difference between side of panel bar
		       and the point it was clicked */
  unsigned long wait_tick;    /* To limit the frame rate */

  hwrbitmap bar,behindbar;

  /* Saved split value before a drag, used to
     calculate whether it should be interpreted as
     a click */
  int osplit;

  /* The split value while the panel is unrolled */
  int unrolled;

  /* Location and previous location for the bar */
  int x,y,ox,oy;
};
#define DATA ((struct paneldata *)(self->data))

void panelbar(struct divnode *d) {
  int x,y,w,h;
  x=y=0; w=d->w; h=d->h;

  addelement(d,&current_theme[E_PANELBAR_BORDER],&x,&y,&w,&h);
  addelement(d,&current_theme[E_PANELBAR_FILL],&x,&y,&w,&h);

  themeify_panel(d);
}

void panel(struct divnode *d) {
  int x,y,w,h;
  x=y=0; w=d->w; h=d->h;

  addelement(d,&current_theme[E_PANEL_BORDER],&x,&y,&w,&h);
  addelement(d,&current_theme[E_PANEL_FILL],&x,&y,&w,&h);
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
  self->in->flags |= S_TOP;

  /* This draws the panel background and provides a border */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->split = PANELMARGIN;
  self->in->div->on_recalc = &panel;

  /* Split off another chunk of space for the bar */
  e = newdiv(&self->in->next,self);
  errorcheck;
  self->in->next->flags |= S_TOP;
  self->in->next->split = PANELBAR_SIZE;

  /* And finally, the divnode that draws the panelbar */
  e = newdiv(&self->in->next->div,self);
  self->in->next->div->on_recalc = &panelbar;

  self->sub = &self->in->div->div;
  self->out = &self->in->next->next;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | 
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE |
    TRIGGER_DRAG | TRIGGER_MOVE;

  return sucess;
}

void panel_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error panel_set(struct widget *self,int property, glob data) {
  switch (property) {

  case WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(ERRT_BADPARAM,38);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    self->in->next->flags &= SIDEMASK;
    self->in->next->flags |= ((sidet)data);
    return sucess;

  case WP_SIZE:
    if (data<0) data = 0;
    self->in->split = data;
    if (data>0)
      DATA->unrolled = data;
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  default:
    return mkerror(ERRT_BADPARAM,39);

  }
  return sucess;
}

glob panel_get(struct widget *self,int property) {
  switch (property) {

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_SIZE:
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
    case S_TOP:
      DATA->grab_offset = param->mouse.y - PANELBAR_DIV->y;
      break;
    case S_BOTTOM:
      DATA->grab_offset = PANELBAR_DIV->y + PANELBAR_DIV->h - 1 - param->mouse.y;
      break;
    case S_LEFT:
      DATA->grab_offset = param->mouse.x - PANELBAR_DIV->x;
      break;
    case S_RIGHT:
      DATA->grab_offset = PANELBAR_DIV->x + PANELBAR_DIV->w - 1 - param->mouse.x;
      break;
    }

    /* Ignore if it's not in the panelbar */
    if (DATA->grab_offset<0) return;

    DATA->osplit = self->in->split;
    DATA->on = 1;

    /* Update the screen now, so we have an up-to-date picture
       of the panelbar stored in DATA->bar */
    themeify_panel(PANELBAR_DIV);
    update();

    /* Lock the screen */
    dts_push();
    (*vid->clip_off)();

    /* Create a bitmap for the panelbar, and for
       the stuff behind it */
    DATA->bar = DATA->behindbar = NULL;
    (*vid->bitmap_new)(&DATA->bar,PANELBAR_DIV->w,PANELBAR_DIV->h);
    (*vid->bitmap_new)(&DATA->behindbar,PANELBAR_DIV->w,PANELBAR_DIV->h);

    /* Grab a bitmap of the panelbar */
    (*vid->blit)(NULL,PANELBAR_DIV->x,
		 PANELBAR_DIV->y,DATA->bar,0,0,
		 PANELBAR_DIV->w,PANELBAR_DIV->h,LGOP_NONE);

    /* Reset ox and oy */
    DATA->ox = DATA->oy = -1;

    break;

  case TRIGGER_UP:
  case TRIGGER_RELEASE:
    if (!DATA->on) return;
    if (param->mouse.chbtn != 1) return;

    /* Now use grab_offset to calculate a new split value */
    switch (self->in->flags & (~SIDEMASK)) {
    case S_TOP:
      self->in->split =  param->mouse.y - DATA->grab_offset - self->in->y;
      if (self->in->split + PANELBAR_DIV->h > self->in->h)
	self->in->split = self->in->h - PANELBAR_DIV->h;
      break;
    case S_BOTTOM:
      self->in->split = self->in->y + self->in->h - 1 -
	param->mouse.y - DATA->grab_offset;
      if (self->in->split + PANELBAR_DIV->h > self->in->h)
	self->in->split = self->in->h - PANELBAR_DIV->h;
      break;
    case S_LEFT:
      self->in->split =  param->mouse.x - DATA->grab_offset - self->in->x;
      if (self->in->split + PANELBAR_DIV->w > self->in->w)
	self->in->split = self->in->w - PANELBAR_DIV->w;
      break;
    case S_RIGHT:
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

    /* Unlock it */
    dts_pop();

    (*vid->bitmap_free)(DATA->bar);
    (*vid->bitmap_free)(DATA->behindbar);

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
    case S_TOP:
      DATA->x = PANELBAR_DIV->x;
      DATA->y = param->mouse.y - DATA->grab_offset;
      break;
    case S_BOTTOM:
      DATA->x = PANELBAR_DIV->x;
      DATA->y = param->mouse.y + DATA->grab_offset - PANELBAR_DIV->h;
      break;
    case S_LEFT:
      DATA->y = PANELBAR_DIV->y;
      DATA->x = param->mouse.x - DATA->grab_offset;
      break;
    case S_RIGHT:
      DATA->y = PANELBAR_DIV->y;
      DATA->x = param->mouse.x + DATA->grab_offset - PANELBAR_DIV->w;
      break;
    }
    /* Clippin' stuff... Prevent segfaults and missing panelbars */
    if (DATA->x < self->in->x) DATA->x = self->in->x;
    if (DATA->y < self->in->y) DATA->y = self->in->y;
    if (DATA->x+PANELBAR_DIV->w > self->in->x+self->in->w)
      DATA->x = self->in->x + self->in->w - PANELBAR_DIV->w;
    if (DATA->y+PANELBAR_DIV->h > self->in->y+self->in->h)
      DATA->y = self->in->y + self->in->h - PANELBAR_DIV->h;

    /* Put back the old image */
    if (DATA->ox != -1)
      (*vid->blit)(DATA->behindbar,0,0,NULL,
		   DATA->ox,DATA->oy,PANELBAR_DIV->w,
		   PANELBAR_DIV->h,LGOP_NONE);
    
    /* Grab a new one */
    DATA->ox = DATA->x; DATA->oy = DATA->y;
    (*vid->blit)(NULL,DATA->ox,DATA->oy,DATA->behindbar,
		 0,0,PANELBAR_DIV->w,PANELBAR_DIV->h,
		 LGOP_NONE);

    /* Do a Bit Block Transfer (tm)   :-)  */
    (*vid->blit)(DATA->bar,0,0,NULL,
		 DATA->x,DATA->y,PANELBAR_DIV->w,
		 PANELBAR_DIV->h,LGOP_NONE);

    /* Because we have this divtree to ourselves, do
     * the hwr_update() directly. */
    (*vid->update)();

    return;

  }

  themeify_panel(PANELBAR_DIV);

  /* Use the Power of the almighty update() to redraw the screen! */
  update();
}

void themeify_panel(struct divnode *d) {
  int state;
  struct widget *self = d->owner;

  /* Apply the current state to the elements */
  if (DATA->on)
    state = STATE_ACTIVATE;
  else if (DATA->over)
    state = STATE_HILIGHT;
  else
    state = STATE_NORMAL;

  applystate(d->grop,
	     &current_theme[E_PANELBAR_BORDER],state);
  applystate(d->grop->next,
	     &current_theme[E_PANELBAR_FILL],state);

  /* Rotate the gradient on the panelbar
     depending on the side it is attached to */
  switch (self->in->flags & (~SIDEMASK)) {
  case S_RIGHT:
    d->grop->param.gradient.angle += 90;
    d->grop->next->param.gradient.angle += 90;
    break;
  case S_BOTTOM:
    d->grop->param.gradient.angle += 180;
    d->grop->next->param.gradient.angle += 180;
    break;
  case S_LEFT:
    d->grop->param.gradient.angle += 270;
    d->grop->next->param.gradient.angle += 270;
    break;
  }


  /* Redraw this node only */
  d->flags |= DIVNODE_NEED_REDRAW;
  self->dt->flags |= DIVTREE_NEED_REDRAW;   
}

/* The End */




