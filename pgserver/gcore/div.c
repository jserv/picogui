/* $Id: div.c,v 1.87 2002/09/15 10:51:47 micahjd Exp $
 *
 * div.c - calculate, render, and build divtrees
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
#include <pgserver/widget.h>
#include <pgserver/hotspot.h>

/* Check flags for divnode-level scrolling, and modify the
 * divnode's size if necessary. We must do this before
 * divnode_recalc() and before div_rebuild()
 */
void divnode_divscroll(struct divnode *n) {
  /* Implement DIVNODE_EXTEND_* flags, used for scrolling */

  n->x = n->calcx;
  n->y = n->calcy;
  n->w = n->calcw;
  n->h = n->calch;

  if (n->flags & DIVNODE_EXTEND_WIDTH)
    n->w  = max(n->w,max(n->pw,n->cw));
  if (n->flags & DIVNODE_EXTEND_HEIGHT)
    n->h  = max(n->h,max(n->ph,n->ch));
  
  /* Implement the calculation-time part of DIVNODE_DIVSCROLL */
  if (n->flags & DIVNODE_DIVSCROLL) {
    if (!n->divscroll)
      n->divscroll = n;
    if (n->next) {
      n->next->flags |= DIVNODE_DIVSCROLL;
      n->next->divscroll = n->divscroll;
    }
    if (n->div) {
      n->div->flags |= DIVNODE_DIVSCROLL;
      n->div->divscroll = n->divscroll;
    }
  }
}

/* Split a divnode into two rectangles, according to the
 * supplied divnode flags and 'split' value.
 */
void divnode_split(struct divnode *n, struct rect *divrect,
		   struct rect *nextrect) {
  s16 split = n->split;
  int popupclip = 0;

  /* DIVNODE_UNDERCONSTRUCTION _must_ be first, so it can
   * override all the normal sizing flags.
   */
  if (n->flags & DIVNODE_UNDERCONSTRUCTION) {

    /* Sometimes we should just not mess with it... */
    if (n->flags & (DIVNODE_SPLIT_POPUP | DIVNODE_SPLIT_IGNORE))
      return;

    nextrect->x = n->calcx;
    nextrect->y = n->calcy;
    nextrect->w = n->calcw;
    nextrect->h = n->calch;
    
    divrect->x = n->calcx;
    divrect->y = n->calcy;
    divrect->w = 0;
    divrect->h = 0;
  }

  /* Process as a popup box size */
  else if (n->flags & DIVNODE_SPLIT_POPUP) {
    int x,y,w,h,margin,i;

    n->flags &= ~DIVNODE_SPLIT_POPUP;   /* Clear flag */

    /* Get size */
    x = n->div->calcx;
    y = n->div->calcy;
    w = n->div->calcw;
    h = n->div->calch;

    /* Get margin value */
    margin = theme_lookup(n->owner->in->div->state,PGTH_P_MARGIN);
    
    /* If the size is zero, default to the preferred size 
     *
     * This preferred size has already accounted for the popup's 
     * border, but we add this in again ourselves below.
     *
     * Simplest way to avoid adding it in twice is to subtract
     * it from the preferred size.
     */

    if (!w)
      w = max(n->div->cw,n->div->pw) - (margin<<1);
    if (!h)
      h = max(n->div->ch,n->div->ph) - (margin<<1);
    
    /* The width and height specified in the theme are minimum values */
    i = theme_lookup(n->owner->in->div->state,PGTH_P_WIDTH);
    if (w<i)
      w = i;
    i = theme_lookup(n->owner->in->div->state,PGTH_P_HEIGHT);
    if (h<i)
      h = i;
    
    /* Special positioning codes */
    
    if (x == PG_POPUP_CENTER) {
      x=(vid->lxres>>1)-(w>>1);
      y=(vid->lyres>>1)-(h>>1);
    }
    else if (x == PG_POPUP_ATCURSOR || x==PG_POPUP_ATEVENT) {
      struct widget *snap;
      struct conbuf *cb;

      /* Snap to the last click or to the last event */
      if (x == PG_POPUP_ATEVENT && n->owner) {
	/* The last event is stored along with the client's connection buffer */
	cb = find_conbuf(n->owner->owner);
	if ((!cb) || iserror(rdhandle((void**)&snap, 
				      PG_TYPE_WIDGET, -1, cb->lastevent_from)))
	  snap = NULL;
      }
      else {
	struct cursor *c;
	c = cursor_get_default();
	if ((!c) || iserror(rdhandle((void**)&snap,
				     PG_TYPE_WIDGET, -1, c->ctx.widget_last_clicked)))
	  snap = NULL;
      }

      /* This is a menu, allow it to overlap toolbars */
      n->div->flags &= ~DIVNODE_POPUP_NONTOOLBAR;
      
      if (snap && snap->type == PG_WIDGET_BUTTON) {
	/* snap to a button edge */
	x = snap->in->div->x;
	y = snap->in->div->y + snap->in->div->h + margin;
	if ((y+h)>=vid->yres) /* Flip over if near the bottom */
	  y = snap->in->div->y - h - margin;
      }
      else if (snap && snap->type == PG_WIDGET_MENUITEM) {
	/* snap to a menuitem edge */
	x = snap->in->div->x + snap->in->div->w;
	y = snap->in->div->y;
      }
      else {
	/* exactly at the cursor */
	cursor_getposition(NULL,&x,&y);
      } 
    }
    
    /* Set the position and size, accounting for the popup's border */
    divrect->x = x-margin;
    divrect->y = y-margin;
    divrect->w = w+(margin<<1);
    divrect->h = h+(margin<<1);

    /* Remember to clip this later */
    popupclip = 1;
  }
  
  /* All available space for div */
  else if (n->flags & DIVNODE_SPLIT_EXPAND) {

    /* NOTE: the discrepancy in using calcx/y and normal w/h here
     * is intentional. We want the normal calculated x/y coordinates
     * because we don't want scrolling to be taking effect. But,
     * the width and height should be expanded if necessary.
     */
    divrect->x = n->calcx;
    divrect->y = n->calcy;
    divrect->w = n->w;
    divrect->h = n->h; 

    nextrect->x = n->calcx;
    nextrect->y = n->calcy;
    nextrect->w = 0;
    nextrect->h = 0;
  }

  /* Vertical */
  else if (n->flags & (DIVNODE_SPLIT_TOP|DIVNODE_SPLIT_BOTTOM)) {
    
    if (n->flags & DIVNODE_UNIT_PERCENT)
      split = (n->h*split)/100;
    
    else if ( (n->flags & DIVNODE_UNIT_CNTFRACT) && 
	      n->owner && (split & 0xFF)) {
      struct widget *container;
      if (!iserror(rdhandle((void**)&container,PG_TYPE_WIDGET,
			    -1,n->owner->container)))
	split = container->in->div->h * (split >> 8) / (split & 0xFF);
    }
    
    /* Not enough space. Shrink the divnode to fit in the available space. 
     */
    if (split>n->h)
	split = n->h;
    
    divrect->x = n->calcx;
    divrect->w = n->w;
    nextrect->x = n->calcx;
    nextrect->w = n->w;

    if (n->flags & DIVNODE_SPLIT_TOP) {
      divrect->y = n->calcy;
      divrect->h = split;
      nextrect->y = n->calcy+split;
      nextrect->h = n->h-split;
    }
    else {
      divrect->y = n->calcy+n->h-split;
      divrect->h = split;
      nextrect->y = n->calcy;
      nextrect->h = n->h-split;
    }
  }
  
  /* Horizontal */
  else if (n->flags & (DIVNODE_SPLIT_LEFT|DIVNODE_SPLIT_RIGHT)) {
    if (n->flags & DIVNODE_UNIT_PERCENT)
      split = (n->w*split)/100;
    
    else if ( (n->flags & DIVNODE_UNIT_CNTFRACT) && 
	      n->owner && (split & 0xFF)) {
      struct widget *container;
      if (!iserror(rdhandle((void**)&container,PG_TYPE_WIDGET,
			    -1,n->owner->container)))
	split = container->in->div->w * (split >> 8) / (split & 0xFF);
    }
    

    /* Not enough space. Shrink the divnode to fit in the available space. 
     */
    if (split>n->w)
	split = n->w;
    
    divrect->y = n->calcy;
    divrect->h = n->h;
    nextrect->y = n->calcy;
    nextrect->h = n->h;

    if (n->flags & DIVNODE_SPLIT_LEFT) {
      divrect->x = n->calcx;
      divrect->w = split;
      nextrect->x = n->calcx+split;
      nextrect->w = n->w-split;
    }
    else {
      divrect->x = n->calcx+n->w-split;
      divrect->w = split;
      nextrect->x = n->calcx;
      nextrect->w = n->w-split;
    }
  }
  
  /* Center the 'div' node in this one. If a 'next' node exists,
   * it has the same coords as this one */
  else if (n->flags & DIVNODE_SPLIT_CENTER) {
    divrect->w = n->cw;
    divrect->h = n->ch;
    divrect->x = n->calcx+(n->w-divrect->w)/2;
    divrect->y = n->calcy+(n->h-divrect->h)/2;
    
    nextrect->x = n->calcx;
    nextrect->y = n->calcy;
    nextrect->w = n->w;
    nextrect->h = n->h;
  }
  
  /* Create a border of 'split' pixels between the 'div' node and
   * this node, if a 'next' exists it is the same as this node. */
  else if (n->flags & DIVNODE_SPLIT_BORDER) {
    if ((split*2)>n->h || (split*2)>n->w) {
      if (n->h>n->w)
	split = n->w/2;
      else
	split = n->h/2;
    }
    
    divrect->x = n->calcx+split;
    divrect->y = n->calcy+split;
    divrect->w = n->w-split*2;
    divrect->h = n->h-split*2;
    
    nextrect->x = n->calcx;
    nextrect->y = n->calcy;
    nextrect->w = n->w;
    nextrect->h = n->h;
  }

  /* Keep existing sizes, ignoring split? */
  else if (n->flags & DIVNODE_SPLIT_IGNORE) {
    /* Prevent transferring new rectangles, bail out immediately */
    return;
  }

  /* Otherwise give children same w,h,x,y */
  else {
    nextrect->x = n->calcx;
    nextrect->y = n->calcy;
    nextrect->w = n->w;
    nextrect->h = n->h;
    
    divrect->x = n->calcx;
    divrect->y = n->calcy;
    divrect->w = n->w;
    divrect->h = n->h;
  }

  /* Transfer over rectangles */
  if (n->next) {
    n->next->x = nextrect->x;
    n->next->y = nextrect->y;
    n->next->w = nextrect->w;
    n->next->h = nextrect->h;
    n->next->calcx = nextrect->x;
    n->next->calcy = nextrect->y;
    n->next->calcw = nextrect->w;
    n->next->calch = nextrect->h;
  }
  if (n->div) {
    n->div->x = divrect->x;
    n->div->y = divrect->y;
    n->div->w = divrect->w;
    n->div->h = divrect->h;
    n->div->calcx = divrect->x;
    n->div->calcy = divrect->y;
    n->div->calcw = divrect->w;
    n->div->calch = divrect->h;
  }

  /* Validate the size of a popup*/
  if (popupclip)
    clip_popup(n->div);  
}	   

/* Fill in the x,y,w,h of this divnode's children node based on it's
 * x,y,w,h and it's split. Also rebuilds child divnodes.
 * Recurse into all the node's children.
 */
void divnode_recalc(struct divnode **pn, struct divnode *parent) {
   struct divnode *n = *pn;
   struct rect divrect, nextrect;
   struct rect old_divrect, old_nextrect;

   if (!n)
     return;

   if (n->flags & DIVNODE_NEED_REBUILD)
     div_rebuild(n);

   if (n->flags & DIVNODE_NEED_RECALC) {
     /* Save the old positions of each child */
     if (n->div) {
       old_divrect.x = n->div->calcx;
       old_divrect.y = n->div->calcy;
       old_divrect.w = n->div->calcw;     
       old_divrect.h = n->div->calch;
     }
     if (n->next) {
       old_nextrect.x = n->next->calcx;
       old_nextrect.y = n->next->calcy;
       old_nextrect.w = n->next->calcw;
       old_nextrect.h = n->next->calch;     
     }

     /* Split the rectangle */
     divnode_split(n,&divrect,&nextrect);
     
     /* Recalc completed.  Propagate the changes to child nodes if their
      * dimensions have changed.
      */
     if (n->div && memcmp(&divrect,&old_divrect,sizeof(divrect)))
       n->div->flags |= DIVNODE_NEED_RECALC | DIVNODE_NEED_REBUILD;
     if (n->next && memcmp(&nextrect,&old_nextrect,sizeof(nextrect)))
       n->next->flags |= DIVNODE_NEED_RECALC | DIVNODE_NEED_REBUILD;

     /* Force child recalc if necessary */
     if (n->flags & DIVNODE_FORCE_CHILD_RECALC) {
       if (n->div)
	 n->div->flags |= DIVNODE_NEED_RECALC | DIVNODE_FORCE_CHILD_RECALC;
       if (n->next)
	 n->next->flags |= DIVNODE_NEED_RECALC | DIVNODE_FORCE_CHILD_RECALC;
       n->flags &= ~DIVNODE_FORCE_CHILD_RECALC;
     }

     /* Apply changed necessary for scrolling to the children we just calculated
      */
     if (n->div)
       divnode_divscroll(n->div);
     if (n->next)
       divnode_divscroll(n->next);
     
     /* We're done */
     if (!(n->flags & DIVNODE_UNDERCONSTRUCTION))
       n->flags &= ~DIVNODE_NEED_RECALC;
   }
   
   divnode_recalc(&n->div,n);
   divnode_recalc(&n->next,n);
}

/* Redraw the divnodes if they need it.
 * Recursive. Does not call hwr_update
 */
void divnode_redraw(struct divnode *n,int all) {
   if (!n) return;

   /* Make sure the node needs redrawing and it's
    * ready to be rendered.
    */
   if ( (all || (n->flags & (DIVNODE_NEED_REDRAW | 
			     DIVNODE_SCROLL_ONLY | 
			     DIVNODE_INCREMENTAL) )) &&
	!(n->flags & DIVNODE_UNDERCONSTRUCTION) ) { 

     if (n->w && n->h)
       grop_render(n,NULL);

     if (n->next && (n->flags & DIVNODE_PROPAGATE_REDRAW))
	 n->next->flags |= DIVNODE_NEED_REDRAW | DIVNODE_PROPAGATE_REDRAW;

     if (n->div && (n->flags & DIVNODE_NEED_REDRAW))
       n->div->flags |= DIVNODE_NEED_REDRAW | DIVNODE_PROPAGATE_REDRAW;

     if (n->flags & DIVNODE_PROPAGATE_SCROLL) {
       if (n->next)
	 n->next->flags |= DIVNODE_SCROLL_ONLY | DIVNODE_PROPAGATE_SCROLL;
       if (n->div)
	 n->div->flags |= DIVNODE_SCROLL_ONLY | DIVNODE_PROPAGATE_SCROLL;
     }

     if (n->div && (n->flags & DIVNODE_SCROLL_ONLY))
       n->div->flags |= DIVNODE_SCROLL_ONLY | DIVNODE_PROPAGATE_SCROLL;

     n->flags &= ~(DIVNODE_NEED_REDRAW | DIVNODE_PROPAGATE_REDRAW |
		   DIVNODE_SCROLL_ONLY | DIVNODE_INCREMENTAL |
		   DIVNODE_PROPAGATE_SCROLL);
   }

   /* Don't propagate if this node is invisible */
   if (n->w && n->h) {
     divnode_redraw(n->div,all);
     divnode_redraw(n->next,all);
   }
}

/************* Functions for building the divtree */

/* Allocate an empty divnode */
g_error newdiv(struct divnode **p,struct widget *owner) {
  g_error e;
#ifdef DEBUG_KEYS
  num_divs++;
#endif
  e = g_malloc((void **)p,sizeof(struct divnode));
  errorcheck;
  memset(*p,0,sizeof(struct divnode));
  (*p)->flags = 
    DIVNODE_NEED_RECALC | 
    DIVNODE_SIZE_RECURSIVE |
    DIVNODE_SIZE_AUTOSPLIT |
    DIVNODE_UNDERCONSTRUCTION;
  (*p)->owner = owner;
  return success;
}

/* Make a new divtree */
g_error divtree_new(struct divtree **dt) {
  g_error e;
  e = g_malloc((void **) dt,sizeof(struct divtree));
  memset(*dt,0,sizeof(struct divtree));
  errorcheck;
  e = newdiv(&(*dt)->head,NULL);
  errorcheck;
  (*dt)->head->calcw = vid->lxres;
  (*dt)->head->calch = vid->lyres;
  (*dt)->head->w = vid->lxres;
  (*dt)->head->h = vid->lyres;
  (*dt)->flags = DIVTREE_ALL_REDRAW;
  (*dt)->head->flags &= ~DIVNODE_UNDERCONSTRUCTION;
  return success;
}

/* Delete a divtree */
void divtree_free(struct divtree *dt) {
  r_divnode_free(dt->head);               /* Delete the tree of divnodes */
  if (dt->hotspot_cursor)
    pointer_free(-1,dt->hotspot_cursor);  /* Delete the hotspot cursor   */
  g_free(dt);                             /* Delete the divtree */
}

/* Delete a divnode recursively */
void r_divnode_free(struct divnode *n) {
  if (!n) return;
  r_divnode_free(n->next);
  r_divnode_free(n->div);
  grop_free(&n->grop);
#ifdef DEBUG_KEYS
  num_divs--;
#endif
  g_free(n);
}

void update(struct divnode *subtree,int show) {
  if (VID(update_hook)())
    return;

  if (subtree) {
    /* Subtree update */
    
    if (subtree->owner->dt != dts->top) {
      /* Well, it's not from around here. Only allow it if the popups
       * are in the nontoolbar area and the update is in a toolbar */

      if (!popup_toolbar_passthrough())
	return;
      if (!divnode_in_toolbar(subtree))
	return;
    }

    divnode_recalc(&subtree,NULL);
    divnode_redraw(subtree,0);
  }
  else 
    /* Full update */
    r_dtupdate(dts->top);
  
  if (show) {
    VID(sprite_showall) ();
    
    /* NOW we update the hardware */
    realize_updareas();
  }

#ifdef DEBUG_VIDEO
  printf("****************** Update (sub: %p)\n",subtree);
#endif
}

void divtree_size_and_calc(struct divtree *dt) {
  /* Perform a divresize_recursive if it has been requested */
  if (dt->flags & DIVTREE_NEED_RESIZE) {
    divresize_recursive(dt->head);
    dt->flags &= ~DIVTREE_NEED_RESIZE;
  }
  
  if (dt->flags & DIVTREE_NEED_RECALC) {
#ifdef DEBUG_VIDEO
    printf("divnode_recalc(%p)\n",dt->head);
#endif

    divnode_recalc(&dt->head,NULL);
    
    /* If we need yet another resize, try again. This accounts for iterative
     * sizing of things that change preferred size depending on what actual size they get.
     * FIXME: There's probably a cleaner way to do this
     */
    if (dt->flags & DIVTREE_NEED_RESIZE) {
      divtree_size_and_calc(dt);
      return;
    }

    /* If we recalc, at least one divnode will need redraw */
    dt->flags |= DIVTREE_NEED_REDRAW;

    /* The hotspot graph is now invalid */
    hotspot_free();

    /* Update which widgets are under the cursors */
    cursor_update_hover();
  }
}

/* Update the divtree's calculations and render (both only if necessary) */
void r_dtupdate(struct divtree *dt) {

  /* Update the layers below only if this is a _complete_ update. */
  if (dt->next && (dt->next->flags & DIVTREE_ALL_REDRAW)) 
    r_dtupdate(dt->next);

  /* Draw on the way back up from the recursion, so the layers appear
     in the right order */

  /* If we're drawing everything anyway, might as well take this opportunity
   * to update the popup clipping. This is necessary when toolbars are added
   * when a popup is onscreen */
  if (dt->flags & DIVTREE_CLIP_POPUP) {
    if (dt->head->next && dt->head->next->owner &&
	dt->head->next->owner->type == PG_WIDGET_POPUP)
      clip_popup(dt->head->next->div);
  }

  divtree_size_and_calc(dt);

  if (dt->flags &(DIVTREE_NEED_REDRAW|DIVTREE_ALL_REDRAW)) {
#ifdef DEBUG_VIDEO
    printf("divnode_redraw\n");
#endif

    if (dt->flags & DIVTREE_ALL_NONTOOLBAR_REDRAW)
      divnode_redraw(appmgr_nontoolbar_area(),dt->flags & DIVTREE_ALL_REDRAW);
    else
      divnode_redraw(dt->head,dt->flags & DIVTREE_ALL_REDRAW);
  }

  /* All clean now, clear flags. */
  dt->flags &= ~(DIVTREE_ALL_REDRAW | DIVTREE_NEED_REDRAW | 
		 DIVTREE_NEED_RECALC | DIVTREE_ALL_NONTOOLBAR_REDRAW |
		 DIVTREE_NEED_RESIZE);
}

/* Create a stack and its root node */
g_error dts_new(void) {
  g_error e;
  e = g_malloc((void **) &dts,sizeof(struct dtstack));
  errorcheck;
  memset(dts,0,sizeof(struct dtstack));
  
  /* Make the root tree */
  e = divtree_new(&dts->root);
  errorcheck;
  dts->top = dts->root;

  return success;
}

/* Delete the stack and all trees inside it */
void dts_free(void) {
  struct divtree *p,*condemn;

  p = dts->top;
  while (p) {
    condemn = p;
    p = p->next;
    divtree_free(condemn);
  }
  g_free(dts);
}

/* dts_push creates a new layer, and dts_pop deletes a layer */

g_error dts_push(void) {
  g_error e;
  struct divtree *t;

  e = divtree_new(&t);
  errorcheck;

  t->next = dts->top;
  dts->top = t;

  /* The hotspot graph is invalid */
  hotspot_free();
  hotspot_hide();

  return success;
}

void dts_pop(struct divtree *dt) {
  struct divtree **p;

  /* Traverse the stack, find the pointer to this tree */
  p = &dts->top;
  while (*p) {
     if (*p == dt) break;
     p = &((*p)->next);
  }
#ifdef DEBUG_WIDGET
   if (*p != dt)
     guru("In dts_pop(): This shouldn't happen!");
#endif
   
  hotspot_free();
  *p = (*p)->next;
  divtree_free(dt);
}

/* Aligns a 'thing' of specified width and height in the specified divnode
 * according to the alignment type specified in align.
 */
void align(struct gropctxt *d,alignt align,s16 *w,s16 *h,s16 *x,s16 *y) {
  switch (align) {
  case PG_A_CENTER:
    *x = (d->w-*w)/2;
    *y = (d->h-*h)/2;
    break;
  case PG_A_TOP:
    *x = (d->w-*w)/2;
    *y = 0;
    break;
  case PG_A_BOTTOM:
    *x = (d->w-*w)/2;
    *y = d->h-*h;
    break;
  case PG_A_LEFT:
    *y = (d->h-*h)/2;
    *x = 0;
    break;
  case PG_A_RIGHT:
    *y = (d->h-*h)/2;
    *x = d->w-*w;
    break;
  case PG_A_NW:
    *x = 0;
    *y = 0;
    break;
  case PG_A_NE:
    *x = d->w-*w;
    *y = 0;
    break;
  case PG_A_SW:
    *x = 0;
    *y = d->h-*h;
    break;
  case PG_A_SE:
    *x = d->w-*w;
    *y = d->h-*h;
    break;
  case PG_A_ALL:
    *x = 0;
    *y = 0;
    *w = d->w;
    *h = d->h;
    break;
  }
  *x += d->x;
  *y += d->y;
}

/* Little helper to rotate a side constant 90 degrees counterclockwise */
int rotate_side(int s) {
  switch (s) {
  case PG_S_TOP:    s = PG_S_LEFT;   break;
  case PG_S_LEFT:   s = PG_S_BOTTOM; break;
  case PG_S_BOTTOM: s = PG_S_RIGHT;  break;
  case PG_S_RIGHT:  s = PG_S_TOP;    break;
  }
  return s;
}

/* Rotate them 90 degrees counterclockwise, then mirror
   across the horizontal axis */
int mangle_align(int al) {
  switch (al) {
  case PG_A_TOP:    al = PG_A_LEFT;   break;
  case PG_A_NE:     al = PG_A_SW;     break;
  case PG_A_RIGHT:  al = PG_A_BOTTOM; break;
  case PG_A_SE:     al = PG_A_SE;     break;
  case PG_A_BOTTOM: al = PG_A_RIGHT;  break;
  case PG_A_SW:     al = PG_A_NE;     break;
  case PG_A_LEFT:   al = PG_A_TOP;    break;
  case PG_A_NW:     al = PG_A_NW;     break;
  }   
  return al;
}

/* Calculate a new split value for the specified divnode
 * based on the pw,ph,cw, and ch values of the node's 'div' child */
void divresize_split(struct divnode *div) {
  s16 oldsplit = div->split;

  if ((div->flags & DIVNODE_SIZE_AUTOSPLIT) && div->div) {

    /* Calculate the correct split value based on the preferred size of
     * this node, the preferred size of its children, and this node's
     * orientation.
     */
    switch (div->flags & ~SIDEMASK) {
    case PG_S_TOP:
    case PG_S_BOTTOM:
      div->split = max(div->div->ph,div->div->ch);
      break;
    case PG_S_LEFT:
    case PG_S_RIGHT: 
      div->split = max(div->div->pw,div->div->cw);
      break;
      
      /* If it's something else, leave the split alone */
    }
    
    if (div->split != oldsplit) {
      /* recalc! */
      div->flags |= DIVNODE_NEED_RECALC;
      if (div->owner)
	div->owner->dt->flags |= DIVTREE_NEED_RECALC;
    }
  }
}

/* Recursively recalculate cw and ch for the given divnode */
void divresize_recursive(struct divnode *div) {
  s16 dw,dh,nw,nh;     /* Size of div and next child nodes */
  s16 old_cw, old_ch;  /* Old preffered size for children */

  old_cw = div->cw;
  old_ch = div->ch;

  /* Calculate child nodes' sizes */
	
  if (div->div) {
    divresize_recursive(div->div);
    dw = max(div->div->cw,div->div->pw);
    dh = max(div->div->ch,div->div->ph);
  }
  else
    dw = dh = 0;
  
  if (div->next) {
    divresize_recursive(div->next);
    nw = max(div->next->cw,div->next->pw);
    nh = max(div->next->ch,div->next->ph);
  }
  else
    nw = nh = 0;
  
  if (div->flags & DIVNODE_SIZE_RECURSIVE) {
    
    /* Combine them depending on orientation */
    switch (div->flags & ~SIDEMASK) {
      
    case PG_S_TOP:
    case PG_S_BOTTOM:
      div->ch = dh + nh;
      div->cw = max(dw,nw);
      break;
      
    case PG_S_LEFT:
    case PG_S_RIGHT:
      div->cw = dw + nw;
      div->ch = max(dh,nh);
      break;
      
    case PG_S_ALL:
    case DIVNODE_SPLIT_CENTER:
      div->cw = dw;
      div->ch = dh;
      break;
      
    case DIVNODE_SPLIT_BORDER:
      div->cw = dw + (div->split<<1);
      div->ch = dh + (div->split<<1);
      break;

    default:
      div->cw = max(dw,nw);
      div->ch = max(dh,nh);
      break;

    }

    if (old_cw != div->cw || old_ch != div->ch) {

      /* If this divnode is under the control of a scrollbar, make sure
       * we recalc the scrollbar if the preferred size changes
       */
      if (div->divscroll && div->divscroll->owner && div->divscroll->owner->scrollbind) {
	struct widget *w;
	if (!iserror(rdhandle((void**)&w,PG_TYPE_WIDGET,-1,div->divscroll->owner->scrollbind))) {
	  w->in->flags |= DIVNODE_NEED_RECALC;
	  w->dt->flags |= DIVTREE_NEED_RECALC;
	}
      }

      div->flags |= DIVNODE_NEED_RECALC | DIVNODE_FORCE_CHILD_RECALC | DIVNODE_NEED_REBUILD;
    }
  }    

  divresize_split(div);
}

/* This function returns nonzero if there is more than one divtree layer,
   and all layers except for the root divtree has DIVNODE_POPUP_NONTOOLBAR */
int popup_toolbar_passthrough(void) {
  struct divtree *t;

  if (dts->top == dts->root)
    return 0;

  t = dts->top;
  while (t && t!=dts->root) {
    if (t->head && t->head->next && t->head->next->div)
      if (!(t->head->next->div->flags & DIVNODE_POPUP_NONTOOLBAR))
	return 0;
    t = t->next;
  }

  return 1;
}

/* Returns nonzero if the specified divnode is within a toolbar root widget */
int divnode_in_toolbar(struct divnode *div) {
  struct widget *w;

  w = div->owner;
  if (!w)
    return 0;

  /* Find the root divnode by following the container handles */
  while (!w->isroot)
    if (iserror(rdhandle((void**) &w,PG_TYPE_WIDGET,-1,w->container)) || !w)
      return 0;
  return w->type == PG_WIDGET_TOOLBAR;
}

/* Given a starting point and a node, this finds the 'div' branch the node
 * is on. This is used in word wrapping to figure out what line a
 * divnode is on.
 */
struct divnode *r_divnode_findbranch(struct divnode *p,
				     struct divnode *dest,
				     struct divnode *branch) {
  struct divnode *x;

  /* Leaf */
  if (!p)
    return NULL;

  /* We've found it, so hopefully we know what branch it came from */
  if (p==dest)
    return branch;

  /* Traversing 'div' we change branches, traversing 'next' we don't */
  if ((x = r_divnode_findbranch(p->div,dest,p)))
    return x;
  if ((x = r_divnode_findbranch(p->next,dest,branch)))
    return x;

  return NULL;
}
struct divnode *divnode_findbranch(struct divnode *tree,
				   struct divnode *dest) {
  return r_divnode_findbranch(tree,dest,NULL);
}

/* Find a pointer to the supplied divnode pointer in the supplied tree.
 * This is useful for deleting divnodes.
 */
struct divnode **divnode_findpointer(struct divnode *tree,
				     struct divnode *dest) {
  struct divnode **p;

  if (!tree)
    return NULL;

  /* Check if one of tree's children is what we're looking for */
  if (tree->div == dest)
    return &tree->div;
  if (tree->next == dest)
    return &tree->next;

  if ((p = divnode_findpointer(tree->div,dest)))
    return p;
  if ((p = divnode_findpointer(tree->next,dest)))
    return p;
    
  return NULL;
} 

/* Find a divnode's parent */
struct divnode *divnode_findparent(struct divnode *tree,
				   struct divnode *dest) {
  struct divnode *p;

  if (!tree)
    return NULL;

  /* Check if one of tree's children is what we're looking for */
  if (tree->div == dest || tree->next == dest)
    return tree;

  if ((p = divnode_findparent(tree->div,dest)))
    return p;
  if ((p = divnode_findparent(tree->next,dest)))
    return p;
    
  return NULL;
}

/* Turn off the DIVNODE_UNDERCONSTRUCTION flags for all divnodes owned by this client
 */
void r_activate_client_divnodes(int client, struct divnode *n) {
  if (!n)
    return;

  if (n->owner && n->owner->owner==client)
    n->flags &= ~DIVNODE_UNDERCONSTRUCTION;

  r_activate_client_divnodes(client,n->next);
  r_activate_client_divnodes(client,n->div);
}
void activate_client_divnodes(int client) {
  struct divtree *dt;
  for (dt=dts->top;dt;dt=dt->next) {
    r_activate_client_divnodes(client, dt->head);
    
    /* It's likely that the nodes we just activated need to be calculated */
    dt->flags |= DIVTREE_NEED_RECALC;
  }
}

/* Remove scrolling information from the sub-divtree */
void r_div_unscroll(struct divnode *div) {
  /* FIXME: this might not work for nested scrolling */

  if (!div) return;
  if (!(div->owner && div->owner->scrollbind)) {
    div->divscroll = NULL;
    div->flags &= ~DIVNODE_DIVSCROLL;
  }
  r_div_unscroll(div->div);
  r_div_unscroll(div->next);
}

/* The End */












