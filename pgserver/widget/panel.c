/* $Id: panel.c,v 1.17 2000/08/07 11:09:27 micahjd Exp $
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

/* A shortcut... */
#define PANELBAR_DIV self->in->next->div

struct paneldata {
  int on,over;
  int grab_offset;  /* Difference between side of panel bar
		       and the point it was clicked */
  unsigned long wait_tick;    /* To limit the frame rate */

  struct bitmap *bar,*behindbar;

  /* Location and previous location for the bar */
  int x,y,ox,oy;
};
#define DATA ((struct paneldata *)(self->data))

void panelbar(struct divnode *d) {
  int x,y,w,h;
  x=y=0; w=d->w; h=d->h;

  addelement(d,&current_theme[E_PANELBAR_BORDER],&x,&y,&w,&h);
  addelement(d,&current_theme[E_PANELBAR_FILL],&x,&y,&w,&h);

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
  if (e.type != ERRT_NONE) return e;
  memset(self->data,0,sizeof(struct paneldata));

  /* This split determines the size of the main panel area */
  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_TOP;

  /* This draws the panel background and provides a border */
  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->split = PANELMARGIN;
  self->in->div->on_recalc = &panel;

  /* Split off another chunk of space for the bar */
  e = newdiv(&self->in->next,self);
  if (e.type != ERRT_NONE) return e;
  self->in->next->flags |= S_TOP;
  self->in->next->split = PANELBAR_SIZE;

  /* And finally, the divnode that draws the panelbar */
  e = newdiv(&self->in->next->div,self);
  self->in->next->div->on_recalc = &panelbar;

  self->sub = &self->in->div->div;
  self->out = &self->in->next->next;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | 
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_DRAG;

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
    if (!VALID_SIDE(data)) return mkerror(ERRT_BADPARAM,
	"WP_SIDE param is not a valid side value (panel)");
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    self->in->next->flags &= SIDEMASK;
    self->in->next->flags |= ((sidet)data);
    return sucess;

  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for panel");

  }
  return sucess;
}

glob panel_get(struct widget *self,int property) {
  switch (property) {

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);
    
  }
  return 0;
}

void panel_trigger(struct widget *self,long type,union trigparam *param) {
  unsigned long tick;
  g_error e;

  switch (type) {

  case TRIGGER_ENTER:
    DATA->over = 1;
    break;

  case TRIGGER_LEAVE:
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

    DATA->on = 1;

    /* Lock the screen */
    dts_push();

    /* Create a bitmap for the panelbar, and for
       the stuff behind it */
    DATA->bar = DATA->behindbar = NULL;
    hwrbit_new(&DATA->bar,PANELBAR_DIV->w,PANELBAR_DIV->h);
    hwrbit_new(&DATA->behindbar,PANELBAR_DIV->w,PANELBAR_DIV->h);

    /* Grab a bitmap of the panelbar */
    hwr_blit(NULL,LGOP_NONE,NULL,PANELBAR_DIV->x,
	     PANELBAR_DIV->y,DATA->bar,0,0,
	     PANELBAR_DIV->w,PANELBAR_DIV->h);

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
    
    /* Do a recalc, because we just changed everything's size */
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;

    /* Unlock it */
    dts_pop();

    hwrbit_free(DATA->bar);
    hwrbit_free(DATA->behindbar);

    DATA->on = 0;
    break;

  case TRIGGER_DRAG:
    if (!DATA->on) return;    
    /* Ok, button 1 is dragging through our widget... */

    /* If we haven't waited long enough since the last update,
       go away */
    tick = getticks();
    if (tick < DATA->wait_tick) break;
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
      hwr_blit(NULL,LGOP_NONE,DATA->behindbar,0,0,NULL,
	       DATA->ox,DATA->oy,PANELBAR_DIV->w,PANELBAR_DIV->h);
    
    /* Grab a new one */
    DATA->ox = DATA->x; DATA->oy = DATA->y;
    hwr_blit(NULL,LGOP_NONE,NULL,DATA->ox,DATA->oy,DATA->behindbar,
	     0,0,PANELBAR_DIV->w,PANELBAR_DIV->h);

    /* Do a Bit Block Transfer (tm)   :-)  */
    hwr_blit(NULL,LGOP_NONE,DATA->bar,0,0,NULL,
	     DATA->x,DATA->y,PANELBAR_DIV->w,PANELBAR_DIV->h);

    /* Because we have this divtree to ourselves, do
     * the hwr_update() directly. */
    hwr_update();

    return;

  }

  /* Use the Power of the almighty update() to redraw the screen! */
  update();
}

/* The End */




