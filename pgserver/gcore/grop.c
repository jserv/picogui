/* $Id: grop.c,v 1.33 2001/01/12 04:49:01 micahjd Exp $
 *
 * grop.c - rendering and creating grop-lists
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/divtree.h>
#include <pgserver/video.h>
#include <pgserver/g_malloc.h>
#include <pgserver/font.h>
#include <pgserver/handle.h>

short int defaultgropflags;

/* This renders a divnode's groplist using the x,y,w,h,tx,ty from 
 * the divnode. The grop is translated by (x+tx,y+ty) and clipped to
 * x,y,w,h. 
 */
void grop_render(struct divnode *div) {
  struct gropnode *list;
  struct fontdesc *fd;
  hwrbitmap bit;
  int x,y,w,h,ydif,bw,bh;
  unsigned char *str;
  int cx1,cx2,cy1,cy2;   /* Clipping */
  int srcx,srcy;         /* Offset added to source bitmap,
			    calculated from clipping */
  int ox,oy,ow,oh;       /* Copy of x,y,w,h before clipping */
  int type;
  struct cliprect cr;
  unsigned int incflag;
   
  list = div->grop;

  /* Normally the clipping is set to the divnode */
  cx1 = div->x;
  cx2 = div->x+div->w-1;
  cy1 = div->y;
  cy2 = div->y+div->h-1;

  /* If we're doing an incremental redraw, look for the PG_GROPF_INCREMENTAL
   * flag set, otherwise make sure it's unset */
  if ((div->flags & DIVNODE_INCREMENTAL) && 
      !(div->flags & DIVNODE_NEED_REDRAW))
     incflag = PG_GROPF_INCREMENTAL;
   else
     incflag = 0;
   
  if ((div->flags & DIVNODE_SCROLL_ONLY) && 
      !(div->flags & DIVNODE_NEED_REDRAW)) {

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
	(*vid->sprite_protectarea)(&cr,spritelist);

	(*vid->blit)(NULL,div->x,div->y-ydif,
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
	(*vid->sprite_protectarea)(&cr,spritelist);

	(*vid->scrollblit)(div->x,div->y,div->x,div->y+ydif,
			   div->w,div->h-ydif);
      }      

      if (cr.y1<cy2)
	cy2 = cr.y1;
    }
    else
      return;

    #ifdef DEBUG_VIDEO
    /* Illustrate the clipping rect */
    (*vid->rect)(cx1,cy1,cx2-cx1+1,cy2-cy1+1,(*vid->color_pgtohwr)(0x00FF00));
    #endif
  }

  div->otx = div->tx;
  div->oty = div->ty;

  /* Package up that clipping rectangle */
  cr.x1 = cx1;
  cr.x2 = cx2;
  cr.y1 = cy1;
  cr.y2 = cy2;
   
  /* Get rid of any pesky sprites in the area. If this isn't incremental
   * go ahead and protect the whole divnode */
  if (!incflag) {
     (*vid->sprite_protectarea)(&cr,spritelist);
     
     /* "dirty" this region of the screen so the blits notice it */
     add_updarea(cx1,cy1,cx2-cx1+1,cy2-cy1+1);
  }
   
  while (list) {
     
    /* Handle incremental updates */
    if ((list->flags & PG_GROPF_INCREMENTAL) != incflag)
       goto skip_this_node;
     
    x = list->x+div->x;
    y = list->y+div->y;
    w = list->w;
    h = list->h;

    if (list->flags & PG_GROPF_TRANSLATE) {
      x += div->tx;
      y += div->ty;
    }

    ox = x; oy = y; ow = w; oh = h;

    /* Save the type locally so we can change it if needed */
    type = list->type;
     
    /* Clip - clipping serves two purposes:
     *   1. security, an app/widget stays in its allocated space
     *   2. scrolling needs to be able to update any arbitrary
     *      slice of an area
     */
    srcx = srcy = 0;
    switch (type) {
      
     case PG_GROP_LINE:

       /* Is this line just completely out there? */
       if ( ((x<cx1) && ((x+w)<cx1)) ||
	   ((x>cx2) && ((x+w)>cx2)) ||
	   ((y<cy1) && ((y+h)<cy1)) ||
	   ((y>cy2) && ((y+h)>cy2)) )
	 goto skip_this_node;
       
       if (w && h) {        /* Not horizontal or vertical */
	  int t,u;

	  /* A real line - clip, taking slope into account */

	  if (x<cx1) {               /* Point 1 left of clip */
	     t = cx1 - x;
	     u = t*h/w;
	     y += u;
	     h -= u;
	     w -= t;
	     x = cx1;
	  }
	  
	  else if (x>cx2) {          /* Point 1 right of clip */
	     t = x - cx2;
	     u = t*h/w;
	     y -= u;
	     h += u;
	     w += t;
	     x = cx2;
	  }
	  
	  if (y<cy1) {               /* Point 1 above clip */
	     t = cy1 - y;
	     if (!h) goto skip_this_node;   /* Avoid divide by zero */
	     u = t*w/h;
	     x += u;
	     w -= u;
	     h -= t;
	     y = cy1;
	  }

	  else if (y>cy2) {          /* Point 1 below clip */
	     t = y - cy2;
	     if (!h) goto skip_this_node;   /* Avoid divide by zero */
	     u = t*w/h;
	     x -= u;
	     w += u;
	     h += t;
	     y = cy2;
	  }

	  if ((x+w)<cx1) {           /* Point 2 left of clip */
	     t = cx1 - x - w;
	     if (!w) goto skip_this_node;   /* Avoid divide by zero */
	     h += t*h/w;
	     w = cx1-x;
	  }

	  else if ((x+w)>cx2) {      /* Point 2 right of clip */
	     t = x + w - cx2;
	     if (!w) goto skip_this_node;   /* Avoid divide by zero */
	     h -= t*h/w;
	     w = cx2-x;
	  }

	  if ((y+h)<cy1) {           /* Point 2 above clip */
	     t = cy1 - y - h;
	     if (!h) goto skip_this_node;   /* Avoid divide by zero */
	     w += t*w/h;
	     h = cy1-y;
	  }

	  else if ((y+h)>cy2) {      /* Point 2 below clip */
	     t = y + h - cy2;
	     if (!h) goto skip_this_node;   /* Avoid divide by zero */
	     w -= t*w/h;
	     h = cy2-y;
	  }

	  /* If the line's endpoints are no longer within the clipping
	   * rectangle, it means the line never intersected it in the
	   * first place */
	  
	  if ( (x<cx1) || (x>cx2) || (y<cy1) || (y>cy2) ||
	       ((x+w)<cx1) || ((x+w)>cx2) || ((y+h)<cy1) || ((y+h)>cy2) )
	    goto skip_this_node;
	  
      	  break;
       }
       
       else {
	  /* It's horizontal or vertical. Do a little prep, then
	   * let it -fall through- to the normal clipper */

	  if (w) {
	     h=1;   
	     if (w<0) {
		x += w;
		w = 1-w;
	     }
	     type = PG_GROP_SLAB;
	  }
	  
	  else {
	     w=1;
	     if (h<0) {
		y += h;
		h = 1-h;
	     }
	     type = PG_GROP_BAR;
	  }

	  /* ... and fall through to default */
       }
       
       /* Text clipping is handled by the charblit, but we can
	* go ahead and handle a few cases on the string level */
    case PG_GROP_TEXT:
      if (x>cx2 || y>cy2)
	goto skip_this_node;
      break;
    case PG_GROP_TEXTV:
      if (y<cy1)
	goto skip_this_node;
      break;

       /* Default clipping just truncates */
     default:
       if (x<cx1) {
	  w -= srcx = cx1 - x;
	  x = cx1;
       }
       if (y<cy1) {
	  h -= srcy = cy1 - y;
	  y = cy1;
       }
       if ((x+w-1)>cx2)
	 w = cx2-x+1;
       if ((y+h-1)>cy2)
	 h = cy2-y+1;
    }

    /* Anything to do? */
    if ((w <= 0 || h <= 0) && type!=PG_GROP_LINE)
      goto skip_this_node;

    #ifdef DEBUG_VIDEO
    /* Illustrate the grop extents */
    //    (*vid->rect)(x,y,w,h,(*vid->color_pgtohwr)(0x00FF00));
    #endif

    /* If this is incremental, do the sprite protection and double-buffer
     * update rectangle things for each gropnode because the updated area
     * is usually small compared to the whole
     */
    if (incflag) {
       struct cliprect lcr;
       lcr.x1 = x;
       lcr.y1 = y;
       lcr.x2 = x+w-1;
       lcr.y2 = y+h-1;
       
       (*vid->sprite_protectarea)(&lcr,spritelist);
       
       /* "dirty" this region of the screen so the blits notice it */
       add_updarea(x,y,w,h);
    }
     
    switch (type) {
    case PG_GROP_PIXEL:
      (*vid->pixel)(x,y,list->param[0]);
      break;
    case PG_GROP_LINE:
      (*vid->line)(x,y,w+x,h+y,list->param[0]);
      break;
    case PG_GROP_RECT:
      (*vid->rect)(x,y,w,h,list->param[0]);
      break;
    case PG_GROP_DIM:
      (*vid->dim)(x,y,w,h);
      break;
    case PG_GROP_FRAME:
       
       /* Bogacious clipping cruft for frames */
       if (oy>=cy1 && oy<=cy2 ) 
	 (*vid->slab)(x,y,w,list->param[0]);
       if ((oy+oh-1)>=cy1 && (oy+oh-1)<=cy2)
	 (*vid->slab)(x,y+h-1,w,list->param[0]);
       if (ox>=cx1 && ox<=cx2)
	 (*vid->bar)(x,y+1,h-2,list->param[0]);
       if ((ox+ow-1)>=cx1 && (ox+ow-1)<=cx2)
	 (*vid->bar)(x+w-1,y+1,h-2,list->param[0]);
       break;
       
    case PG_GROP_SLAB:
      (*vid->slab)(x,y,w,list->param[0]);
      break;
    case PG_GROP_BAR:
      (*vid->bar)(x,y,h,list->param[0]);
      break;
    case PG_GROP_TEXT:
    case PG_GROP_TEXTV:
      if (iserror(rdhandle((void**)&str,PG_TYPE_STRING,-1,
			   list->param[0])) || !str) break;
      if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
			   list->param[1])) || !fd) break;	
      if (type == PG_GROP_TEXT)
	 outtext(fd,x,y,list->param[2],str,&cr);
       else
	 outtext_v(fd,x,y,list->param[2],str,&cr);
      break;

      /* The workhorse of the terminal widget. 4 params: buffer handle,
       * font handle, buffer width, and buffer offset */
    case PG_GROP_TEXTGRID:
       if (iserror(rdhandle((void**)&str,PG_TYPE_STRING,-1,
			    list->param[0])) || !str) break;
       if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
			    list->param[1])) || !fd) break;
       
	 {
	    int buffersz,bufferw,bufferh;
	    int celw,celh,charw,charh,offset;
	    int i;
	    unsigned char attr;

	    /* Should be fine for fixed width fonts
	     * and pseudo-acceptable for others? */
	    celw      = fd->font->vwtab['W'];  
	    celh      = fd->font->h;

	    bufferw   = list->param[2];
	    buffersz  = strlen(str) - list->param[3];
	    str      += list->param[3];
	    bufferh   = (buffersz / bufferw) >> 1;
	    if (buffersz<=0) return;
	    
	    charw     = w/celw;
	    charh     = h/celh;
	    offset    = (bufferw-charw)<<1;
	    if (offset<0) {
	       offset = 0;
	       charw = bufferw;
	    }
	    if (charh>bufferh)
	      charh = bufferh;

	    ox = x;
	    for (;charh;charh--,y+=celh,str+=offset)
	      for (x=ox,i=charw;i;i--,x+=celw,str++) {
		 attr = *(str++);
		 if (attr & 0xF0)
		   (*vid->rect)(x,y,celw,celh,textcolors[attr>>4]);
		 if ((*str) != ' ')
		   (*vid->charblit)((((unsigned char *)fd->font->bitmaps)+
				     fd->font->trtab[*str]),
				    x,y,fd->font->vwtab[*str],
				    celh,0,textcolors[attr & 0x0F],NULL);
	      }
	    
	 }

       break;      
       
    case PG_GROP_BITMAP:
      if (list->param[1]==PG_LGOP_NULL) break;
      if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			   list->param[0])) || !bit) break;
      (*vid->bitmap_getsize)(bit,&bw,&bh);
      /** FIXME: this will crash for large negative src_x or src_y */
      if (list->param[2]<0)
	list->param[2] += bw;
      if (list->param[3]<0)
	list->param[3] += bh;
      (*vid->blit)(bit,(list->param[2]+srcx)%bw,(list->param[3]+srcy)%bh,x,y,w,h,list->param[1]);
      break;
    case PG_GROP_TILEBITMAP:
      if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			   list->param[0])) || !bit) break;
      (*vid->tileblit)(bit,
		       list->param[1]+srcx,
		       list->param[2]+srcy,
		       list->param[3],
		       list->param[4],
		       x,y,w,h);
      break;
    case PG_GROP_GRADIENT:
      /* Gradients are fun! */
      (*vid->gradient)(x,y,w,h,
		       list->param[0],
		       list->param[1],
		       list->param[2],
		       list->param[3]);      
      break;
    }
  skip_this_node:
    list = list->next;
  }
}

/* Add a new gropnode to the context. Caller fills in
   all the grop's parameters afterwards. */
g_error addgrop(struct gropctxt *ctx, int type,int x,
		int y,int w,int h) {
  struct gropnode *p,*node;
  g_error e;

  /* ctx == NULL is legal, used to disable actual output */
  if (!ctx) return sucess;

  /* This will probably soon be changed to a heap-based
     system for allocating gropnodes 
  */
#ifdef DEBUG_KEYS
  num_grops++;
#endif
  e = g_malloc((void**)&node,sizeof(struct gropnode));
  errorcheck;
  node->type = type;
  node->next = NULL;
  node->flags = defaultgropflags;
  node->x = x;
  node->y = y;
  node->w = w;
  node->h = h;

  if (!ctx->current)
    *ctx->headpp = ctx->current = node;
  else {
    ctx->current->next = node;
    ctx->current = node;
  }

  ctx->n++;   /* There goes another gropnode! */

  return sucess;
}

/* Delete the whole list */
void grop_free(struct gropnode **headpp) {
  struct gropnode *p,*condemn;

#ifdef DEBUG_MEMORY
  static int lock = 0;
  printf("-> grop_free(0x%08X = &0x%08X)\n",headpp,*headpp);
  if (lock)
    printf("     -- grop_free lock triggered --\n");
  lock = 1;
#endif

  if ((!headpp)) return;
  p = *headpp;
  while (p) {
    condemn = p;
    p = p->next;
    g_free(condemn);
#ifdef DEBUG_KEYS
  num_grops--;
#endif
  }
  *headpp = NULL;

#ifdef DEBUG_MEMORY
  printf("<- grop_free()\n");
  lock = 0;
#endif
}

/* Set up a grop context for rendering to a divnode */
void gropctxt_init(struct gropctxt *ctx, struct divnode *div) {
  ctx->headpp = &div->grop;
  ctx->current = div->grop;
  ctx->n = 0;
  ctx->x = 0;
  ctx->y = 0;
  ctx->w = div->w;
  ctx->h = div->h;
}

/* The End */

