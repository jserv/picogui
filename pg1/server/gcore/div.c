/* $Id$
 *
 * div.c - calculate, render, and build divtrees
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

#include <string.h>
#include <stdio.h>

#include <pgserver/common.h>

#include <pgserver/divtree.h>
#include <pgserver/widget.h>
#include <pgserver/hotspot.h>

/* To save space, instead of checking whether the divtree is valid every time
 * we have to set a divtree flag, assign unattached widgets a fake divtree.
 * This is DT_NIL
 */
struct divnode fakedt_head;
struct divtree fakedt = {
  &fakedt_head /* head */
};

struct dtstack *dts;

/******************************************************** Public functions **/

/* Check flags for divnode-level scrolling, and modify the
 * divnode's size if necessary. We must do this before
 * divnode_recalc() and before div_rebuild()
 */
void divnode_divscroll(struct divnode *n) {
  /* Implement DIVNODE_EXTEND_* flags, used for scrolling */

  n->r = n->calc;

  if (n->flags & DIVNODE_EXTEND_WIDTH)
    n->r.w  = max(n->r.w,max(n->preferred.w,n->child.w));
  if (n->flags & DIVNODE_EXTEND_HEIGHT)
    n->r.h  = max(n->r.h,max(n->preferred.h,n->child.h));
  
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
void divnode_split(struct divnode *n, struct pgrect *divrect,
		   struct pgrect *nextrect) {
  s16 split = n->split;
#ifdef CONFIG_WIDGET_POPUP
  int popupclip = 0;
#endif

  /* DIVNODE_UNDERCONSTRUCTION _must_ be first, so it can
   * override all the normal sizing flags.
   */
  if (n->flags & DIVNODE_UNDERCONSTRUCTION) {

    /* Sometimes we should just not mess with it... */
    if (n->flags & (DIVNODE_SPLIT_POPUP | DIVNODE_SPLIT_IGNORE))
      return;

    *nextrect = n->calc;

    divrect->x = n->calc.x;
    divrect->y = n->calc.y;
    divrect->w = 0;
    divrect->h = 0;
  }

#ifdef CONFIG_WIDGET_POPUP
  /* Process as a popup box size */
  else if (n->flags & DIVNODE_SPLIT_POPUP) {
    int x,y,w,h,margin,i;

    n->flags &= ~DIVNODE_SPLIT_POPUP;   /* Clear flag */

    /* Get size */
    x = n->div->calc.x;
    y = n->div->calc.y;
    w = n->div->calc.w;
    h = n->div->calc.h;

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
      w = max(n->div->child.w,n->div->preferred.w) - (margin<<1);
    if (!h)
      h = max(n->div->child.h,n->div->preferred.h) - (margin<<1);
    
    /* The width and height specified in the theme are minimum values */
    i = theme_lookup(n->owner->in->div->state,PGTH_P_WIDTH);
    if (w<i)
      w = i;
    i = theme_lookup(n->owner->in->div->state,PGTH_P_HEIGHT);
    if (h<i)
      h = i;
    
    /* Special positioning codes */
    
    if (x == PG_POPUP_CENTER) {
      x=(n->calc.w>>1)-(w>>1);
      y=(n->calc.h>>1)-(h>>1);
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
	x = snap->in->div->r.x;
	y = snap->in->div->r.y + snap->in->div->r.h + margin;
	if ((y+h)>=vid->yres) /* Flip over if near the bottom */
	  y = snap->in->div->r.y - h - margin;
      }
      else if (snap && snap->type == PG_WIDGET_MENUITEM) {
	/* snap to a menuitem edge */
	x = snap->in->div->r.x + snap->in->div->r.w;
	y = snap->in->div->r.y;
      }
      else {
	/* exactly at the cursor */
	cursor_getposition(NULL,&x,&y,NULL);
      } 
    }
    
    /* Set the position and size, accounting for the popup's border */
    divrect->x = x-margin;
    divrect->y = y-margin;
    divrect->w = w+(margin<<1);
    divrect->h = h+(margin<<1);

    /* Remember to clip this later */
    popupclip = 1;

    /* Need to recalc it now */
    n->flags |= DIVNODE_NEED_RECALC | DIVNODE_NEED_REBUILD;
  }
#endif /* CONFIG_WIDGET_POPUP */

  
  /* All available space for div */
  else if (n->flags & DIVNODE_SPLIT_EXPAND) {

    /* NOTE: the discrepancy in using calc.x/y and normal w/h here
     * is intentional. We want the normal calculated x/y coordinates
     * because we don't want scrolling to be taking effect. But,
     * the width and height should be expanded if necessary.
     */
    divrect->x = n->calc.x;
    divrect->y = n->calc.y;
    divrect->w = n->r.w;
    divrect->h = n->r.h; 

    nextrect->x = n->calc.x;
    nextrect->y = n->calc.y;
    nextrect->w = 0;
    nextrect->h = 0;
  }

  /* Vertical */
  else if (n->flags & (DIVNODE_SPLIT_TOP|DIVNODE_SPLIT_BOTTOM)) {
    
    if (n->flags & DIVNODE_UNIT_PERCENT)
      split = (n->r.h*split)/100;
    
    else if ( (n->flags & DIVNODE_UNIT_CNTFRACT) && 
	      n->owner && (split & 0xFF)) {
      struct widget *container;
      if (!iserror(rdhandle((void**)&container,PG_TYPE_WIDGET,
			    -1,n->owner->container)))
	split = container->in->div->r.h * (split >> 8) / (split & 0xFF);
    }
    
    /* Not enough space. Shrink the divnode to fit in the available space. 
     */
    if (split>n->r.h)
	split = n->r.h;
    
    divrect->x = n->calc.x;
    divrect->w = n->r.w;
    nextrect->x = n->calc.x;
    nextrect->w = n->r.w;

    if (n->flags & DIVNODE_SPLIT_TOP) {
      divrect->y = n->calc.y;
      divrect->h = split;
      nextrect->y = n->calc.y+split;
      nextrect->h = n->r.h-split;
    }
    else {
      divrect->y = n->calc.y+n->r.h-split;
      divrect->h = split;
      nextrect->y = n->calc.y;
      nextrect->h = n->r.h-split;
    }
  }
  
  /* Horizontal */
  else if (n->flags & (DIVNODE_SPLIT_LEFT|DIVNODE_SPLIT_RIGHT)) {
    if (n->flags & DIVNODE_UNIT_PERCENT)
      split = (n->r.w*split)/100;
    
    else if ( (n->flags & DIVNODE_UNIT_CNTFRACT) && 
	      n->owner && (split & 0xFF)) {
      struct widget *container;
      if (!iserror(rdhandle((void**)&container,PG_TYPE_WIDGET,
			    -1,n->owner->container)))
	split = container->in->div->r.w * (split >> 8) / (split & 0xFF);
    }
    

    /* Not enough space. Shrink the divnode to fit in the available space. 
     */
    if (split>n->r.w)
	split = n->r.w;
    
    divrect->y = n->calc.y;
    divrect->h = n->r.h;
    nextrect->y = n->calc.y;
    nextrect->h = n->r.h;

    if (n->flags & DIVNODE_SPLIT_LEFT) {
      divrect->x = n->calc.x;
      divrect->w = split;
      nextrect->x = n->calc.x+split;
      nextrect->w = n->r.w-split;
    }
    else {
      divrect->x = n->calc.x+n->r.w-split;
      divrect->w = split;
      nextrect->x = n->calc.x;
      nextrect->w = n->r.w-split;
    }
  }
  
  /* Center the 'div' node in this one. If a 'next' node exists,
   * it has the same coords as this one */
  else if (n->flags & DIVNODE_SPLIT_CENTER) {
    divrect->w = n->child.w;
    divrect->h = n->child.h;
    divrect->x = n->calc.x+(n->r.w-divrect->w)/2;
    divrect->y = n->calc.y+(n->r.h-divrect->h)/2;
    
    nextrect->x = n->calc.x;
    nextrect->y = n->calc.y;
    nextrect->w = n->r.w;
    nextrect->h = n->r.h;
  }
  
  /* Create a border of 'split' pixels between the 'div' node and
   * this node, if a 'next' exists it is the same as this node. */
  else if (n->flags & DIVNODE_SPLIT_BORDER) {
    divrect->x = n->calc.x+split;
    divrect->y = n->calc.y+split;
    divrect->w = n->r.w-split*2;
    divrect->h = n->r.h-split*2;
    
    nextrect->x = n->calc.x;
    nextrect->y = n->calc.y;
    nextrect->w = n->r.w;
    nextrect->h = n->r.h;
  }

  /* Keep existing sizes, ignoring split? */
  else if (n->flags & DIVNODE_SPLIT_IGNORE) {
    /* Prevent transferring new rectangles, bail out immediately */
    return;
  }

  /* Otherwise give children same w,h,x,y */
  else {
    nextrect->x = n->calc.x;
    nextrect->y = n->calc.y;
    nextrect->w = n->r.w;
    nextrect->h = n->r.h;
    
    divrect->x = n->calc.x;
    divrect->y = n->calc.y;
    divrect->w = n->r.w;
    divrect->h = n->r.h;
  }

  /* Transfer over rectangles */
  if (n->next) {
    n->next->r = *nextrect;
    n->next->calc = *nextrect;
  }
  if (n->div) {
    n->div->r = *divrect;
    n->div->calc = *divrect;
  }

#ifdef CONFIG_WIDGET_POPUP
  /* Validate the size of a popup*/
  if (popupclip)
    clip_popup(n->div);  
#endif
}	   

/* Fill in the x,y,w,h of this divnode's children node based on it's
 * x,y,w,h and it's split. Also rebuilds child divnodes.
 * Recurse into all the node's children.
 */
void divnode_recalc(struct divnode **pn, struct divnode *parent) {
   struct divnode *n = *pn;
   struct pgrect divrect, nextrect;
   struct pgrect old_divrect, old_nextrect;

   if (!n)
     return;

   if (n->flags & DIVNODE_NEED_REBUILD)
     div_rebuild(n);

   if (n->flags & DIVNODE_NEED_RECALC) {
     /* Save the old positions of each child */
     if (n->div) {
       old_divrect = n->div->calc;
     }
     if (n->next) {
       old_nextrect = n->next->calc;
     }

     /* Split the rectangle */
     divnode_split(n,&divrect,&nextrect);
     
     /* Recalc completed.  Propagate the changes to child nodes if their
      * dimensions have changed.
      */
     if (!(n->flags & DIVNODE_SPLIT_IGNORE)) {
       if (n->div && memcmp(&divrect,&old_divrect,sizeof(divrect)))
	 n->div->flags |= DIVNODE_NEED_RECALC | DIVNODE_NEED_REBUILD;
       if (n->next && memcmp(&nextrect,&old_nextrect,sizeof(nextrect)))
	 n->next->flags |= DIVNODE_NEED_RECALC | DIVNODE_NEED_REBUILD;
     }       

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

     if (n->r.w && n->r.h)
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
   if (n->r.w && n->r.h) {
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
  e = VID(window_new)(&(*dt)->display,*dt);
  errorcheck;

  /* Default to lxres,lyres for this divtree's size.
   * In a rootless driver, this should be set by the driver when
   * the window is resized.
   */
  (*dt)->head->calc.w = vid->lxres;
  (*dt)->head->calc.h = vid->lyres;
  (*dt)->head->r.w = vid->lxres;
  (*dt)->head->r.h = vid->lyres;
  (*dt)->flags = DIVTREE_ALL_REDRAW;
  (*dt)->head->flags &= ~DIVNODE_UNDERCONSTRUCTION;

  /* A handle to this divtree needs to exist, so the divtree
   * can be referred to by input filters.
   */
  e = mkhandle(&(*dt)->h, PG_TYPE_DIVTREE, -1, *dt);
  errorcheck;

  return success;
}

/* Delete a divtree */
void divtree_free(struct divtree *dt) {
  /* Unlink the head divnode and delete it */
  dt->head->div  = NULL;
  dt->head->next = NULL;
  r_divnode_free(dt->head);

  /* Delete the hotspot cursor */
  if (dt->hotspot_cursor)
    pointer_free(-1,dt->hotspot_cursor);
  
  /* Delete any associated window */
  VID(window_free)(dt->display);

  /* Delete the dt structure itself, and the associated handle */
  handle_free(-1,dt->h);
  g_free(dt);
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
  struct divtree *dt;

  if (VID(update_hook)())
    return;

  if (VID(is_rootless)()) {
    /* Rootless mode, disregard whether a divtree is topmost or not */

    if (subtree) {
      /* Rootless subtree update */
      divnode_recalc(&subtree,NULL);
      divnode_redraw(subtree,0);
    }
    else {
      /* Full update, all divtrees */
      for (dt=dts->top;dt;dt=dt->next)
	r_dtupdate(dt);
      cursor_update_hover();
    }

    if (show) {
      VID(sprite_showall) ();
      for (dt=dts->top;dt;dt=dt->next)
	realize_updareas(dt);
    }	
  }

  else {
    /* Normal mode, only render the topmost divtree */

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
    else {
      /* Full update */
      r_dtupdate(dts->top);
      cursor_update_hover();
    }    

    if (show) {
      VID(sprite_showall) ();
      realize_updareas(dts->top);
    }
  }
}

void divtree_size_and_calc(struct divtree *dt) {
  /* Perform a divresize_recursive if it has been requested */
  if (dt->flags & DIVTREE_NEED_RESIZE) {
    divresize_recursive(dt->head);
    dt->flags &= ~DIVTREE_NEED_RESIZE;
    
    /* Give the top-level widget a chance to resize (necessary if
     * it's acting as a gateway between picogui's sizing system and that of 
     * a host GUI.
     */
    if (dt->head->next)
      dt->head->next->owner->def->resize(dt->head->next->owner);
  }
  
  if (dt->flags & DIVTREE_NEED_RECALC) {
#ifdef DEBUG_VIDEO
    fprintf(stderr, "divnode_recalc(%p)\n",dt->head);
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
  }
}

/* Update the divtree's calculations and render (both only if necessary) */
void r_dtupdate(struct divtree *dt) {

  /* Update the layers below only if this is a _complete_ update. */
  if (dt->next && (dt->next->flags & DIVTREE_ALL_REDRAW)) 
    r_dtupdate(dt->next);

  /* Draw on the way back up from the recursion, so the layers appear
     in the right order */

#ifdef CONFIG_WIDGET_POPUP
  /* If we're drawing everything anyway, might as well take this opportunity
   * to update the popup clipping. This is necessary when toolbars are added
   * when a popup is onscreen */
  if (dt->flags & DIVTREE_CLIP_POPUP) {
    if (dt->head->next && dt->head->next->owner &&
	dt->head->next->owner->type == PG_WIDGET_POPUP)
      clip_popup(dt->head->next->div);
  }
#endif

  divtree_size_and_calc(dt);

  if (dt->flags &(DIVTREE_NEED_REDRAW|DIVTREE_ALL_REDRAW)) {
#ifdef DEBUG_VIDEO
    fprintf(stderr, "divnode_redraw\n");
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
    *x = (d->r.w-*w)/2;
    *y = (d->r.h-*h)/2;
    break;
  case PG_A_TOP:
    *x = (d->r.w-*w)/2;
    *y = 0;
    break;
  case PG_A_BOTTOM:
    *x = (d->r.w-*w)/2;
    *y = d->r.h-*h;
    break;
  case PG_A_LEFT:
    *y = (d->r.h-*h)/2;
    *x = 0;
    break;
  case PG_A_RIGHT:
    *y = (d->r.h-*h)/2;
    *x = d->r.w-*w;
    break;
  case PG_A_NW:
    *x = 0;
    *y = 0;
    break;
  case PG_A_NE:
    *x = d->r.w-*w;
    *y = 0;
    break;
  case PG_A_SW:
    *x = 0;
    *y = d->r.h-*h;
    break;
  case PG_A_SE:
    *x = d->r.w-*w;
    *y = d->r.h-*h;
    break;
  case PG_A_ALL:
    *x = 0;
    *y = 0;
    *w = d->r.w;
    *h = d->r.h;
    break;
  }
  *x += d->r.x;
  *y += d->r.y;
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
      div->split = max(div->div->preferred.h,div->div->child.h);
      break;
    case PG_S_LEFT:
    case PG_S_RIGHT: 
      div->split = max(div->div->preferred.w,div->div->child.w);
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

  old_cw = div->child.w;
  old_ch = div->child.h;

  /* Calculate child nodes' sizes */
	
  if (div->div) {
    divresize_recursive(div->div);
    dw = max(div->div->child.w,div->div->preferred.w);
    dh = max(div->div->child.h,div->div->preferred.h);
  }
  else
    dw = dh = 0;
  
  if (div->next) {
    divresize_recursive(div->next);
    nw = max(div->next->child.w,div->next->preferred.w);
    nh = max(div->next->child.h,div->next->preferred.h);
  }
  else
    nw = nh = 0;
  
  if (div->flags & DIVNODE_SIZE_RECURSIVE) {
    
    /* Combine them depending on orientation */
    switch (div->flags & ~SIDEMASK) {
      
    case PG_S_TOP:
    case PG_S_BOTTOM:
      div->child.h = dh + nh;
      div->child.w = max(dw,nw);
      break;
      
    case PG_S_LEFT:
    case PG_S_RIGHT:
      div->child.w = dw + nw;
      div->child.h = max(dh,nh);
      break;
      
    case PG_S_ALL:
    case DIVNODE_SPLIT_CENTER:
      div->child.w = dw;
      div->child.h = dh;
      break;
      
    case DIVNODE_SPLIT_BORDER:
      div->child.w = dw + (div->split<<1);
      div->child.h = dh + (div->split<<1);
      break;

    default:
      div->child.w = max(dw,nw);
      div->child.h = max(dh,nh);
      break;

    }

    if (old_cw != div->child.w || old_ch != div->child.h) {

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

/* Add an area to the update rectangle for this divtree */
void add_updarea(struct divtree *dt, s16 x,s16 y,s16 w,s16 h) {

  /* Clip to logical display */
  if (x<0) {
    w -= x;
    x = 0;
  }
  if (y<0) {
    h -= y;
    y = 0;
  }
  if ((x+w) > dt->head->r.w)
    w = dt->head->r.w - x;
  if ((y+h) > dt->head->r.h)
    h = dt->head->r.h - y;

  /* Is this a bogus update rectangle? */
  if (w<=0 || h<=0)
    return;

  if (dt->update_rect.w) {
    if (x < dt->update_rect.x) {
      dt->update_rect.w += dt->update_rect.x - x;
      dt->update_rect.x = x;
    }
    if (y < dt->update_rect.y) {
      dt->update_rect.h += dt->update_rect.y - y;
      dt->update_rect.y = y;
    }
    if ((w+x) > (dt->update_rect.x+dt->update_rect.w))
      dt->update_rect.w = w+x-dt->update_rect.x;
    if ((h+y) > (dt->update_rect.y+dt->update_rect.h))
      dt->update_rect.h = h+y-dt->update_rect.y;
  }
  else {
    dt->update_rect.x = x;
    dt->update_rect.y = y;
    dt->update_rect.w = w;
    dt->update_rect.h = h;
  }
}

/* Realize the update rectangle for this divtree */
void realize_updareas(struct divtree *dt) {
  if (dt->update_rect.w) {
    VID(update) (dt->display, 
		 dt->update_rect.x,
		 dt->update_rect.y,
		 dt->update_rect.w,
		 dt->update_rect.h);
    memset(&dt->update_rect,0,sizeof(dt->update_rect));
  } 
}

/* The End */












