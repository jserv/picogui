/* $Id: theme.c,v 1.1 2000/04/29 07:29:02 micahjd Exp $
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

#include <theme.h>

/* This is initialized to the default theme */
#if 0
struct theme current_theme = {

  /* Button */ {

    /* border */ {
      1,1,-2,-2,
      ELEM_FLAT,
      {
	{black,black,0,0},
	{black,black,0,0},
	{black,black,0,0}
      }
    },

    /* fill */ {
      0,0,0,0,
      ELEM_FLAT,
      {
	{ltgray,black,0,0},
	{ltgray,black,0,0},
	{0x8080FF,black,0,0}
      }
    },

    /* overlay */ {
      1,1,-2,-2,
      ELEM_FLAT,
      {
	{gray,black,0,0},
	{0xFFFF00,black,0,0},
	{0xFFFF00,black,0,0}
      }
    }

  }

};
#endif

struct theme current_theme = {

  /* Button */ {

    /* border */ {
      2,2,-4,-4,
      ELEM_GRADIENT,
      {
	{0x7080A0,black,45,0},
	{0x7080A0,black,45,0},
	{0x7080A0,black,225,0},
      }
    },

    /* fill */ {
      0,0,0,0,
      ELEM_GRADIENT,
      {
	{0x8090B0,black,240,0},
	{0x8090B0,black,240,0},
	{0x8090B0,black,240,0},
      }
    },

    /* overlay */ {
      0,0,0,0,
      ELEM_GRADIENT,
      {
	{black,black,270,1},
	{0x505000,black,270,1},
	{0x505000,black,270,1},
      }
    }

  }

};


/* Creates a gropnode representing an element.
 * Depending on the type and the size, create a gradient, rectangle,
 * slab, bar, or frame.
 */
void addelement(struct gropnode **headpp,struct element *el,
		int *x,int *y,int *w,int *h,int setp) {
  
  if (el->type == ELEM_GRADIENT) {
    grop_gradient(headpp,*x,*y,*w,*h,el->state[STATE_NORMAL].c1,
		  el->state[STATE_NORMAL].c2,el->state[STATE_NORMAL].angle,
		  el->state[STATE_NORMAL].translucent);
  }
  else if (el->type == ELEM_FLAT) {
    if (el->w==0 && el->h==-1)
      grop_slab(headpp,*x,*y,*w,el->state[STATE_NORMAL].c1);
    else if (el->w==-1 && el->h==0)
      grop_bar(headpp,*x,*y,*h,el->state[STATE_NORMAL].c1);
    else if (el->w==-2 && el->h==-2)
      grop_frame(headpp,*x,*y,*w,*h,el->state[STATE_NORMAL].c1);
    else
      grop_rect(headpp,*x,*y,*w,*h,el->state[STATE_NORMAL].c1);
  }
  else
    grop_null(headpp);

  if (setp) {
    *x += el->x;
    *y += el->y;
    *w += el->w;
    *h += el->h;
  }
}

/* Apply a state to an existing gropnode */
void applystate(struct gropnode *n,struct element *el,int state) {
  /* gradient.c1 occupies the same memory as c */
  n->param.gradient.c1 = el->state[state].c1;
  n->param.gradient.c2 = el->state[state].c2;
  n->param.gradient.angle = el->state[state].angle;
  n->param.gradient.translucent = el->state[state].translucent;
}

/* The End */




