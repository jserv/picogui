/* $Id: theme.h,v 1.3 2000/10/10 00:33:37 micahjd Exp $
 *
 * theme.h - This defines the structures and functions for themes,
 * parameters defining the way widgets are drawn that are reconfigurable
 * at runtime.
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

#ifndef __THEME_H
#define __THEME_H

#include <picogui/constants.h>

#include <pgserver/divtree.h>
#include <pgserver/g_error.h>

/* An element translates to both a gropnode, and a change in the
   current x,y,w,h. A permutation defines how the element is changed
   under different conditions.
*/

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

extern struct element default_theme[PG_E_NUM];
extern struct element current_theme[PG_E_NUM];

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




