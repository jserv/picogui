/* $Id: hotspot.h,v 1.2 2001/07/26 10:11:22 micahjd Exp $
 *
 * pgserver/hotspot.h - This is an interface for managing hotspots.
 *                      The divtree is scanned for hotspot divnodes.
 *                      Their position is saved in a graph so it is easy
 *                      to, for example, find which hotspot is to the left
 *                      of a specified hotspot. This makes navigation with
 *                      arrow keys only possible.
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

#ifndef __H_HOTSPOT
#define __H_HOTSPOT

#include <pgserver/divtree.h>

#define HOTSPOT_LEFT  0   /* For arrow keys */
#define HOTSPOT_RIGHT 1
#define HOTSPOT_UP    2
#define HOTSPOT_DOWN  3
#define HOTSPOT_NEXT  4   /* Tab and shift-tab */
#define HOTSPOT_PREV  5

#define HOTSPOTMAX    6

struct hotspot {
  /* Position */
  s16 x,y;

  /* Links for the hotspot graph, indexed by direction */
  struct hotspot *graph[HOTSPOTMAX];

  /* Simple linked list including all hotspots */
  struct hotspot *next;
};

/* Delete all hotspots */
void hotspot_free(void);

/* Add a new hotspot to the list, don't reconfigure graph */
g_error hotspot_add(s16 x, s16 y);

/* Recursively add hotspots for all applicable divnodes 
 * If 'ntb' is non-NULL, only add hotspots outside it.
 */
g_error hotspot_build(struct divnode *n, struct divnode *ntb);

/* Return the closest hotspot to the given position */
struct hotspot *hotspot_closest(s16 x,s16 y);

/* Rebuild the hotspot graph */
void hotspot_graph(void);

/* This is the 'high level' hotspot function that will usually be called
 * by the widget code.
 *
 * It rebuilds the hotspot graph from the current divtree if necessary, finds
 * the hotspot closest to the mouse pointer, traverses the hotspot graph in
 * the indicated direction, and finds the new mouse pointer position.
 *
 * The direction is a HOTSPOT_* constant
 */
void hotspot_traverse(short direction);

#endif /* __H_HOTSPOT */

/* The End */



