/* $Id: hotspot.c,v 1.23 2002/02/05 00:17:55 micahjd Exp $
 *
 * hotspot.c - This is an interface for managing hotspots.
 *             The divtree is scanned for hotspot divnodes.
 *             Their position is saved in a graph so it is easy
 *             to, for example, find which hotspot is to the left
 *             of a specified hotspot. This makes navigation with
 *             only arrow keys possible.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <string.h>
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
    if (a->x == b->x) {

      /* These two hotspots are at the same spot. This will happen
       * at the edges of a scrolling container. By always returning
       * 1 (and this placing them in order of insertion) it should
       * make things work correctly for vertical lists at least.
       */
      return 1;
    
    }
    else if (a->x > b->x)
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
g_error hotspot_add(s16 x, s16 y, struct divnode *div) {
  struct hotspot *newspot;
  g_error e;
  struct hotspot **where;

  /* Create the new node */
  e = g_malloc((void**) &newspot, sizeof(struct hotspot));
  errorcheck;
  memset(newspot,0,sizeof(struct hotspot));
  newspot->x = x;
  newspot->y = y;
  newspot->div = div;
  
  /* Figure out where to add the node */
  where = &hotspotlist;
  while ((*where) && hotspot_compare(newspot,*where))
    where = &((*where)->next);

  /* Add it */
  newspot->next = *where;
  *where = newspot;
  return success;
}

/* Recursively add hotspots for all applicable divnodes */
g_error hotspot_build(struct divnode *n, struct divnode *ntb) {
  s16 x,y;
  g_error e;

  if (!n) return success;
  
  /* shall this be a hotspot? */
  if ((n->flags & DIVNODE_HOTSPOT) && n->w>0 && n->h>0 &&
      ((!ntb) || n->x < ntb->x || n->y < ntb->y ||
       n->x >= ntb->x+ntb->w || n->y >= ntb->y+ntb->h) &&
      ((!n->divscroll) || (n->divscroll->calcw && n->divscroll->calch))) {

    /* Find a good place within the node for the hotspot */
    divnode_hotspot_position(n,&x,&y);

    /* Scrolled? Clip to the scrolling container. */
    if (n->divscroll) {
      if (x < n->divscroll->calcx) x = n->divscroll->calcx;
      if (y < n->divscroll->calcy) y = n->divscroll->calcy;
      if (x > n->divscroll->calcx + n->divscroll->calcw - 1)
	x = n->divscroll->calcx + n->divscroll->calcw - 1;
      if (y > n->divscroll->calcy + n->divscroll->calch - 1)
	y = n->divscroll->calcy + n->divscroll->calch - 1;
    }

    /* Give the hotspot both an actual hotspot position and the divnode */
    e = hotspot_add(x,y,n);
    errorcheck;
  }

  /* Recursively add all divnodes */
  e = hotspot_build(n->div,ntb);
  errorcheck;
  e = hotspot_build(n->next,ntb);
  errorcheck;
  return success;
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
    hotspot_build(dts->top->head,NULL);
    if (popup_toolbar_passthrough())
      hotspot_build(dts->root->head,appmgr_nontoolbar_area());
    hotspot_graph();
  }

  /* Find the current node */
  p = hotspot_closest(cursor->x,cursor->y);
  if (!p)
    return;

  /* If the closest node isn't really that close, just warp to
   * that closest node. Otherwise, traverse. */
  if ( (!p->div &&
	((cursor->x <= p->x+5) &&        /* no divnode, use a radius of 5 */ 
	(cursor->x >= p->x-5) &&
	(cursor->y <= p->y+5) &&
	(cursor->y >= p->y-5))) ||
       (p->div &&                        /* use divnode as a border */
	(cursor->x < p->div->x + p->div->w) &&
	(cursor->x >= p->div->x) &&
	(cursor->y < p->div->y + p->div->h) &&
	(cursor->y >= p->div->y))) {
  
    /* traverse */
    p = p->graph[direction];
    if (!p)
      return;
  }

  /* Make sure the divnode is scrolled in and focused now */
  if (p->div) {
    request_focus(p->div->owner);
  }
  else {
    /* If it's not a divnode hotspot, just move the mouse to it.
     * If we're actually focusing the widget, that code will handle
     * pointer warping, etc. Otherwise we need to do a simple pointer 
     * warp here.
     */

    s16 px,py;
    px = p->x;
    py = p->y;
    VID(coord_physicalize)(&px,&py);
    dispatch_pointing(PG_TRIGGER_MOVE,px,py,0);
    drivermessage(PGDM_CURSORWARP,0,NULL);
  }
}

void scroll_to_divnode(struct divnode *div) {
  s16 dx = 0,dy = 0;
  struct divnode *ds = div->divscroll;
  struct widget *w;

  if (!ds)
    return;

  /* Figure out how much to scroll, if any. */

  if (div->x < ds->calcx)
    dx = div->x - ds->calcx;
  else if ( (div->x + div->w) > (ds->calcx + ds->calcw) )
    dx = (div->x + div->w) - (ds->calcx + ds->calcw);
  if (div->y < ds->calcy)
    dy = div->y - ds->calcy;
  else if ( (div->y + div->h) > (ds->calcy + ds->calch) )
    dy = (div->y + div->h) - (ds->calcy + ds->calch);

  /* No scrolling? */
  if (!(dx || dy))
    return;

  /* Get a pointer to the scroll bar */
  if (!iserror(rdhandle((void **)&w,PG_TYPE_WIDGET,-1,
			ds->owner->scrollbind)) && w) {
    
    /* FIXME: horizontal scroll here! */
    
    if (dy)
      widget_set(w,PG_WP_VALUE,widget_get(w,PG_WP_VALUE) + dy);
    
    update(NULL,1);
  }
}

/* Return a preferred position for a hotspot within the specified divnode */
void divnode_hotspot_position(struct divnode *div, s16 *hx, s16 *hy) {

  /* FIXME: Get a smarter way to find the hotspot. For example, account for
   * different optimum positioning in different types of widgets, etc. */

  *hx = div->x + div->w - 8;
  *hy = div->y + div->h - 8;
  if (*hx<div->x)
    *hx = div->x;
  if (*hy<div->y)
    *hy = div->y;
}

/* The End */







