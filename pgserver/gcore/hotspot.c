/* $Id: hotspot.c,v 1.3 2001/07/11 07:38:20 micahjd Exp $
 *
 * hotspot.c - This is an interface for managing hotspots.
 *             The divtree is scanned for hotspot divnodes.
 *             Their position is saved in a graph so it is easy
 *             to, for example, find which hotspot is to the left
 *             of a specified hotspot. This makes navigation with
 *             arrow keys only possible.
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

#include <pgserver/common.h>
#include <pgserver/divtree.h>
#include <pgserver/appmgr.h>
#include <pgserver/hotspot.h>
#include <pgserver/widget.h>
#include <pgserver/video.h>

struct hotspot *hotspotlist;

/******************** Private functions */

/* Sorts first by Y, then by X.
 *
 * Returns nonzero if a > b
 */
int hotspot_compare(struct hotspot *a, struct hotspot *b) {
  if (a->y == b->y) {
    if (a->x > b->x)
      return 1;
    else
      return 0;
  }
  else {
    if (a->y > b->y)
      return 1;
    else
      return 0;
  }
}

/******************** Public functions */

/* Delete all hotspots */
void hotspot_free(void) {
  struct hotspot *condemn;
  while (hotspotlist) {
    condemn = hotspotlist;
    hotspotlist = hotspotlist->next;
    g_free(condemn);
  }
}

/* Add a new hotspot to the list with an insertion sort */
g_error hotspot_add(s16 x, s16 y) {
  struct hotspot *newspot;
  g_error e;
  struct hotspot **where;

  /* Create the new node */
  e = g_malloc((void**) &newspot, sizeof(struct hotspot));
  errorcheck;
  memset(newspot,0,sizeof(struct hotspot));
  newspot->x = x;
  newspot->y = y;
  
  /* Figure out where to add the node */
  where = &hotspotlist;
  while ((*where) && hotspot_compare(newspot,*where))
    where = &((*where)->next);

  /* Add it */
  newspot->next = *where;
  *where = newspot;
  return sucess;
}

/* Recursively add hotspots for all applicable divnodes */
g_error hotspot_build(struct divnode *n) {
  s16 x,y;
  g_error e;

  if (!n) return sucess;
  
  /* shall this be a hotspot? */
  if ((n->flags & DIVNODE_HOTSPOT) && n->w>0 && n->h>0) {

    /* Find a good place within the node for the hotspot */
    x = n->x + n->w - 8;
    y = n->y + n->h - 8;
    
    if (x<n->x)
      x = n->x;
    if (y<n->y)
      y = n->y;

    e = hotspot_add(x,y);
    errorcheck;
  }

  /* Recursively add all divnodes */
  e = hotspot_build(n->div);
  errorcheck;
  e = hotspot_build(n->next);
  errorcheck;
  return sucess;
}


/* Return the closest hotspot to the given position */
struct hotspot *hotspot_closest(s16 x,s16 y) {
  struct hotspot *closespot = NULL;
  u32 closedist = 0xFFFFFFFF;
  u32 dist;
  s32 xd,yd;
  struct hotspot *n;
  
  for (n=hotspotlist;n;n=n->next) {
    xd = n->x - x;
    yd = n->y - y;
    dist = xd*xd + yd*yd;

    if (dist<closedist) {
      closedist = dist;
      closespot = n;
    }
  }
  return closespot;
}

/* Rebuild the hotspot graph */
void hotspot_graph(void) {
  struct hotspot *p,*n;

  for (p=hotspotlist;p;p=p->next) {
   
    /* Link previous and next */
    if (p->next) {
      p->graph[HOTSPOT_NEXT] = p->next;
      p->next->graph[HOTSPOT_PREV] = p;
    }
    else {
      p->graph[HOTSPOT_NEXT] = hotspotlist;
      hotspotlist->graph[HOTSPOT_PREV] = p;
    }

    /* Link left and right */
    if (p->next && p->y == p->next->y) {
      p->graph[HOTSPOT_RIGHT] = p->next;
      p->next->graph[HOTSPOT_LEFT] = p;
    }

    /* Link down.
     * Find the beginning of the next row, then find the closest
     * widget in that row.
     */
    n = p;
    while (n && n->y==p->y)
      n = n->next;
    if (n) {
      u16 dist = 0xFFFF;
      struct hotspot *closest = NULL;
      s16 row_y = n->y;
      s16 d;
      while (n && n->y==row_y) {
	d = n->x - p->x;
	if (d<0)
	  d = -d;
	if (d<dist) {
	  dist = d;
	  closest = n;
	}
	n = n->next;
      }
      p->graph[HOTSPOT_DOWN] = closest;
    }
  }

  /* Linking up requires that the HOTSPOT_PREV link is already done,
   * so do it in a separate pass. */

  for (p=hotspotlist;p;p=p->next) {

    /* Link up.
     * Find the beginning of the previous row, then find the closest
     * widget in that row.
     */
    n = p;
    while (n && n->y==p->y)
      n = (n == hotspotlist) ? NULL : n->graph[HOTSPOT_PREV];
    if (n) {
      u16 dist = 0xFFFF;
      struct hotspot *closest = NULL;
      s16 row_y = n->y;
      s16 d;
      while (n && n->y==row_y) {
	d = n->x - p->x;
	if (d<0)
	  d = -d;
	if (d<dist) {
	  dist = d;
	  closest = n;
	}
	n = (n == hotspotlist) ? NULL : n->graph[HOTSPOT_PREV];
      }
      p->graph[HOTSPOT_UP] = closest;
    }

  }
}

/* This is the 'high level' hotspot function that will usually be called
 * by the widget code.
 *
 * It rebuilds the hotspot graph from the current divtree if necessary, finds
 * the hotspot closest to the mouse pointer, traverses the hotspot graph in
 * the indicated direction, and finds the new mouse pointer position.
 *
 * The direction is a HOTSPOT_* constant
 */
void hotspot_traverse(short direction) {
  struct hotspot *p;

  /* rebuild the graph */
  if (!hotspotlist) {
    hotspot_build(dts->top->head);
    hotspot_graph();
  }

  /* Find the current node */
  p = hotspot_closest(cursor->x,cursor->y);
  if (!p)
    return;

  /* If the closest node isn't really that close, just warp to
   * that closest node. Otherwise, traverse. */
  if ( (cursor->x <= p->x+5) &&
       (cursor->x >= p->x-5) &&
       (cursor->y <= p->y+5) &&
       (cursor->y >= p->y-5) ) {
  
    /* traverse */
    p = p->graph[direction];
    if (!p)
      return;
  }

  /* move the cursor */
  dispatch_pointing(PG_TRIGGER_MOVE,p->x,p->y,0);

  /* focus the widget under the cursor*/
  if (under)
    request_focus(under);

  drivermessage(PGDM_CURSORVISIBLE,1);
}

/* The End */







