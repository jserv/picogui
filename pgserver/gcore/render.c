/* $Id: render.c,v 1.26 2002/01/16 19:47:25 lonetech Exp $
 *
 * render.c - gropnode rendering engine. gropnodes go in, pixels come out :)
 *            The gropnode is clipped, translated, and otherwise mangled,
 *            then the appropriate vidlib call is made to put it on the
 *            screen (or where-ever it is supposed to go)
 * 
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

#include <pgserver/common.h>

#include <pgserver/render.h>
#include <pgserver/appmgr.h>    /* for defaultfont */

int display_owner;

/****************************************************** grop_render */

/* This renders a divnode's groplist to the screen
 *
 * Sets up a groprender structure based on the divnode's information,
 * performs pre-render housekeeping, processes flags, and renders each node
 */

void grop_render(struct divnode *div) {
   struct gropnode **listp = &div->grop;
   struct gropnode node;
   u8 incflag;
   struct groprender rend;
   s16 dtx,dty;

   /* Don't render if an app has exclusive display access */
   if (display_owner)
     return;

   /* default render values */
   memset(&rend,0,sizeof(rend));
   rend.lgop = PG_LGOP_NONE;
   rend.output = vid->display;
   rend.hfont = defaultfont;
   
   /* Add in the divnode-level scrolling if necessary */
   if ((div->flags & DIVNODE_DIVSCROLL) && div->divscroll) {
     dtx = div->divscroll->tx;
     dty = div->divscroll->ty;
     div->x = div->calcx + dtx;
     div->y = div->calcy + dty;
   }
   else {
     dtx = 0;
     dty = 0;
   } 
      
   /* Transfer over some numbers from the divnode */   
   rend.clip.x1 = div->x;
   rend.clip.x2 = div->x+div->w-1;
   rend.clip.y1 = div->y;
   rend.clip.y2 = div->y+div->h-1;
   rend.translation.x = div->tx;
   rend.translation.y = div->ty;
   rend.scroll.x = dtx - div->otx;
   rend.scroll.y = dty - div->oty;
   div->otx = dtx;
   div->oty = dty;
   rend.output_rect.x = div->x;
   rend.output_rect.y = div->y;
   rend.output_rect.w = div->w;
   rend.output_rect.h = div->h;

   /* Clip the clipping rectangle to the scrolling container */
   if ((div->flags & DIVNODE_DIVSCROLL) && div->divscroll) {
     if (rend.clip.x1 < div->divscroll->calcx)
       rend.clip.x1 = div->divscroll->calcx;
     if (rend.clip.x2 > (div->divscroll->calcx + div->divscroll->calcw - 1))
       rend.clip.x2 = div->divscroll->calcx + div->divscroll->calcw - 1;
     if (rend.clip.y1 < div->divscroll->calcy)
       rend.clip.y1 = div->divscroll->calcy;
     if (rend.clip.y2 > (div->divscroll->calcy + div->divscroll->calch - 1))
       rend.clip.y2 = div->divscroll->calcy + div->divscroll->calch - 1;
   }

   /* Munge our flags a bit. If this is incremental, and we didn't need
    * a full redraw anyway, look for the incremental divnode flag */
   if ((div->flags & DIVNODE_INCREMENTAL) && 
       !(div->flags & DIVNODE_NEED_REDRAW))
     incflag = PG_GROPF_INCREMENTAL;
   else
     incflag = 0;
   
   /* Scrolling updates */
   if ((div->flags & DIVNODE_SCROLL_ONLY) && 
       !(div->flags & DIVNODE_NEED_REDRAW))
     groplist_scroll(&rend,div);
   
   /* Get rid of any pesky sprites in the area. If this isn't incremental
    * go ahead and protect the whole divnode */
   if (!incflag) {
      VID(sprite_protectarea) (&rend.clip,spritelist);
      
      /* "dirty" this region of the screen so the blits notice it */
      add_updarea(rend.clip.x1,rend.clip.y1,rend.clip.x2-
		  rend.clip.x1+1,rend.clip.y2-rend.clip.y1+1);
   }

   /* Store our clipping rectangle before any 
    * PG_GROP_SETCLIP's can modify it.
    */
   rend.orig_clip = rend.clip;

   /* Begin rendering loop! */
   while (*listp) {
     
      /* Skip if the incremental-ness isn't right,
       * but not if it's pseudoincremental, transient, or universal */
      if ( (( (*listp)->flags &  PG_GROPF_INCREMENTAL) != incflag) &&
	   (!((*listp)->flags & (PG_GROPF_PSEUDOINCREMENTAL |
				 PG_GROPF_TRANSIENT |
				 PG_GROPF_UNIVERSAL))))
	goto skip_this_node;
      
      /* Clear pseudoincremental flag */
      (*listp)->flags &= ~PG_GROPF_PSEUDOINCREMENTAL;
 
      /* If this is a nonvisual gropnode, get it over with now! */
      if (PG_GROP_IS_NONVISUAL((*listp)->type)) {
	 gropnode_nonvisual(&rend,*listp);
	 goto skip_this_node;
      }	
      
      /* Make a local copy of the node so we can clip and transform its
       * coordinates and twiddle its flags */
      node = **listp;

      /* Do the mappings now, before we scroll and add divnode coordinates.
       * Mappings are always relative to the divnode, because they are often
       * not measured in pixels while the divnodes are always in pixels */
      gropnode_map(&rend,&node);
      
      /* Convert from divnode coordinates to screen coordinates */
      gropnode_translate(&rend,&node);
      
      /* Clip clip! */
      gropnode_clip(&rend,&node);

      /* Anything to do? */
      if (node.type == PG_GROP_NOP)
	goto skip_this_node;

      /* If this is incremental, do the sprite protection and double-buffer
       * update rectangle things for each gropnode because the updated area
       * is usually small compared to the whole
       */
      if (incflag) {
	 struct quad lcr;
	 if (node.type == PG_GROP_LINE) {
	    /* Lines are "special" */
	    int xx,yy,xx2,yy2;
	    if (node.r.w<0) {
	       xx2 = node.r.x;
	       xx = node.r.x+node.r.w;
	    }
	    else {
	       xx = node.r.x;
	       xx2 = node.r.x+node.r.w;
	    }
	    if (node.r.h<0) {
	       yy2 = node.r.y;
	       yy = node.r.y+node.r.h;
	    }
	    else {
	       yy = node.r.y;
	       yy2 = node.r.y+node.r.h;
	    }
	    
	    lcr.x1 = xx;
	    lcr.y1 = yy;
	    lcr.x2 = xx2;
	    lcr.y2 = yy2;
	    
	    /* "dirty" this region of the screen so the blits notice it */
	    add_updarea(xx,yy,xx2-xx+1,yy2-yy+1);
	 }
	 else {
	    lcr.x1 = node.r.x;
	    lcr.y1 = node.r.y;
	    lcr.x2 = node.r.x+node.r.w-1;
	    lcr.y2 = node.r.y+node.r.h-1;
	    
	    /* "dirty" this region of the screen so the blits notice it */
	    add_updarea(node.r.x,node.r.y,node.r.w,node.r.h);
	 }
	 
	 VID(sprite_protectarea) (&lcr,spritelist);	  
      }
     
      /* Draw the gropnode! */
      gropnode_draw(&rend,&node);
     
      /* Jump here when done rendering */
      skip_this_node:
     
      /* Delete the grop if it was transient */
      if ((*listp)->flags & PG_GROPF_TRANSIENT) {
	 struct gropnode *condemn;
	 condemn = *listp;
	 *listp = (*listp)->next;   /* close up the hole */
	 gropnode_free(condemn);
	 div->flags |= DIVNODE_GROPLIST_DIRTY;
      }
      else {
	 /* Otherwise just move on */
	 listp = &(*listp)->next;
      }
   }
}

/****************************************************** groplist_scroll */

/* Some utilities for groplist_scroll... */

/* Truncate the given clipping rectangle to include only the new
 * areas to draw when the specified scroll is processed
 */
void scroll_clip(struct quad *clip, struct pair *scroll) {
  s16 i;

  /* Vertical scroll */
  if (scroll->y) {

    /* Scroll the region up */
    if (scroll->y < 0) {
      i = clip->y1;
      clip->y1 = clip->y2 + scroll->y + 1;
      if (clip->y1<i)
	clip->y1 = i;
    }
    /* Scroll the region down */
    else {
      i = clip->y2;
      clip->y2 = clip->y1 + scroll->y - 1;
      if (clip->y2>i)
	clip->y2 = i;
    }

  }
}


void groplist_scroll(struct groprender *r, struct divnode *div) {

  if ((div->flags & DIVNODE_DIVSCROLL) && div->divscroll!=div) {
    /* This is not the scrollable divnode itself, it is a child. 
     * We don't need to do any blitting here, but we do need to draw in
     * the correct clipping rectangle
     */

    /* First, reproduce the truncated clipping rectangle used by the 
     * head scrolling node.
     */
    struct quad trclip;
    trclip.x1 = div->divscroll->calcx;
    trclip.y1 = div->divscroll->calcy;
    trclip.x2 = trclip.x1 + div->divscroll->calcw - 1;
    trclip.y2 = trclip.y1 + div->divscroll->calch - 1;
    scroll_clip(&trclip,&r->scroll);

    /* Clip ourselves to this also */
    if (r->clip.x1 < trclip.x1)
      r->clip.x1 = trclip.x1;
    if (r->clip.x2 > trclip.x2)
      r->clip.x2 = trclip.x2;
    if (r->clip.y1 < trclip.y1)
      r->clip.y1 = trclip.y1;
    if (r->clip.y2 > trclip.y2)
      r->clip.y2 = trclip.y2;

    r->clip = trclip;
  }
  else {
    /* This is the head scrolling node! We are in charge of clearing
     * all sprites out of the area and performing the scroll's blit
     */

    /* Prepare the whole area for drawing */
    VID(sprite_protectarea) (&r->clip,spritelist);
    add_updarea(r->clip.x1,r->clip.y1,r->clip.x2-
		r->clip.x1+1,r->clip.y2-r->clip.y1+1);

    /* Vertical scroll: blit up or down */
    if (r->scroll.y) {
      
      if (r->scroll.y < 0) {
	s16 h = r->clip.y2 - r->clip.y1 + 1 + r->scroll.y;
	/* If h<=0 the whole area is new, so we have nothing to blit */
	if (h>0)
	  VID(blit) (r->output,r->clip.x1,r->clip.y1,
		     r->clip.x2 - r->clip.x1 + 1,h,
		     r->output,r->clip.x1,r->clip.y1 - r->scroll.y,
		     PG_LGOP_NONE);
      }
      else {
	s16 h = r->clip.y2 - r->clip.y1 + 1 - r->scroll.y;
	/* If h<=0 the whole area is new, so we have nothing to blit */
	if (h>0)
	  VID(scrollblit) (r->output,r->clip.x1,r->clip.y1 + r->scroll.y,
			   r->clip.x2 - r->clip.x1 + 1,h,
			   r->output,r->clip.x1,r->clip.y1,
			   PG_LGOP_NONE);
      }
    }
    
    /* Truncate our own clipping rectangle */
    scroll_clip(&r->clip,&r->scroll);
  }
}


/****************************************************** gropnode_nonvisual */

void gropnode_nonvisual(struct groprender *r, struct gropnode *n) {
   switch (n->type) {
    
    case PG_GROP_SETCOLOR:
      r->color = n->param[0];
      break;

    case PG_GROP_SETLGOP:
      r->lgop = n->param[0];
      break;
      
    case PG_GROP_SETANGLE:
      r->angle = n->param[0];
      break;
      
    case PG_GROP_SETSRC:
      r->src = n->r;
      break;
      
    case PG_GROP_SETOFFSET:
      r->offset = n->r;
      break;

    case PG_GROP_SETFONT:
      r->hfont = n->param[0];
      break;
      
    case PG_GROP_SETMAPPING:
      r->map = n->r;
      r->maptype = n->param[0];
      break;

    case PG_GROP_SETCLIP:
      {
	struct gropnode node;

	/* Reset the clipping rectangle */
	r->clip = r->orig_clip;
	
	/* Make a local copy of the SETCLIP to map, translate, and clip */
	node = *n;
	gropnode_map(r,&node);       /* Map in divnode coordinates */
	gropnode_translate(r,&node); /* convert to logical coordinates */
	gropnode_clip(r,&node);      /* Clip to divnode */

	/* This is our new clip */
	r->clip.x1 = node.r.x;
	r->clip.y1 = node.r.y;
	r->clip.x2 = node.r.x + node.r.w - 1;
	r->clip.y2 = node.r.y + node.r.h - 1;
      }
      break;

   }
}

/****************************************************** gropnode_translate */

void gropnode_translate(struct groprender *r, struct gropnode *n) {
  /* Always draw relative to divnode, optionally add translation */
  n->r.x += r->output_rect.x;
  n->r.y += r->output_rect.y;
  if (n->flags & PG_GROPF_TRANSLATE) {
    n->r.x += r->translation.x;
    n->r.y += r->translation.y;
  }
}

/****************************************************** gropnode_map */

/* Utility to scale and translate a gropnode */
void gropnode_scaletranslate(struct groprender *r, struct gropnode *n,
			     struct fractionpair *scale, struct pair *trans) {
  if(n->type==PG_GROP_FPOLYGON) {

    /**** FIXME: Polygons can't yet be scaled.
     *           the below code doesn't work because it would modify
     *           the polygon's array permanently every time it's drawn
  
    s32* arr;
    int i;
    if (iserror(rdhandle((void**)&arr,PG_TYPE_ARRAY,-1,
			 n->param[0])) || !arr) break;
    for(i=1;i<=arr[0];i+=2) {
      arr[i  ] = arr[i  ] * scale->x.n / scale->x.d + trans->x;
      arr[i+1] = arr[i+1] * scale->y.n / scale->y.d + trans->y;
    }
    n->r.x = n->r.x * scale->x.n / scale->x.d;
    n->r.y = n->r.y * scale->y.n / scale->y.d;

    */
  } 
  else {
    n->r.x = n->r.x * scale->x.n / scale->x.d + trans->x;
    n->r.y = n->r.y * scale->y.n / scale->y.d + trans->y;
    if (n->type != PG_GROP_TEXT) {
      if (n->type != PG_GROP_BAR)
	n->r.w = n->r.w * scale->x.n / scale->x.d;
      if (n->type != PG_GROP_SLAB)
	n->r.h = n->r.h * scale->y.n / scale->y.d;
    }
  }
}

/* NOTE: When new mapping types are added, add input mappings
 * to canvas.c! */

void gropnode_map(struct groprender *r, struct gropnode *n) {
  struct fractionpair scale;
  struct pair trans;

  switch (r->maptype) {
  case PG_MAP_NONE:
    break;

    /* Simple scaling, stretch the virtual canvas to
     * the entire physical canvas.
     */
  case PG_MAP_SCALE:
    trans.x = r->map.x;
    trans.y = r->map.y;
    scale.x.n = r->output_rect.w;
    scale.x.d = r->map.w;
    scale.y.n = r->output_rect.h;
    scale.y.d = r->map.h;
    gropnode_scaletranslate(r,n,&scale,&trans);
    break;

    /* This mapping preserves the virtual canvas's aspect
     * ratio and centers it within the actual canvas.
     * Determine which scale parameter will be smaller,
     * and assign that scale to both axes.
     */
  case PG_MAP_SQUARESCALE:
    if (r->output_rect.w * r->map.h - r->output_rect.h * r->map.w > 0) {
      /* Center horizontally */
      scale.x.n = r->output_rect.h;
      scale.x.d = r->map.h;
      trans.x = (r->output_rect.w - r->map.w * r->output_rect.h / r->map.h) >> 1;
      trans.y = 0;
    }
    else {
      /* Center vertically */
      scale.x.n = r->output_rect.w;
      scale.x.d = r->map.w;
      trans.x = 0;
      trans.y = (r->output_rect.h - r->map.h * r->output_rect.w / r->map.w) >> 1;
    }
    scale.y = scale.x;
    gropnode_scaletranslate(r,n,&scale,&trans);
    break;

    
  }

  /* Add mapping offset */
  n->r.x += r->offset.x;
  n->r.y += r->offset.y;
  n->r.w += r->offset.w;
  n->r.h += r->offset.h;
}

/****************************************************** gropnode_clip */

void gropnode_clip(struct groprender *r, struct gropnode *n) {
    /* Clip - clipping serves two purposes:
     *   1. security, an app/widget stays in its allocated space
     *   2. scrolling needs to be able to update any arbitrary
     *      slice of an area
     */

   /* Save node's coordinates before clipping */
   r->orig = n->r;    

   r->csrc.x = 0;
   r->csrc.y = 0;
   switch (n->type) {
      
      /* Text clipping is handled by the charblit, but we can
       * go ahead and handle a few cases on the string level */
    case PG_GROP_TEXT:
      switch (r->angle) {
	 
       case 0:
	 if (n->r.x>r->clip.x2 || n->r.y>r->clip.y2)
	   goto skip_this_node;
	 break;
	 
       case 90:
	 if (n->r.y<r->clip.y1)
	   goto skip_this_node;
	 break;
	 
      }
      break;

      /* Similar deal with textgrid, easier to handle clipping later */
    case PG_GROP_TEXTGRID:
      if (n->r.x>r->clip.x2 || n->r.y>r->clip.y2)
	goto skip_this_node;
      break;
	 
    case PG_GROP_LINE:
      
      /* Is this line just completely out there? */
      if ( ((n->r.x<r->clip.x1) && ((n->r.x+n->r.w)<r->clip.x1)) ||
	  ((n->r.x>r->clip.x2) && ((n->r.x+n->r.w)>r->clip.x2)) ||
	  ((n->r.y<r->clip.y1) && ((n->r.y+n->r.h)<r->clip.y1)) ||
	  ((n->r.y>r->clip.y2) && ((n->r.y+n->r.h)>r->clip.y2)) )
	goto skip_this_node;
            
      if (n->r.w && n->r.h) {        /* Not horizontal or vertical */
	 int t,u;
	 
	 /* A real line - clip, taking slope into account */
	 
	 if (n->r.x<r->clip.x1) {               /* Point 1 left of clip */
	    t = r->clip.x1 - n->r.x;
	    u = t * n->r.h / n->r.w;
	    n->r.y += u;
	    n->r.h -= u;
	    n->r.w -= t;
	    n->r.x = r->clip.x1;
	 }
	 
	 else if (n->r.x>r->clip.x2) {          /* Point 1 right of clip */
	    t = n->r.x - r->clip.x2;
	    u = t * n->r.h / n->r.w;
	    n->r.y -= u;
	    n->r.h += u;
	    n->r.w += t;
	    n->r.x = r->clip.x2;
	 }
	 
	 if (n->r.y<r->clip.y1) {               /* Point 1 above clip */
	    t = r->clip.y1 - n->r.y;
	    if (!n->r.h) goto skip_this_node;   /* Avoid divide by zero */
	    u = t * n->r.w / n->r.h;
	    n->r.x += u;
	    n->r.w -= u;
	    n->r.h -= t;
	    n->r.y = r->clip.y1;
	 }
	 
	 else if (n->r.y>r->clip.y2) {          /* Point 1 below clip */
	    t = n->r.y - r->clip.y2;
	    if (!n->r.h) goto skip_this_node;   /* Avoid divide by zero */
	    u = t * n->r.w / n->r.h;
	    n->r.x -= u;
	    n->r.w += u;
	    n->r.h += t;
	    n->r.y = r->clip.y2;
	 }
	 
	 if ((n->r.x+n->r.w)<r->clip.x1) {           /* Point 2 left of clip */
	    t = r->clip.x1 - n->r.x - n->r.w;
	    if (!n->r.w) goto skip_this_node;   /* Avoid divide by zero */
	    n->r.h += t * n->r.h / n->r.w;
	    n->r.w = r->clip.x1-n->r.x;
	 }
	 
	 else if ((n->r.x+n->r.w)>r->clip.x2) {      /* Point 2 right of clip */
	    t = n->r.x + n->r.w - r->clip.x2;
	    if (!n->r.w) goto skip_this_node;   /* Avoid divide by zero */
	    n->r.h -= t * n->r.h / n->r.w;
	    n->r.w = r->clip.x2-n->r.x;
	 }
	 
	 if ((n->r.y+n->r.h)<r->clip.y1) {           /* Point 2 above clip */
	    t = r->clip.y1 - n->r.y - n->r.h;
	    if (!n->r.h) goto skip_this_node;   /* Avoid divide by zero */
	    n->r.w += t * n->r.w / n->r.h;
	    n->r.h = r->clip.y1-n->r.y;
	 }
	 
	 else if ((n->r.y+n->r.h)>r->clip.y2) {      /* Point 2 below clip */
	    t = n->r.y + n->r.h - r->clip.y2;
	    if (!n->r.h) goto skip_this_node;   /* Avoid divide by zero */
	    n->r.w -= t * n->r.w / n->r.h;
	    n->r.h = r->clip.y2-n->r.y;
	 }
	 
	 /* If the line's endpoints are no longer within the clipping
	  * rectangle, it means the line never intersected it in the
	  * first place */
	 
	 if ( (n->r.x<r->clip.x1) || (n->r.x>r->clip.x2) || 
	     (n->r.y<r->clip.y1) || (n->r.y>r->clip.y2) ||
	     ((n->r.x+n->r.w)<r->clip.x1) || ((n->r.x+n->r.w)>r->clip.x2) ||
	     ((n->r.y+n->r.h)<r->clip.y1) || ((n->r.y+n->r.h)>r->clip.y2) )
	   goto skip_this_node;
	 
	 break;
      }
      
      else {
	 /* It's horizontal or vertical. Do a little prep, then
	  * let it -fall through- to the normal clipper */
	 
	 if (n->r.w) {
	    n->r.h=1;   
	    if (n->r.w<0) {
	       n->r.x += n->r.w;
	       n->r.w = -n->r.w;
	    }
	    n->r.w++;
	    n->type = PG_GROP_SLAB;
	 }
	 
	 else {
	    n->r.w=1;
	    if (n->r.h<0) {
	       n->r.y += n->r.h;
	       n->r.h = -n->r.h;
	    }
	    n->r.h++;
	    n->type = PG_GROP_BAR;
	 }
	 
	 /* ... and fall through to default */
      }
      /* Default clipping just truncates */
    default:
      if (n->r.x<r->clip.x1) {
	 n->r.w -= r->csrc.x = r->clip.x1 - n->r.x;
	 n->r.x = r->clip.x1;
      }
      if (n->r.y<r->clip.y1) {
	 n->r.h -= r->csrc.y = r->clip.y1 - n->r.y;
	 n->r.y = r->clip.y1;
      }
      if ((n->r.x+n->r.w-1)>r->clip.x2)
	n->r.w = r->clip.x2-n->r.x+1;
      if ((n->r.y+n->r.h-1)>r->clip.y2)
	n->r.h = r->clip.y2-n->r.y+1;
   }

   if ((n->r.w <= 0 || n->r.h <= 0) && n->type!=PG_GROP_LINE)
     goto skip_this_node;

   return;
   skip_this_node:
   n->type = PG_GROP_NOP;
}
 
/****************************************************** gropnode_draw */

void gropnode_draw(struct groprender *r, struct gropnode *n) {
   char *str;
   u32 *arr;
   hwrbitmap bit;
   s16 bw,bh;
   hwrcolor c;
   struct fontdesc *fd;

   /* Normally get color from r->color, but if this gropnode has 
      PG_GROPF_COLORED get a hwrcolor from the grop's param[0] */
   if (n->flags & PG_GROPF_COLORED)
     c = n->param[0];
   else
     c = r->color;
   
   switch (n->type) {

    case PG_GROP_PIXEL:
      VID(pixel) (r->output,
		  n->r.x,
		  n->r.y,
		  c,
		  r->lgop);
      break;
      
    case PG_GROP_LINE:
      VID(line) (r->output,
		 n->r.x,
		 n->r.y,
		 n->r.w+n->r.x,
		 n->r.h+n->r.y,
		 c,
		 r->lgop);
      break;

    case PG_GROP_RECT:
      VID(rect) (r->output,
		 n->r.x,
		 n->r.y,
		 n->r.w,
		 n->r.h,
		 c,
		 r->lgop);
      break;
   
    case PG_GROP_FRAME:
       
       /* Bogacious clipping cruft for frames. A clipped frame is no
	* longer a frame, so clip it as it is separated into slabs and bars */
       if (r->orig.y>=r->clip.y1 && r->orig.y<=r->clip.y2 ) 
	 VID(slab) (r->output,n->r.x,n->r.y,n->r.w,c,r->lgop);
       if ((r->orig.y+r->orig.h-1)>=r->clip.y1 && (r->orig.y+r->orig.h-1)<=r->clip.y2)
	 VID(slab) (r->output,n->r.x,n->r.y+n->r.h-1,n->r.w,c,r->lgop);
       if (r->orig.x>=r->clip.x1 && r->orig.x<=r->clip.x2)
	 VID(bar) (r->output,n->r.x,n->r.y,n->r.h,c,r->lgop);
       if ((r->orig.x+r->orig.w-1)>=r->clip.x1 &&
	   (r->orig.x+r->orig.w-1)<=r->clip.x2)
	 VID(bar) (r->output,n->r.x+n->r.w-1,n->r.y,n->r.h,c,r->lgop);
       break;
       
    case PG_GROP_SLAB:
      VID(slab) (r->output,
		 n->r.x,
		 n->r.y,
		 n->r.w,
		 c,
		 r->lgop);
      break;
      
    case PG_GROP_BAR:
      VID(bar) (r->output,
		n->r.x,
		n->r.y,
		n->r.h,
		c,
		r->lgop);
      break;
      
    case PG_GROP_TEXT:
      if (iserror(rdhandle((void**)&str,PG_TYPE_STRING,-1,
			   n->param[0])) || !str) break;
      if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
			   r->hfont)) || !fd) break;
      outtext(r->output,fd,n->r.x,n->r.y,c,str,&r->clip,
	      r->lgop,r->angle);
      break;

      /* The workhorse of the terminal widget. 3 params:
       * 1. buffer handle,
       * 2. buffer width and offset (0xWWWWOOOO)
       * 3. handle to textcolors array
       */
    case PG_GROP_TEXTGRID:
      if (iserror(rdhandle((void**)&str,PG_TYPE_STRING,-1,
			    n->param[0])) || !str) break;
      if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
			   r->hfont)) || !fd) break;
	{
	   int buffersz,bufferw,bufferh;
	   int celw,celh,charw,charh,offset;
	   int i;
	   unsigned char attr;
	   u32 *textcolors;
	   s16 temp_x;
	   struct gropnode bn;
	   struct groprender br;
	   
	   /* Set up background color node (we'll need it later) */
	   memset(&bn,0,sizeof(bn));
	   bn.type = PG_GROP_RECT;
	   bn.flags = PG_GROPF_COLORED;

	   /* Read textcolors parameter */
	   if (iserror(rdhandle((void**)&textcolors,PG_TYPE_PALETTE,-1,
				n->param[2])) || !textcolors) break;
	   if (textcolors[0] < 16)    /* Make sure it's big enough */
	     break;
	   textcolors++;              /* Skip length entry */
	   
	   /* Should be fine for fixed width fonts
	    * and pseudo-acceptable for others? */
	   celw      = fd->font->w;  
	   celh      = fd->font->h;
	   
	   bufferw   = n->param[1] >> 16;
	   buffersz  = strlen(str) - (n->param[1] & 0xFFFF);
	   str      += n->param[1] & 0xFFFF;
	   bufferh   = (buffersz / bufferw) >> 1;
	   if (buffersz<=0) return;
	   
	   charw     = n->r.w/celw;
	   charh     = n->r.h/celh;
	   offset    = (bufferw-charw)<<1;
	   if (offset<0) {
	      offset = 0;
	      charw = bufferw;
	   }
	   if (charh>bufferh)
	     charh = bufferh;
	   
	   r->orig.x = n->r.x;
	   for (;charh;charh--,n->r.y+=celh,str+=offset) {

	     /* Skip the entire line if it's clipped out */
	     if (n->r.y > r->clip.y2 ||
		 (n->r.y+celh) < r->clip.y1) {
	       str += charw << 1;
	       continue;
	     }

	     for (n->r.x=r->orig.x,i=charw;i;i--,str++) {
		attr = *(str++);

		/* Background color (clipped rectangle) */
		if ((attr & 0xF0)!=0) {
		  br = *r;
		  bn.r.x = n->r.x;
		  bn.r.y = n->r.y;
		  bn.r.w = celw;
		  bn.r.h = celh;
		  bn.param[0] = textcolors[attr>>4];
		  gropnode_clip(&br,&bn);
		  gropnode_draw(&br,&bn);
		}

		temp_x = n->r.x;
		n->r.x += celw;
		outchar(r->output, fd, &temp_x, &n->r.y, 
			textcolors[attr & 0x0F],
			*str, &r->clip,r->lgop, 0);
	     }
	   }
	}
      break;      
      
    case PG_GROP_BITMAP:
      if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			   n->param[0])) || !bit) break;
      VID(bitmap_getsize) (bit,&bw,&bh);
      VID(blit) (r->output,n->r.x,n->r.y,n->r.w,n->r.h,bit,
		 (r->src.x+r->csrc.x)%bw,
		 (r->src.y+r->csrc.y)%bh,r->lgop);
		 
      break;
   
    case PG_GROP_TILEBITMAP:
      if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			   n->param[0])) || !bit) break;
      VID(tileblit) (r->output,n->r.x,n->r.y,n->r.w,n->r.h,bit,
		     r->src.x+r->csrc.x,
		     r->src.y+r->csrc.y,
		     r->src.w,
		     r->src.h,
		     r->lgop);
		     
      break;
      
    case PG_GROP_GRADIENT:
      /* Gradients are fun! */
      VID(gradient) (r->output,
		     n->r.x,
		     n->r.y,
		     n->r.w,
		     n->r.h,
		     n->param[0],
		     n->param[1],
		     n->param[2],
		     r->lgop);      
      break;
   case PG_GROP_ELLIPSE: 
     VID(ellipse) (r->output, 
           n->r.x, 
           n->r.y, 
           n->r.w, 
           n->r.h, 
           c, 
           r->lgop 
           ); 
     break; 
   case PG_GROP_FELLIPSE: 
     VID(fellipse) (r->output, 
            n->r.x, 
            n->r.y, 
            n->r.w, 
            n->r.h, 
            c, 
            r->lgop 
            ); 
     break; 

   case PG_GROP_VIDUPDATE:
     if (r->output == vid->display)
       VID(update)(n->r.x,n->r.y,n->r.w,n->r.h);
     break;
	
   case PG_GROP_FPOLYGON:
     if (iserror(rdhandle((void**)&arr,PG_TYPE_ARRAY,-1,
			  n->param[0])) || !arr) break;
     VID(fpolygon) (r->output,
 		    arr,
		    n->r.x,
 		    n->r.y,
		    c,
		    r->lgop
 		    );
     break;
     
		  
   }
}


/* The End */

