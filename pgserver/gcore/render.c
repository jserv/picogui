/* $Id: render.c,v 1.3 2001/05/05 02:00:07 micahjd Exp $
 *
 * render.c - gropnode rendering engine. gropnodes go in, pixels come out :)
 *            The gropnode is clipped, translated, and otherwise mangled,
 *            then the appropriate vidlib call is made to put it on the
 *            screen (or where-ever it is supposed to go)
 * 
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

#include <pgserver/render.h>
#include <pgserver/appmgr.h>    /* for defaultfont */

/****************************************************** grop_render */

/* This renders a divnode's groplist to the screen
 *
 * Sets up a groprender structure based on the divnode's information,
 * performs pre-render housekeeping, processes flags, and renders each node
 */

void grop_render(struct divnode *div) {
   struct gropnode **listp = &div->grop;
   struct gropnode node;
   struct quad cr;
   u8 incflag;
   struct groprender rend;

   /* default render values */
   memset(&rend,0,sizeof(rend));
   rend.lgop = PG_LGOP_NONE;
   rend.output = vid->display;
   rdhandle((void**)&rend.font,PG_TYPE_FONTDESC,-1,defaultfont);
   
   /* Transfer over some numbers from the divnode */   
   rend.clip.x1 = div->x;
   rend.clip.x2 = div->x+div->w-1;
   rend.clip.y1 = div->y;
   rend.clip.y2 = div->y+div->h-1;
   rend.translation.x = div->tx;
   rend.translation.y = div->ty;
   rend.scroll.x = div->tx - div->otx;
   rend.scroll.y = div->ty - div->oty;
   div->otx = div->tx;
   div->oty = div->ty;
   rend.output_rect.x = div->w;
   rend.output_rect.y = div->h;
   
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
     groplist_scroll(&rend);
   
   /* Get rid of any pesky sprites in the area. If this isn't incremental
    * go ahead and protect the whole divnode */
   if (!incflag) {
      VID(sprite_protectarea) (&rend.clip,spritelist);
      
      /* "dirty" this region of the screen so the blits notice it */
      add_updarea(rend.clip.x1,rend.clip.y1,rend.clip.x2-
		  rend.clip.x1+1,rend.clip.y2-rend.clip.y1+1);
   }
   
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
      
      /* Always draw relative to divnode, optionally add translation */
      node.r.x += div->x;
      node.r.y += div->y;
      if (node.flags & PG_GROPF_TRANSLATE) {
	 node.r.x += rend.translation.x;
	 node.r.y += rend.translation.y;
      }
      
      /* Add mapping offset */
      node.r.x += rend.offset.x;
      node.r.y += rend.offset.y;
      node.r.w += rend.offset.w;
      node.r.h += rend.offset.h;

      /* Save node's coordinates before clipping */
      rend.orig = node.r;
      
      /* Clip clip! */
      gropnode_clip(&rend,&node);

      /* Anything to do? */
      if ((node.r.w <= 0 || node.r.h <= 0) && node.type!=PG_GROP_LINE)
	goto skip_this_node;
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

void groplist_scroll(struct groprender *r) {
#if 0     /* Scrolling is broke right now */
   
   int ydif;
     
    /**** Scroll-only redraw */

    /* Get deltas now to prevent a race condition? */
    ydif = div->ty-div->oty;

    /* Shift the existing image, and draw the strip along the edge */
    if (ydif<0) {
      /* Go up */

      if ((div->h+ydif)>0) {
	cr.x1 = div->x;
	cr.y1 = div->y;
	cr.x2 = div->x + div->w - 1;
	cr.y2 = div->y + div->h + ydif - 1;
	add_updarea(cr.x1,cr.y1,cr.x2,cr.y2);
	VID(sprite_protectarea) (&cr,spritelist);

	VID(blit) (NULL,div->x,div->y-ydif,
		     div->x,div->y,
		     div->w,div->h+ydif,PG_LGOP_NONE);
      }

      if (cr.y2>cy1)
	cy1 = cr.y2;
    }
    else if (ydif>0) {
      /* Go down */

      if ((div->h-ydif)>0) {
	cr.x1 = div->x;
	cr.y1 = div->y + ydif;
	cr.x2 = div->x + div->w - 1;
	cr.y2 = div->y + div->h - 1;
	add_updarea(cr.x1,cr.y1,cr.x2,cr.y2);
	VID(sprite_protectarea) (&cr,spritelist);

	VID(scrollblit) (div->x,div->y,div->x,div->y+ydif,
			   div->w,div->h-ydif);
      }      

      if (cr.y1<cy2)
	cy2 = cr.y1;
    }
    else
      return;

#endif /* 0 */
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
      rdhandle((void**)&r->font,PG_TYPE_FONTDESC,-1,n->param[0]);
      break;
      
    case PG_GROP_SETMAPPING:
      r->map = n->r;
      r->maptype = n->param[0];
      break;
      
   }
}

/****************************************************** gropnode_map */

void gropnode_map(struct groprender *r, struct gropnode *n) {

   /* FIXME: Fix negative (or otherwise bad) src values */
   
   switch (r->maptype) {
    case PG_MAP_NONE:
      return;
    
    case PG_MAP_SCALE:
      n->r.x = n->r.x * r->output_rect.x / r->map.w;
      n->r.y = n->r.y * r->output_rect.y / r->map.h;
      if (n->type != PG_GROP_TEXT) {
	 if (n->type != PG_GROP_BAR)
	   n->r.w = n->r.w * r->output_rect.x / r->map.w;
	 if (n->type != PG_GROP_SLAB)
	   n->r.h = n->r.h * r->output_rect.y / r->map.h;
      }
      return;
      
   }
}

/****************************************************** gropnode_clip */

void gropnode_clip(struct groprender *r, struct gropnode *n) {
    /* Clip - clipping serves two purposes:
     *   1. security, an app/widget stays in its allocated space
     *   2. scrolling needs to be able to update any arbitrary
     *      slice of an area
     */
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

   return;
   skip_this_node:
   n->type = PG_GROP_NOP;
}
 
/****************************************************** gropnode_draw */

void gropnode_draw(struct groprender *r, struct gropnode *n) {
   char *str;
   hwrbitmap bit;
   s16 bw,bh;
   hwrcolor c;

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
       if (r->orig.x>=r->clip.x1 && r->orig.x<=r->clip.x2 && n->r.h>2)
	 VID(bar) (r->output,n->r.x,n->r.y+1,n->r.h-2,c,r->lgop);
       if ((r->orig.x+r->orig.w-1)>=r->clip.x1 && (r->orig.x+r->orig.w-1)<=r->clip.x2 && n->r.h>2)
	 VID(bar) (r->output,n->r.x+n->r.w-1,n->r.y+1,n->r.h-2,c,r->lgop);
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
      outtext(vid->display,r->font,n->r.x,n->r.y,c,str,&r->clip,
	      r->fill,r->bg,r->lgop,r->angle);
      break;

      /* The workhorse of the terminal widget. 4 params: buffer handle,
       * font handle, buffer width, and buffer offset */
    case PG_GROP_TEXTGRID:
      if (iserror(rdhandle((void**)&str,PG_TYPE_STRING,-1,
			    n->param[0])) || !str) break;
	{
	   int buffersz,bufferw,bufferh;
	   int celw,celh,charw,charh,offset;
	   int i;
	   unsigned char attr;
	   
	   /* Should be fine for fixed width fonts
	    * and pseudo-acceptable for others? */
	   celw      = r->font->font->vwtab['W'];  
	   celh      = r->font->font->h;
	   
	   bufferw   = n->param[1];
	   buffersz  = strlen(str) - n->param[2];
	   str      += n->param[2];
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
	   for (;charh;charh--,n->r.y+=celh,str+=offset)
	     for (n->r.x=r->orig.x,i=charw;i;i--,n->r.x+=celw,str++) {
		attr = *(str++);
/*
 * This code made obsolete by fill and bg parameter of charblit
 * 
 if (attr & 0xF0)
 VID(rect) (r->output,n->r.x,n->r.y,celw,celh,
 textcolors[attr>>4],r->lgop);
 if ((*str) != ' ')
 */
		VID(charblit) (r->output,
			       (((unsigned char *)r->font->font->bitmaps)+
				r->font->font->trtab[*str]),
			       n->r.x,n->r.y,r->font->font->vwtab[*str],
			       celh,0,0,textcolors[attr & 0x0F],NULL,
			       attr & 0xF0,textcolors[attr>>4],r->lgop);
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
    }
}


/* The End */

