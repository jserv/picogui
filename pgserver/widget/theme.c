/* $Id: theme.c,v 1.15 2000/09/23 06:11:55 micahjd Exp $
 *
 * theme.h - This defines the structures and functions for themes,
 * parameters defining the way widgets are drawn that are reconfigurable
 * at runtime.
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

#include <pgserver/theme.h>
#include <pgserver/divtree.h>
#include <pgserver/widget.h>

struct element current_theme[PG_E_NUM];

/* Creates a gropnode representing an element.
 * Depending on the type and the size, create a gradient, rectangle,
 * slab, bar, or frame.
 */
void addelement(struct divnode *d,struct element *el,
		int *x,int *y,int *w,int *h) {
  
  if (el->type == PG_ELEM_GRADIENT) {
    grop_gradient(&d->grop,*x,*y,*w,*h,el->state[PG_STATE_NORMAL].c1,
		  el->state[PG_STATE_NORMAL].c2,el->state[PG_STATE_NORMAL].angle,
		  el->state[PG_STATE_NORMAL].translucent);
  }
  else if (el->type == PG_ELEM_FLAT) {
    if (el->width==1)
      grop_frame(&d->grop,*x,*y,*w,*h,el->state[PG_STATE_NORMAL].c1);
    else
      grop_rect(&d->grop,*x,*y,*w,*h,el->state[PG_STATE_NORMAL].c1);
  }
  else
    grop_null(&d->grop);

  if (el->width > 0) {
    *x += el->width;
    *y += el->width;
    *w -= el->width <<1;
    *h -= el->width <<1;
  }
  else if (el->width < 0) {
    switch (d->owner->in->flags & (~SIDEMASK)) {

    case PG_S_LEFT:
      *w += el->width;
      break;

    case PG_S_RIGHT:
      *w += el->width;
      *x -= el->width;
      break;

    case PG_S_TOP:
      *h += el->width;
      break;

    case PG_S_BOTTOM:
      *h += el->width;
      *y -= el->width;
      break;

    }
  }
}

/* Apply a state to an existing gropnode */
void applystate(struct gropnode *n,struct element *el,int state) {
  /* gradient.c1 occupies the same memory as c */
  n->param.gradient.c1 = (*vid->color_pgtohwr)(el->state[state].c1);
  n->param.gradient.c2 = (*vid->color_pgtohwr)(el->state[state].c2);
  n->param.gradient.angle = el->state[state].angle;
  n->param.gradient.translucent = el->state[state].translucent;
}

void themeset(int element,int state,int param,unsigned long value) {
  struct element *el;
  if ((element<0) || (element>=PG_E_NUM)) return;

  /* With PG_STATE_ALL, call ourselves for all states */
  if (state == PG_STATE_ALL) {
    int i;
    for (i=0;i<PG_STATENUM;i++)
      themeset(element,i,param,value);
    return;
  }

  el = &current_theme[element];
  switch (param) {
  case PG_EPARAM_WIDTH:
    el->width = value;
    break;
  case PG_EPARAM_TYPE:
    el->type = value;
    break;
  case PG_EPARAM_C1:
    if ((state<PG_STATENUM) && (state>=0))
      el->state[state].c1 = value;
    break;
  case PG_EPARAM_C2:
    if ((state<PG_STATENUM) && (state>=0))
      el->state[state].c2 = value;
    break;
  case PG_EPARAM_ANGLE:
    if ((state<PG_STATENUM) && (state>=0))
      el->state[state].angle = value;
    break;
  case PG_EPARAM_TRANSLUCENT:
    if ((state<PG_STATENUM) && (state>=0))
      el->state[state].translucent = value;
    break;
  }  
}

/* Restore defaults */
void restoretheme(void) {
  memcpy(&current_theme,&default_theme,sizeof(struct element)*PG_E_NUM);
  appmgr_setbg(-1,0);
}

/* The End */






