/* $Id$
 *
 * render.c - gropnode rendering engine. gropnodes go in, pixels come out :)
 *            The gropnode is clipped, translated, and otherwise mangled,
 *            then the appropriate vidlib call is made to put it on the
 *            screen (or where-ever it is supposed to go)
 * 
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

#include <pgserver/common.h>

#include <pgserver/render.h>
#include <pgserver/appmgr.h>    /* for res[PGRES_DEFAULT_FONT] */
#include <pgserver/pgstring.h>
#include <pgserver/paragraph.h>
#include <pgserver/widget.h>

int display_owner;
int disable_output;   /* can be used by the video driver to disable rendering,
		       * if pgserver is minimized or on a different terminal
		       */

/* Primitives used in clipping, return 1 to discard the gropnode */
int gropnode_line_clip(struct groprender *r, struct gropnode *n);
void gropnode_rect_clip(struct groprender *r, struct gropnode *n);

/****************************************************** grop_render */

/* Final calculations for the given divnode, and render
 * its groplist to the screen
 *
 * Sets up a groprender structure based on the divnode's information,
 * performs pre-render housekeeping, processes flags, and renders each node
 */

void grop_render(struct divnode *div, struct pgquad *clip) {
  struct gropnode **listp = &div->grop;
  struct gropnode node;
  u8 incflag;
  struct groprender rend;
  s16 dtx,dty;

  /* Don't render if an app has exclusive display 
   * access, or we have nothing to render 
   */
  if (display_owner || disable_output || !div->owner)
    return;

  /* default render values */
  memset(&rend,0,sizeof(rend));
  rend.lgop = PG_LGOP_NONE;
  rend.hfont = res[PGRES_DEFAULT_FONT];
  rend.output = div->owner->dt->display;
   
  /* Allow the video driver to override */
  if (!VID(grop_render_presetup_hook)(&div,&listp,&rend)) {

    /* Add in the divnode-level scrolling if necessary */
    if ((div->flags & DIVNODE_DIVSCROLL) && div->divscroll) {
      dtx = div->divscroll->translation.x;
      dty = div->divscroll->translation.y;
      div->r.x = div->calc.x + dtx;
      div->r.y = div->calc.y + dty;
    }
    else {
      dtx = 0;
      dty = 0;
    } 
     
    /* Transfer over some numbers from the divnode */   
    if (clip)
      rend.clip = *clip;
    else
      rend.clip = *rect_to_quad(&div->r);
    rend.translation = div->translation;
    rend.scroll.x = dtx - div->old_translation.x;
    rend.scroll.y = dty - div->old_translation.y;
    div->old_translation.x = dtx;
    div->old_translation.y = dty;
    rend.output_rect = div->r;
     
    /* Clip the clipping rectangle to the scrolling container */
    if ((div->flags & DIVNODE_DIVSCROLL) && div->divscroll)
      quad_intersect(&rend.clip, rect_to_quad(&div->divscroll->calc));
     
    /* Scrolling updates */
    if ((div->flags & DIVNODE_SCROLL_ONLY) && 
	!(div->flags & DIVNODE_NEED_REDRAW))
      groplist_scroll(&rend,div);
  }     
   
   
  /* Munge our flags a bit. If this is incremental, and we didn't need
   * a full redraw anyway, look for the incremental divnode flag */
  if ((div->flags & DIVNODE_INCREMENTAL) && 
      !(div->flags & DIVNODE_NEED_REDRAW))
    incflag = PG_GROPF_INCREMENTAL;
  else
    incflag = 0;
   
  /* Get rid of any pesky sprites in the area. If this isn't incremental
   * go ahead and protect the whole divnode */
  if (!incflag) {
    VID(sprite_protectarea) (&rend.clip,spritelist);
     
    /* "dirty" this region of the screen so the blits notice it */
    add_updarea(div->owner->dt,
		rend.clip.x1,rend.clip.y1,rend.clip.x2-
		rend.clip.x1+1,rend.clip.y2-rend.clip.y1+1);
  }

  /* Store our clipping rectangle before any 
   * PG_GROP_SETCLIP's can modify it.
   */
  rend.orig_clip = rend.clip;
   
  /* Let the video driver know before we start drawing */
  if (VID(grop_render_postsetup_hook)(&div,&listp,&rend))
    return;

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
     
    /* Let the video driver do its own transformations */
    if (!VID(grop_render_node_hook)(&div,&listp,&rend,&node)) {
       
      /* Do the mappings now, before we scroll and add divnode coordinates.
       * Mappings are always relative to the divnode, because they are often
       * not measured in pixels while the divnodes are always in pixels */
      gropnode_map(&rend,&node);
       
      /* Convert from divnode coordinates to screen coordinates */
      gropnode_translate(&rend,&node);
       
      /* Clip clip! */
      gropnode_clip(&rend,&node);
    }     

    /* Anything to do? */
    if (node.type == PG_GROP_NOP)
      goto skip_this_node;
     
    /* If this is incremental, do the sprite protection and double-buffer
     * update rectangle things for each gropnode because the updated area
     * is usually small compared to the whole
     */
    if (incflag) {
      struct pgquad lcr;
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
	add_updarea(div->owner->dt,xx,yy,xx2-xx+1,yy2-yy+1);
      }
      else {
	lcr.x1 = node.r.x;
	lcr.y1 = node.r.y;
	lcr.x2 = node.r.x+node.r.w-1;
	lcr.y2 = node.r.y+node.r.h-1;
	 
	/* "dirty" this region of the screen so the blits notice it */
	add_updarea(div->owner->dt,node.r.x,node.r.y,node.r.w,node.r.h);
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

  /* Let the video driver know we're done */
  VID(grop_render_end_hook)(&div,&listp,&rend);
}

/****************************************************** groplist_scroll */

/* Some utilities for groplist_scroll... */

/* Truncate the given clipping rectangle to include only the new
 * areas to draw when the specified scroll is processed
 */
void scroll_clip(struct pgquad *clip, struct pgpair *scroll) {
  s16 i;

  /* Scroll the region up */
  if (scroll->y < 0) {
    i = clip->y1;
    clip->y1 = clip->y2 + scroll->y + 1;
    if (clip->y1<i)
      clip->y1 = i;
  }

  /* Scroll the region down */
  if (scroll->y > 0) {
    i = clip->y2;
    clip->y2 = clip->y1 + scroll->y - 1;
    if (clip->y2>i)
      clip->y2 = i;
  }

  /* Scroll the region left */
  if (scroll->x < 0) {
    i = clip->x1;
    clip->x1 = clip->x2 + scroll->x + 1;
    if (clip->x1<i)
      clip->x1 = i;
  }

  /* Scroll the region right */
  if (scroll->x > 0) {
    i = clip->x2;
    clip->x2 = clip->x1 + scroll->x - 1;
    if (clip->x2>i)
      clip->x2 = i;
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
    struct pgquad trclip;

    trclip = *rect_to_quad(&div->divscroll->calc);
    scroll_clip(&trclip,&r->scroll);

    /* Clip ourselves to this also */
    quad_intersect(&r->clip,&trclip);

    return;
  }
  
  /* This is the head scrolling node! We are in charge of clearing
   * all sprites out of the area and performing the scroll's blit
   */
  
  /* Prepare the whole area for drawing */
  VID(sprite_protectarea) (&r->clip,spritelist);
  add_updarea(div->owner->dt,
	      r->clip.x1,r->clip.y1,r->clip.x2-
	      r->clip.x1+1,r->clip.y2-r->clip.y1+1);

  /* Now shift the existing pixels to where they need to be.
   * We use scrollblit since the source and destination rectangles
   * will overlap. Note that if w or h <= 0 below there are no pixels saved.
   */
 
  /* Scroll the region up */
  if (r->scroll.y < 0) {
    s16 h = r->clip.y2 - r->clip.y1 + 1 + r->scroll.y;
    if (h>0)
      VID(scrollblit) (r->output,r->clip.x1,r->clip.y1,
		       r->clip.x2 - r->clip.x1 + 1,h,
		       r->output,r->clip.x1,r->clip.y1 - r->scroll.y,
		       PG_LGOP_NONE);
  }

  /* Scroll the region down */
  if (r->scroll.y > 0) {
    s16 h = r->clip.y2 - r->clip.y1 + 1 - r->scroll.y;
    if (h>0)
      VID(scrollblit) (r->output,r->clip.x1,r->clip.y1 + r->scroll.y,
		       r->clip.x2 - r->clip.x1 + 1,h,
		       r->output,r->clip.x1,r->clip.y1,
		       PG_LGOP_NONE);
  }

  /* Scroll the region left */
  if (r->scroll.x < 0) {
    s16 w = r->clip.x2 - r->clip.x1 + 1 + r->scroll.x;
    if (w>0)
      VID(scrollblit) (r->output,r->clip.x1,r->clip.y1,
		       w,r->clip.y2 - r->clip.y1 + 1,
		       r->output,r->clip.x1 - r->scroll.x,r->clip.y1,
		       PG_LGOP_NONE);
  }

  /* Scroll the region right */
  if (r->scroll.x > 0) {
    s16 w = r->clip.x2 - r->clip.x1 + 1 - r->scroll.x;
    if (w>0)
      VID(scrollblit) (r->output,r->clip.x1 + r->scroll.x,r->clip.y1,
		       w, r->clip.y2 - r->clip.y1 + 1,
		       r->output,r->clip.x1,r->clip.y1,
		       PG_LGOP_NONE);
  }

  /* Truncate our own clipping rectangle */
  scroll_clip(&r->clip,&r->scroll);
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
    r->angle %= 360;
    if (r->angle<0) r->angle += 360;
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

  default:
    VID(grop_handler)(r,n);
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
			     struct fractionpair *scale, struct pgpair *trans) {
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
  struct pgpair trans;

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

    /* The pixels keep their usual size, but center them */
  case PG_MAP_CENTER:
    trans.x = (r->output_rect.w - r->map.w) >> 1;
    trans.y = (r->output_rect.h - r->map.h) >> 1;
    scale.x.n = scale.x.d = scale.y.n = scale.y.d = 1;
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
  
  /* Save node's coordinates before clipping */
  r->orig = n->r;    
  
  memset(&r->csrc,0,sizeof(r->csrc));

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
    
    /* Similar deal with textgrid and paragraph, easier to handle clipping later */
  case PG_GROP_TEXTGRID:
  case PG_GROP_PARAGRAPH:
  case PG_GROP_PARAGRAPH_INC:
  case PG_GROP_TEXTRECT:
    if (n->r.x>r->clip.x2 || n->r.y>r->clip.y2)
      goto skip_this_node;
    break;

    /* Handle rotateblit later, don't bother checking for
     * easy cases because there are none :)
     */
  case PG_GROP_ROTATEBITMAP:
    break;

  case PG_GROP_LINE:
    if (gropnode_line_clip(r,n))
      goto skip_this_node;
    break;

  default:
    gropnode_rect_clip(r,n);
  }
  
  if ((n->r.w <= 0 || n->r.h <= 0) && n->type!=PG_GROP_LINE)
    goto skip_this_node;
  
  return;
 skip_this_node:
  n->type = PG_GROP_NOP;
}

int gropnode_line_clip(struct groprender *r, struct gropnode *n) {  
  /* Is this line just completely out there? */
  if ( ((n->r.x<r->clip.x1) && ((n->r.x+n->r.w)<r->clip.x1)) ||
       ((n->r.x>r->clip.x2) && ((n->r.x+n->r.w)>r->clip.x2)) ||
       ((n->r.y<r->clip.y1) && ((n->r.y+n->r.h)<r->clip.y1)) ||
       ((n->r.y>r->clip.y2) && ((n->r.y+n->r.h)>r->clip.y2)) )
    return 1;
  
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
      if (!n->r.h) return 1;   /* Avoid divide by zero */
      u = t * n->r.w / n->r.h;
      n->r.x += u;
      n->r.w -= u;
      n->r.h -= t;
      n->r.y = r->clip.y1;
    }
    
    else if (n->r.y>r->clip.y2) {          /* Point 1 below clip */
      t = n->r.y - r->clip.y2;
      if (!n->r.h) return 1;   /* Avoid divide by zero */
      u = t * n->r.w / n->r.h;
      n->r.x -= u;
      n->r.w += u;
      n->r.h += t;
      n->r.y = r->clip.y2;
    }
    
    if ((n->r.x+n->r.w)<r->clip.x1) {           /* Point 2 left of clip */
      t = r->clip.x1 - n->r.x - n->r.w;
      if (!n->r.w) return 1;   /* Avoid divide by zero */
      n->r.h += t * n->r.h / n->r.w;
      n->r.w = r->clip.x1-n->r.x;
    }
    
    else if ((n->r.x+n->r.w)>r->clip.x2) {      /* Point 2 right of clip */
      t = n->r.x + n->r.w - r->clip.x2;
      if (!n->r.w) return 1;   /* Avoid divide by zero */
      n->r.h -= t * n->r.h / n->r.w;
      n->r.w = r->clip.x2-n->r.x;
    }
    
    if ((n->r.y+n->r.h)<r->clip.y1) {           /* Point 2 above clip */
      t = r->clip.y1 - n->r.y - n->r.h;
      if (!n->r.h) return 1;   /* Avoid divide by zero */
      n->r.w += t * n->r.w / n->r.h;
      n->r.h = r->clip.y1-n->r.y;
    }
    
    else if ((n->r.y+n->r.h)>r->clip.y2) {      /* Point 2 below clip */
      t = n->r.y + n->r.h - r->clip.y2;
      if (!n->r.h) return 1;   /* Avoid divide by zero */
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
      return 1;
  }
  
  else {
    /* It's horizontal or vertical. Do a little prep, then
     * clip it like a normal rectangle
     */
    
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
    
    gropnode_rect_clip(r,n);
  }
  
  return 0;
}

void gropnode_rect_clip(struct groprender *r, struct gropnode *n) {
  if (n->r.x<r->clip.x1) {
    n->r.w -= r->csrc.x = r->clip.x1 - n->r.x;
    n->r.x = r->clip.x1;
  }
  if (n->r.y<r->clip.y1) {
    n->r.h -= r->csrc.y = r->clip.y1 - n->r.y;
    n->r.y = r->clip.y1;
  }
  if ((n->r.x+n->r.w-1)>r->clip.x2)
    n->r.w -= r->csrc.w = n->r.w - (r->clip.x2-n->r.x+1);
  if ((n->r.y+n->r.h-1)>r->clip.y2)
    n->r.h -= r->csrc.h = n->r.h - (r->clip.y2-n->r.y+1);
}

/****************************************************** gropnode_draw */

void gropnode_draw(struct groprender *r, struct gropnode *n) {
  struct pgstring *str;
  u32 *arr;
  hwrbitmap bit;
  s16 bw,bh;
  hwrcolor c;
  struct font_descriptor *fd;
  struct pgrect clipsrc;

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
    if (iserror(rdhandle((void**)&str,PG_TYPE_PGSTRING,-1,
			 n->param[0])) || !str) break;
    if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
			 r->hfont)) || !fd) break;
    fd->lib->draw_string(fd,r->output,xy_to_pair(n->r.x,n->r.y),c,
			 str,&r->clip,r->lgop,r->angle);
    break;

  case PG_GROP_TEXTRECT:
    if (iserror(rdhandle((void**)&str,PG_TYPE_PGSTRING,-1,
			 n->param[0])) || !str) break;
    if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
			 r->hfont)) || !fd) break;
    //    textrect(r->output,fd,&n->r,str,c,&r->clip,r->lgop,r->angle);
    break;

#ifdef CONFIG_WIDGET_TERMINAL
  case PG_GROP_TEXTGRID:
    textgrid_render(r,n);
    break;      
#endif

#ifdef CONFIG_WIDGET_TEXTBOX
  case PG_GROP_PARAGRAPH:
    paragraph_render(r,n);
    break;
  case PG_GROP_PARAGRAPH_INC:
    paragraph_render_inc(r,n);
    break;
#endif

  case PG_GROP_BITMAP:
    if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			 n->param[0])) || !bit) break;
    VID(bitmap_getsize) (bit,&bw,&bh);
    VID(multiblit) (r->output,n->r.x,n->r.y,n->r.w,n->r.h,bit,
		    0,0,bw,bh,(r->src.x+r->csrc.x)%bw,
		    (r->src.y+r->csrc.y)%bh,r->lgop);     
    break;

  case PG_GROP_ROTATEBITMAP:
    if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			 n->param[0])) || !bit) break;

    /* get the size of the _source_ bitmap */
    VID(bitmap_getsize) (bit,&bw,&bh);
     
    /* Source rect clipping */
    clipsrc = r->src;
    if (clipsrc.x < 0) clipsrc.x = 0;
    if (clipsrc.y < 0) clipsrc.y = 0;
    if (clipsrc.w > (bw - clipsrc.x)) clipsrc.w = bw - clipsrc.x;
    if (clipsrc.h > (bh - clipsrc.y)) clipsrc.h = bh - clipsrc.y;
    if (clipsrc.w <= 0 || clipsrc.h <= 0)
      break;
    
    VID(rotateblit) (r->output,n->r.x,n->r.y,
		     bit,clipsrc.x,clipsrc.y,clipsrc.w,clipsrc.h,
		     &r->clip,r->angle,r->lgop);     
    break;
    
  case PG_GROP_BLUR:
    VID(blur) (r->output,n->r.x,n->r.y,n->r.w,n->r.h,n->param[0]);
    break;
     
  case PG_GROP_TILEBITMAP:
    if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			 n->param[0])) || !bit) break;

    clipsrc = r->src;
    VID(bitmap_getsize) (bit,&bw,&bh);
    if (clipsrc.x < 0) clipsrc.x = 0;
    if (clipsrc.y < 0) clipsrc.y = 0;
    if (clipsrc.w > (bw - clipsrc.x)) clipsrc.w = bw - clipsrc.x;
    if (clipsrc.h > (bh - clipsrc.y)) clipsrc.h = bh - clipsrc.y;
    if (n->r.w <= 0 || n->r.h <= 0)
      break;
    
    VID(multiblit) (r->output,n->r.x,n->r.y,n->r.w,n->r.h,bit,
		    clipsrc.x,clipsrc.y,clipsrc.w,clipsrc.h,
		    r->csrc.x % clipsrc.w,r->csrc.y % clipsrc.h,r->lgop);     
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
  case PG_GROP_ARC: 
    VID(arc) (r->output,
	      n->r.x,
	      n->r.y,
	      n->r.w,
	      n->r.h,
	      n->param[0],
	      n->param[1],
	      r->angle,
	      c,
	      r->lgop 
	      ); 
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
    VID(update)(r->output,n->r.x,n->r.y,n->r.w,n->r.h);
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


  default:
    VID(grop_handler)(r,n);
    break;
		  
  }
}

/****************************************************** geometry */

void quad_intersect(struct pgquad *dest, struct pgquad *src) {
  dest->x1 = max(dest->x1, src->x1);
  dest->y1 = max(dest->y1, src->y1);
  dest->x2 = min(dest->x2, src->x2);
  dest->y2 = min(dest->y2, src->y2);
}

struct pgquad *rect_to_quad(struct pgrect *rect) {
  static struct pgquad q;
  q.x1 = rect->x;
  q.y1 = rect->y;
  q.x2 = rect->x + rect->w - 1;
  q.y2 = rect->y + rect->h - 1;
  return &q;
}

struct pgpair *xy_to_pair(s16 x, s16 y) {
  static struct pgpair p;
  p.x = x;
  p.y = y;
  return &p;
}

/* The End */

  
