/* $Id: grop.c,v 1.20 2000/10/19 01:21:23 micahjd Exp $
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
  int x,y,w,h,ydif;
  char *str;

  list = div->grop;

  if ((div->flags & DIVNODE_SCROLL_ONLY) && 
      !(div->flags & DIVNODE_NEED_REDRAW)) {

    /**** Scroll-only redraw */

    (*vid->clip_off)();

    /* Get deltas now to prevent a race condition? */
    ydif = div->ty-div->oty;

    /* Shift the existing image, and draw the strip along the edge */
    if (ydif<0) {
      /* Go up */

      if ((div->h+ydif)>0)
	(*vid->blit)(NULL,div->x,div->y-ydif,
		     NULL,div->x,div->y,
		     div->w,div->h+ydif,PG_LGOP_NONE);

      x = div->y+div->h-1+ydif;
      if (x>div->y)
	(*vid->clip_set)(div->x,
			 x,
			 div->x+div->w-1,
			 div->y+div->h-1);      
      else
	(*vid->clip_set)(div->x,div->y,div->x+div->w-1,div->y+div->h-1);

    }
    else if (ydif>0) {
      /* Go down */

      if ((div->h-ydif)>0)
	(*vid->scrollblit)(div->x,div->y,div->x,div->y+ydif,
			   div->w,div->h-ydif);
      
      x = div->y+ydif-1;
      if (x<(div->y+div->h-1))
	(*vid->clip_set)(div->x,
			 div->y,
			 div->x+div->w-1,
			 x);      
      else
	(*vid->clip_set)(div->x,div->y,div->x+div->w-1,div->y+div->h-1);
    }
    else
      return;
  }
  else  
    (*vid->clip_set)(div->x,div->y,div->x+div->w-1,div->y+div->h-1);

  div->otx = div->tx;
  div->oty = div->ty;

  while (list) {
    if ((list->w <= 0 || list->h <= 0) && list->type!=PG_GROP_LINE) {
      /* There is no spoon */
      list = list->next;
      continue;
    }

    x = list->x+div->x;
    y = list->y+div->y;
    w = list->w;
    h = list->h;

    if (list->flags & PG_GROPF_TRANSLATE) {
      x += div->tx;
      y += div->ty;
    }

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
      (*vid->dim)();
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
      if (iserror(rdhandle((void**)&str,PG_TYPE_STRING,-1,
			   list->param[0])) || !str) break;
      if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
			   list->param[1])) || !fd) break;

      outtext(fd,x,y,list->param[2],str);
      break;
    case PG_GROP_BITMAP:
      if (iserror(rdhandle((void**)&bit,PG_TYPE_BITMAP,-1,
			   list->param[0])) || !bit) break;
      (*vid->blit)(bit,list->param[2],list->param[3],NULL,x,y,w,h,list->param[1]);
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

