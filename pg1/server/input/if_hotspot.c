/* $Id$
 *
 * if_hotspot.c - Use arrow keys to navigate around the screen.
 *                Besides the actual input filter, this has utilities to build the
 *                hotspot graph, scroll lists of widgets, and more.
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

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/hotspot.h>   /* Defines the interface for our utilities */
#include <pgserver/divtree.h>
#include <pgserver/widget.h>
#include <pgserver/video.h>

struct hotspot *hotspotlist;

int hotspot_compare(struct hotspot *a, struct hotspot *b);

/******************************************* Input filter ******/

void infilter_hotspot_handler(struct infilter *self, u32 trigger, union trigparam *param) {
  int consume = 0;

  /* Traverse the hotspot graph using keys defined in the theme
   */

  if (!(param->kbd.mods & ~(PGMOD_CAPS|PGMOD_NUM))) {
    /* Key down, no modifiers */
    
    if (param->kbd.key == hotkey_next) {
      consume = 1;
      if (trigger==PG_TRIGGER_KEYDOWN)
	hotspot_traverse(HOTSPOT_NEXT);
    }
    else if (param->kbd.key == hotkey_up) {
      consume = 1;
      if (trigger==PG_TRIGGER_KEYDOWN)
	hotspot_traverse(HOTSPOT_UP);
    }
    else if (param->kbd.key == hotkey_down) {
      consume = 1;
      if (trigger==PG_TRIGGER_KEYDOWN)
	hotspot_traverse(HOTSPOT_DOWN);
    }
    else if (param->kbd.key == hotkey_left) {
      consume = 1;
      if (trigger==PG_TRIGGER_KEYDOWN)
	hotspot_traverse(HOTSPOT_LEFT);
    } 
    else if (param->kbd.key == hotkey_right) {
      consume = 1;
      if (trigger==PG_TRIGGER_KEYDOWN)
	hotspot_traverse(HOTSPOT_RIGHT);    
    }
  }
  
  else if (!(param->kbd.mods & ~(PGMOD_CAPS|PGMOD_NUM|PGMOD_SHIFT))) {
    /* Key down with shift */
    
    if (param->kbd.key == hotkey_next) {
      consume = 1;
      if (trigger==PG_TRIGGER_KEYDOWN)
	hotspot_traverse(HOTSPOT_PREV);
    }
  }
    
  /* Pass it on if we didn't use it */
  if (!consume)
    infilter_send(self,trigger,param);
}

struct infilter infilter_hotspot = {
  /*accept_trigs:  */PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP,
  /*absorb_trigs:  */PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP,
       /*handler:  */&infilter_hotspot_handler 
};


/******************************************* Public hotspot utilities ******/

/* Delete all hotspots */
void hotspot_free(void) {
  struct hotspot *condemn;
  while (hotspotlist) {
    condemn = hotspotlist;
    hotspotlist = hotspotlist->next;
    g_free(condemn);
  }
}

/* Hide the hotspot navigation cursor */
void hotspot_hide(void) { 
  if (dts->top->hotspot_cursor)
    cursor_hide(dts->top->hotspot_cursor);
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
  int x,y;
  g_error e;

  if (!n) return success;
  
  /* shall this be a hotspot? */
  if ((n->flags & DIVNODE_HOTSPOT) && n->r.w>0 && n->r.h>0 &&
      ((!ntb) || n->r.x < ntb->r.x || n->r.y < ntb->r.y ||
       n->r.x >= ntb->r.x+ntb->r.w || n->r.y >= ntb->r.y+ntb->r.h) &&
      ((!n->divscroll) || (n->divscroll->calc.w && n->divscroll->calc.h))) {

    /* Find a good place within the node for the hotspot */
    divnode_hotspot_position(n,&x,&y);

    /* Scrolled? Clip to the scrolling container. */
    if (n->divscroll) {
      if (x < n->divscroll->calc.x) x = n->divscroll->calc.x;
      if (y < n->divscroll->calc.y) y = n->divscroll->calc.y;
      if (x > n->divscroll->calc.x + n->divscroll->calc.w - 1)
	x = n->divscroll->calc.x + n->divscroll->calc.w - 1;
      if (y > n->divscroll->calc.y + n->divscroll->calc.h - 1)
	y = n->divscroll->calc.y + n->divscroll->calc.h - 1;
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
  int x,y;
  struct divtree *dt;

  /* rebuild the graph */
  if (!hotspotlist) {
    hotspot_build(dts->top->head,NULL);
    if (popup_toolbar_passthrough())
      hotspot_build(dts->root->head,appmgr_nontoolbar_area());
    hotspot_graph();
  }

  /* Create the hotspotnav cursor if it doesn't exist */
  if (!dts->top->hotspot_cursor)
    if (iserror(cursor_new(&dts->top->hotspot_cursor,NULL,-1)))
      return;

  /* Find the current node 
   */
  cursor_getposition(dts->top->hotspot_cursor,&x,&y,&dt);
  p = hotspot_closest(x,y);

  if (!p)
    return;

  /* If the closest node isn't really that close, just warp to
   * that closest node. Otherwise, traverse. 
   *
   * NOTE: We used to test whether the hotspot cursor was within
   * the same divnode as the hotspot, but now that the hotspot cursor
   * is separate from all driver-controlled cursors we can do an
   * exact match.
   */
  if (x==p->x && y==p->y) {
    /* traverse */
    p = p->graph[direction];
    if (!p)
      return;
  }

  /* Make sure the divnode is scrolled in and focused now */
  if (p->div)
    request_focus(p->div->owner);

  /* Warp the hotspot cursor to the new hotspot location */
  cursor_move(dts->top->hotspot_cursor,p->x,p->y,dt);
}

void scroll_to_divnode(struct divnode *div) {
  s16 dx = 0,dy = 0;
  struct divnode *ds = div->divscroll;
  struct widget *w;

  if (!ds)
    return;

  /* If the divnode is larger or equal size to the scrolled
   * container, no need to scroll it in.
   * This fixes some confusing scroll behavior when focusing the textbox widget.
   */
  if ( (div->r.x <= ds->calc.x && (div->r.x + div->r.w) > (ds->calc.x + ds->calc.w)) ||
       (div->r.y <= ds->calc.y && (div->r.y + div->r.h) > (ds->calc.y + ds->calc.h)) )
    return;

  /* Figure out how much to scroll, if any. */

  if (div->r.x < ds->calc.x)
    dx = div->r.x - ds->calc.x;
  else if ( (div->r.x + div->r.w) > (ds->calc.x + ds->calc.w) )
    dx = (div->r.x + div->r.w) - (ds->calc.x + ds->calc.w);
  if (div->r.y < ds->calc.y)
    dy = div->r.y - ds->calc.y;
  else if ( (div->r.y + div->r.h) > (ds->calc.y + ds->calc.h) )
    dy = (div->r.y + div->r.h) - (ds->calc.y + ds->calc.h);

  /* No scrolling? */
  if (!(dx || dy))
    return;

  /* Get a pointer to the scroll bar */
  if (!iserror(rdhandle((void **)&w,PG_TYPE_WIDGET,-1,
			ds->owner->scrollbind)) && w) {
    
    if (dx)
      widget_set(w,PG_WP_SCROLL_X,widget_get(w,PG_WP_SCROLL_X) + dx);    
    if (dy)
      widget_set(w,PG_WP_SCROLL_Y,widget_get(w,PG_WP_SCROLL_Y) + dy);
    
    update(NULL,1);
  }
}

/* Return a preferred position for a hotspot within the specified divnode */
void divnode_hotspot_position(struct divnode *div, int *hx, int *hy) {

  /* FIXME: Get a smarter way to find the hotspot. For example, account for
   * different optimum positioning in different types of widgets, etc. */

  *hx = div->r.x + div->r.w - 8;
  *hy = div->r.y + div->r.h - 8;
  if (*hx<div->r.x)
    *hx = div->r.x;
  if (*hy<div->r.y)
    *hy = div->r.y;
}


/* Global hotkeys, and a function to load them */
u16 hotkey_left, hotkey_right, hotkey_up, hotkey_down;
u16 hotkey_activate, hotkey_next;
void reload_hotkeys(void) {
  hotkey_left     = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_LEFT);
  hotkey_right    = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_RIGHT);
  hotkey_up       = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_UP);
  hotkey_down     = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_DOWN);
  hotkey_activate = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_ACTIVATE);
  hotkey_next     = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_NEXT);
}

/******************************************* Private utilities ******/

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

/* The End */






