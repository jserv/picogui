/* $Id: grop.c,v 1.27 2000/12/17 05:53:49 micahjd Exp $
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
  struct bitmap *bit;
  int x,y,w,h,ydif,i,j,xo;
  unsigned char attr;
  unsigned char *str;
  int cx1,cx2,cy1,cy2;   /* Clipping */

  list = div->grop;

  /* Normally the clipping is set to the divnode */
  cx1 = div->x;
  cx2 = div->x+div->w-1;
  cy1 = div->y;
  cy2 = div->y+div->h-1;

  if ((div->flags & DIVNODE_SCROLL_ONLY) && 
      !(div->flags & DIVNODE_NEED_REDRAW)) {

    /**** Scroll-only redraw */

    /* Get deltas now to prevent a race condition? */
    ydif = div->ty-div->oty;

    /* Shift the existing image, and draw the strip along the edge */
    if (ydif<0) {
      /* Go up */

      if ((div->h+ydif)>0)
	(*vid->blit)(NULL,div->x,div->y-ydif,
		     div->x,div->y,
		     div->w,div->h+ydif,PG_LGOP_NONE);

      x = div->y+div->h-1+ydif;
      if (x>cy1)
	cy1 = x;
    }
    else if (ydif>0) {
      /* Go down */

      if ((div->h-ydif)>0)
	(*vid->scrollblit)(div->x,div->y,div->x,div->y+ydif,
			   div->w,div->h-ydif);
      
      x = div->y+ydif-1;
      if (x<cx2)
	cx2 = x;
    }
    else
      return;
  }

  div->otx = div->tx;
  div->oty = div->ty;

  while (list) {
    if ((list->w <= 0 || list->h <= 0) && list->type!=PG_GROP_LINE)
      /* There is no spoon */
      goto skip_this_node;

    x = list->x+div->x;
    y = list->y+div->y;
    w = list->w;
    h = list->h;

    if (list->flags & PG_GROPF_TRANSLATE) {
      x += div->tx;
      y += div->ty;
    }

    /* Clip - clipping serves two purposes:
     *   1. security, an app/widget stays in its allocated space
     *   2. scrolling needs to be able to update any arbitrary
     *      slice of an area
     */
    switch (list->type) {
      /* Default clipping just truncates */
    default:
      if (x<cx1) {
	w += cx1 - x;
	x = cx1;
      }
      if (y<cy1) {
	h += cy1 - y;
	y = cy1;
      }
      if ((x+w-1)>cx2)
	w += 1-x-w+cx2;
      if ((y+h-1)>cy2)
	h += 1-y-h+cy2;
    }

    /* Anything to do? */
    if (!w || !h)
      goto skip_this_node;

    /* "dirty" this region of the screen so the blits notice it */
    add_updarea(x,y,w,h);

    #ifdef DEBUG_VIDEO
    /* Illustrate the grop extents */
    (*vid->frame)(x,y,w,h,(*vid->color_pgtohwr)(0x00FF00));
    #endif

    switch (list->type) {
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
      (*vid->frame)(x,y,w,h,list->param[0]);
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
      if (list->type == PG_GROP_TEXT)
	outtext(fd,x,y,list->param[2],str);
      else
	outtext_v(fd,x,y,list->param[2],str);
      break;

      /* The workhorse of the terminal widget. 4 params: buffer handle, font handle,
	 buffer width, and buffer offset */
    case PG_GROP_TEXTGRID:
      if (iserror(rdhandle((void**)&str,PG_TYPE_STRING,-1,
			   list->param[0])) || !str) break;
      if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
			   list->param[1])) || !fd) break;
      for (;list->param[3] && *str;list->param[3]--) str++;
      i = 0;
      xo = x;
      while ((*str) && (*(str+1))) {
	if (i==list->param[2]) {
	  y += fd->font->h;
	  i = 0;
	  x = xo;
	}

	attr = *(str++);
	(*vid->rect)(x,y,fd->font->vwtab['W'],fd->font->h,textcolors[attr>>4]);
	if (fd->font->trtab[*str] >= 0)
	  (*vid->charblit)((((unsigned char *)fd->font->bitmaps)+fd->font->trtab[*str]),
			   x,y,fd->font->vwtab[*str],fd->font->h,0,textcolors[attr & 0x0F]);
	x += fd->font->vwtab['W'];
	str++; i++;
      }	
      break;      

    case PG_GROP_BITMAP:
      if (list->param[1]==PG_LGOP_NULL) break;
      if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			   list->param[0])) || !bit) break;

      /* Did they really mean a tiled blit instead? */
      (*vid->bitmap_getsize)(bit,&i,&j);
      if (w>i || h>j)
	(*vid->tileblit)(bit,list->param[2],list->param[3],i,j,x,y,w,h);
      else
	(*vid->blit)(bit,list->param[2],list->param[3],x,y,w,h,list->param[1]);
      break;
    case PG_GROP_TILEBITMAP:
      if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			   list->param[0])) || !bit) break;
      (*vid->tileblit)(bit,
		       list->param[1],
		       list->param[2],
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

