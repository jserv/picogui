/* $Id: theme.h,v 1.1 2000/09/03 19:27:59 micahjd Exp $
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

#include <pgserver/divtree.h>
#include <pgserver/g_error.h>

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
#define STATE_ALL       255
#define STATE_NORMAL    0
#define STATE_HILIGHT   1
#define STATE_ACTIVATE  2

#define STATE_NUM       3

struct element {

  /* If this is 0,the element is solid.  A positive value indicates that
     it forms a border of this width on each side.
     A negative value forms a border only on the exposed side (the
     one opposite the widget's 'side')
  */
  signed char width;

  unsigned char type;

  struct {
    hwrcolor c1,c2;
    int angle;
    signed char translucent;
  } state[3];
};

/* The theme structure (new elements must be added at the end) */

#define E_BUTTON_BORDER     0
#define E_BUTTON_FILL       1
#define E_BUTTON_OVERLAY    2
#define E_TOOLBAR_BORDER    3
#define E_TOOLBAR_FILL      4
#define E_SCROLLBAR_BORDER  5
#define E_SCROLLBAR_FILL    6
#define E_SCROLLIND_BORDER  7
#define E_SCROLLIND_FILL    8
#define E_SCROLLIND_OVERLAY 9
#define E_INDICATOR_BORDER  10
#define E_INDICATOR_FILL    11
#define E_INDICATOR_OVERLAY 12
#define E_PANEL_BORDER      13
#define E_PANEL_FILL        14
#define E_PANELBAR_BORDER   15
#define E_PANELBAR_FILL     16
#define E_POPUP_BORDER      17
#define E_POPUP_FILL        18

#define E_NUM 19

#define EPARAM_WIDTH        1
#define EPARAM_TYPE         2
#define EPARAM_C1           3
#define EPARAM_C2           4
#define EPARAM_ANGLE        5
#define EPARAM_TRANSLUCENT  6

extern struct element default_theme[E_NUM];
extern struct element current_theme[E_NUM];

/**** Functions for themes */

/* Creates a gropnode representing an element 
 * If setp is nonzero, then x,y,w,h are updated. */
void addelement(struct divnode *d,struct element *el,
		int *x,int *y,int *w,int *h);

/* Apply a state to an existing gropnode */
void applystate(struct gropnode *n,struct element *el,int state);

/* This is how theme elements are stored in a portable way */
void themeset(int element,int state,int param,unsigned long value);

/* Restore all theme defaults */
void restoretheme(void);

#endif /* __WIDGET_H */

/* The End */




