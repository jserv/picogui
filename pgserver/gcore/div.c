/* $Id: div.c,v 1.36 2001/03/17 04:16:34 micahjd Exp $
 *
 * div.c - calculate, render, and build divtrees
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
#include <pgserver/widget.h>

/* Fill in the x,y,w,h of this divnode's children node based on it's
 * x,y,w,h and it's split. Also rebuilds child divnodes.
 * Recurse into all the node's children.
 */
void divnode_recalc(struct divnode *n) {
   int split;

   if (!n) return;

   if (n->flags & DIVNODE_NEED_RECALC) {
     split = n->split;

     /* All available space for div */
     if (n->flags & DIVNODE_SPLIT_EXPAND) {
       if (n->div) {
	 n->div->x = n->x;
	 n->div->y = n->y;
	 n->div->w = n->w;
	 n->div->h = n->h;
       }       
       if (n->next) {
	 n->next->x = n->x;
	 n->next->y = n->y;
	 n->next->w = 0;
	 n->next->h = 0;
       }       
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
	
       if (split>n->h) split = n->h;
       if (n->div) {
	 n->div->x = n->x;
	 n->div->w = n->w;
       }
       if (n->next) {
	 n->next->x = n->x;
	 n->next->w = n->w;
       }
       if (n->flags & DIVNODE_SPLIT_TOP) {
	 if (n->div) {
	   n->div->y = n->y;
	   n->div->h = split;
	 }
	 if (n->next) { 
	   n->next->y = n->y+split;
	   n->next->h = n->h-split;
	 }
       }
       else {
	 if (n->div) {
	   n->div->y = n->y+n->h-split;
	   n->div->h = split;
	 }
	 if (n->next) {
	   n->next->y = n->y;
	   n->next->h = n->h-split;
	 }
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

       if (split>n->w) split = n->w;
       if (n->div) {
	 n->div->y = n->y;
	 n->div->h = n->h;
       }
       if (n->next) {
	 n->next->y = n->y;
	 n->next->h = n->h;
       }
       if (n->flags & DIVNODE_SPLIT_LEFT) {
	 if (n->div) {
	   n->div->x = n->x;
	   n->div->w = split;
	 }
	 if (n->next) {
	   n->next->x = n->x+split;
	   n->next->w = n->w-split;
	 }
       }
       else {
	 if (n->div) {
	   n->div->x = n->x+n->w-split;
	   n->div->w = split;
	 }
	 if (n->next) {
	   n->next->x = n->x;
	   n->next->w = n->w-split;
	 }
       }
     }
     
     /* Center the 'div' node in this one. If a 'next' node exists,
      * it has the same coords as this one */
     else if (n->flags & DIVNODE_SPLIT_CENTER) {
       if (n->div) {
	 n->div->x = n->x+(n->w-n->div->w)/2;
	 n->div->y = n->y+(n->h-n->div->h)/2;
       }
       if (n->next) {
	 n->next->x = n->x;
	 n->next->y = n->y;
	 n->next->w = n->w;
	 n->next->h = n->h;
       }
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

       if (n->div) {
	 n->div->x = n->x+split;
	 n->div->y = n->y+split;
	 n->div->w = n->w-split*2;
	 n->div->h = n->h-split*2;
       }
       if (n->next) {
	 n->next->x = n->x;
	 n->next->y = n->y;
	 n->next->w = n->w;
	 n->next->h = n->h;
       }
     }

     /* Otherwise give children same w,h,x,y */
     else if (!(n->flags & DIVNODE_SPLIT_IGNORE)) {
       if (n->next) {
	 n->next->x = n->x;
	 n->next->y = n->y;
	 n->next->w = n->w;
	 n->next->h = n->h;
       }
       if (n->div) {
	 n->div->x = n->x;
	 n->div->y = n->y;
	 n->div->w = n->w;
	 n->div->h = n->h;
       }
     }
     
     /* Recalc completed.  Propagate the changes- always propagate to
	div, only propagate to next if our changes affected other nodes. 
      */
     if (n->div) {
       n->div->flags |= DIVNODE_NEED_RECALC | (n->flags & DIVNODE_PROPAGATE_RECALC);
       div_rebuild(n->div);
     }     
     if ((n->flags & DIVNODE_PROPAGATE_RECALC) && n->next) {
       n->next->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
       div_rebuild(n->next);
     }
     
     /* We're done */
     n->flags &= ~(DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC);
   }

   /* A child node might need a recalc even if we aren't forcing one */
   divnode_recalc(n->div);
   divnode_recalc(n->next);
}

/* Redraw the divnodes if they need it.
 * Recursive. Does not call hwr_update
 */
void divnode_redraw(struct divnode *n,int all) {
   if (!n) return;

   if (all || (n->flags & (DIVNODE_NEED_REDRAW | 
			   DIVNODE_SCROLL_ONLY | 
			   DIVNODE_INCREMENTAL) )) { 
     grop_render(n);
     if (n->flags & DIVNODE_PROPAGATE_REDRAW)
       if (n->next)
	 n->next->flags |= DIVNODE_NEED_REDRAW | DIVNODE_PROPAGATE_REDRAW;
     if (n->div)
       n->div->flags |= DIVNODE_NEED_REDRAW | DIVNODE_PROPAGATE_REDRAW;
     n->flags &= ~(DIVNODE_NEED_REDRAW | DIVNODE_PROPAGATE_REDRAW |
		   DIVNODE_SCROLL_ONLY | DIVNODE_INCREMENTAL);
   }

   divnode_redraw(n->next,all);
   divnode_redraw(n->div,all);
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
  (*p)->flags = DIVNODE_NEED_RECALC;
  (*p)->owner = owner;
  return sucess;
}

/* Make a new divtree */
g_error divtree_new(struct divtree **dt) {
  g_error e;
  e = g_malloc((void **) dt,sizeof(struct divtree));
  memset(*dt,0,sizeof(struct divtree));
  errorcheck;
  e = newdiv(&(*dt)->head,NULL);
  errorcheck;
  (*dt)->head->w = vid->xres;
  (*dt)->head->h = vid->yres;
  (*dt)->flags = DIVTREE_ALL_REDRAW;
  return sucess;
}

/* Delete a divtree */
void divtree_free(struct divtree *dt) {
  r_divnode_free(dt->head);
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
  if (subtree) {
    /* Subtree update */
    
    if (subtree->owner->dt != dts->top) return;

    divnode_recalc(subtree);
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
  printf("****************** Update (sub: 0x%08X)\n",subtree);
#endif
}

/* Update the divtree's calculations and render (both only if necessary) */
void r_dtupdate(struct divtree *dt) {

  /* Update the layers below only if this is a _complete_ update. */
  if (dt->next && (dt->next->flags & DIVTREE_ALL_REDRAW)) 
    r_dtupdate(dt->next);

  /* Draw on the way back up from the recursion, so the layers appear
     in the right order */

  if (dt->flags & DIVTREE_NEED_RECALC) {
#ifdef DEBUG_VIDEO
    printf("divnode_recalc\n",dt->head);
#endif
    divnode_recalc(dt->head);
    /* If we recalc, at least one divnode will need redraw */
    dt->flags |= DIVTREE_NEED_REDRAW;
  }
  if (dt->flags &(DIVTREE_NEED_REDRAW|DIVTREE_ALL_REDRAW)) {
#ifdef DEBUG_VIDEO
    printf("divnode_redraw\n");
#endif
    divnode_redraw(dt->head,dt->flags & DIVTREE_ALL_REDRAW);
  }

  /* All clean now, clear flags. */
  dt->flags &= ~(DIVTREE_ALL_REDRAW | DIVTREE_NEED_REDRAW | 
		 DIVTREE_NEED_RECALC);
}

/*********** Functions for managing the dtstack */

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

  return sucess;
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

/* dts_push creates a new layer, and dts_pop discards the top layer, and
   sets the DIVTREE_ALL_REDRAW flag on the new top tree
*/

g_error dts_push(void) {
  g_error e;
  struct divtree *t;

  e = divtree_new(&t);
  errorcheck;

  t->next = dts->top;
  dts->top = t;

  return sucess;
}

void dts_pop(void) {
  struct divtree *condemn;

  reset_pointer();
  condemn = dts->top;
  dts->top = dts->top->next;
  divtree_free(condemn);
}

/* Aligns a 'thing' of specified width and height in the specified divnode
 * according to the alignment type specified in align.
 */
void align(struct gropctxt *d,alignt align,int *w,int *h,int *x,int *y) {
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

/* The End */












