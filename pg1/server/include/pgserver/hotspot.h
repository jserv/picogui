/* $Id$
 *
 * pgserver/hotspot.h - This is an interface for managing hotspots.
 *                      The divtree is scanned for hotspot divnodes.
 *                      Their position is saved in a graph so it is easy
 *                      to, for example, find which hotspot is to the left
 *                      of a specified hotspot. This makes navigation with
 *                      arrow keys only possible.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
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

#define HOTSPOTNUM    6

struct hotspot {
  /* Position */
  s16 x,y;

  /* If this hotspot is attached to a specific divnode, this should be
   * a pointer to that divnode. This allows focusing and scrolling to work
   * automatically, but is not required.
   */
  struct divnode *div;

  /* Links for the hotspot graph, indexed by direction */
  struct hotspot *graph[HOTSPOTNUM];

  /* Simple linked list including all hotspots */
  struct hotspot *next;
};

/* Delete all hotspots */
void hotspot_free(void);

/* Hide the hotspot navigation cursor */
void hotspot_hide(void);

/* Add a new hotspot to the list, don't reconfigure graph */
g_error hotspot_add(s16 x, s16 y, struct divnode *div);

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

/* Given a divnode, this will scroll the divnode's container so that
 * the divnode is completely visible.
 */
void scroll_to_divnode(struct divnode *div);

/* Return a preferred position for a hotspot within the specified divnode */
void divnode_hotspot_position(struct divnode *div, int *hx, int *hy);

/* Reloads global hotkey settings from the theme when it changes */
void reload_hotkeys(void);
extern u16 hotkey_left, hotkey_right, hotkey_up, hotkey_down;
extern u16 hotkey_activate, hotkey_next;

#endif /* __H_HOTSPOT */

/* The End */



