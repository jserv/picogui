/* $Id: panel.c,v 1.15 2000/08/06 20:35:10 micahjd Exp $
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

#define PANELBAR_SIZE 15

#define DRAG_DELAY    20   /* Min. # of milliseconds between
			      updates while dragging */

struct paneldata {
  int on,over;
  int grab_offset;  /* Difference between side of panel bar
		       and the point it was clicked */
  unsigned long wait_tick;    /* To limit the frame rate */
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

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_TOP;

  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &panel;

  e = newdiv(&self->in->next,self);
  if (e.type != ERRT_NONE) return e;
  self->in->next->on_recalc = &panelbar;
  self->in->next->flags |= S_TOP;
  self->in->next->split = PANELBAR_SIZE;

  self->sub = &self->in->next->div;
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
      DATA->grab_offset = param->mouse.y - self->in->next->y;
      break;
    case S_BOTTOM:
      DATA->grab_offset = self->in->next->y - param->mouse.y;
      break;
    case S_LEFT:
      DATA->grab_offset = param->mouse.x - self->in->next->x;
      break;
    case S_RIGHT:
      DATA->grab_offset = self->in->next->x - param->mouse.x;
      break;
    }

    /* Ignore if it's not in the panelbar */
    if (DATA->grab_offset<0) return;

    DATA->on = 1;
    break;

  case TRIGGER_UP:
  case TRIGGER_RELEASE:
    if (param->mouse.chbtn != 1) return;
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

    /* Now use grab_offset to calculate a new split value */
    switch (self->in->flags & (~SIDEMASK)) {
    case S_TOP:
      self->in->split =  param->mouse.y - DATA->grab_offset - self->in->y;
      break;
    case S_BOTTOM:
      self->in->split = self->in->y + self->in->h - param->mouse.y - DATA->grab_offset;
      break;
    case S_LEFT:
      self->in->split =  param->mouse.x - DATA->grab_offset - self->in->x;
      break;
    case S_RIGHT:
      self->in->split = self->in->x + self->in->w - param->mouse.x - DATA->grab_offset;
      break;
    }
    if (self->in->split < 0) self->in->split = 0;

    /* Do a recalc, because we just changed everything's size */
    self->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  }

  /* Use the Power of the almighty update() to redraw the screen! */
  update();
}

/* The End */




