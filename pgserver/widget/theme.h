/* $Id: theme.h,v 1.2 2000/04/29 17:51:59 micahjd Exp $
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

#ifndef __THEME_H
#define __THEME_H

#include <divtree.h>
#include <g_error.h>

/* An element translates to both a gropnode, and a change in the
   current x,y,w,h. A permutation defines how the element is changed
   under different conditions.
*/

/* Types applicable to elements */
#define ELEM_NULL       0
#define ELEM_FLAT       1   /* Uses c1. Depending on x,y,w,h it can be
			       a rectangle, slab, bar, or frame. */
#define ELEM_GRADIENT   2   /* Uses c1,c2,angle,translucent */

/* Element states */
#define STATE_NORMAL    0
#define STATE_HILIGHT   1
#define STATE_ACTIVATE  2

struct element {
  /* Delta values from the divnode's size */
  int x,y,w,h;

  int type;

  struct {
    devcolort c1,c2;
    int angle,translucent;
  } state[3];
};

/* The theme structure (new elements must be added at the end) */

#define E_BUTTON_BORDER     0
#define E_BUTTON_FILL       1
#define E_BUTTON_OVERLAY    2
#define E_TOOLBAR_BORDER    3
#define E_TOOLBAR_FILL      4

#define E_NUM 5

extern struct element current_theme[E_NUM];

/**** Functions for themes */

/* Creates a gropnode representing an element 
 * If setp is nonzero, then x,y,w,h are updated. */
void addelement(struct gropnode **headpp,struct element *el,
		int *x,int *y,int *w,int *h,int setp);

/* Apply a state to an existing gropnode */
void applystate(struct gropnode *n,struct element *el,int state);

#endif /* __WIDGET_H */

/* The End */




