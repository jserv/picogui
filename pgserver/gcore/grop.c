/* $Id: grop.c,v 1.13 2000/08/14 19:35:45 micahjd Exp $
 *
 * grop.c - rendering and creating grop-lists
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <divtree.h>
#include <video.h>
#include <g_malloc.h>
#include <font.h>
#include <handle.h>

/* This renders a divnode's groplist using the x,y,w,h,tx,ty from 
 * the divnode. The grop is translated by (x+tx,y+ty) and clipped to
 * x,y,w,h. 
 */
void grop_render(struct divnode *div) {
  struct gropnode *list;
  struct cliprect clip = {div->x,div->y,div->x+div->w-1,div->y+div->h-1};
  struct fontdesc *fd;
  struct bitmap *bit;
  int x,y,w,h,ydif;
  char *str;

  if (!div) return;
  list = div->grop;

  if ((div->flags & DIVNODE_SCROLL_ONLY) && 
      !(div->flags & DIVNODE_NEED_REDRAW)) {

    /**** Scroll-only redraw */

    /* Get deltas now to prevent a race condition? */
    ydif = div->ty-div->oty;

    /* Shift the existing image, and draw the strip along the edge */
    if (ydif<0) {
      /* Go up */

      hwr_blit(NULL,LGOP_NONE,NULL,div->x,div->y-ydif,NULL,
	       div->x,div->y,div->w,div->h+ydif);
      x = div->y+div->h-1+ydif;
      if (x>clip.y) clip.y = x;
    }
    else if (ydif>0) {
      /* Go down */

      hwr_blit(NULL,LGOP_NONE,NULL,div->x,div->y,NULL,
	       div->x,div->y+ydif,div->w,div->h-ydif);
      x = div->y+ydif-1;
      if (x<clip.y2) clip.y2 = x;
    }
    else
      return;
  }

  div->otx = div->tx;
  div->oty = div->ty;

  while (list) {
    if (list->w==-1 && list->h==-1) {
      /* -1,-1 is the magical code that causes the grop to fill
	 the entire divnode, and not scroll */
      x = div->x;
      y = div->y;
      w = div->w;
      h = div->h;
    }
    else {
      x = list->x+div->x+div->tx;
      y = list->y+div->y+div->ty;
      w = list->w;
      h = list->h;
    }

    switch (list->type) {
    case GROP_PIXEL:
      hwr_pixel(&clip,x,y,list->param.c);
      break;
    case GROP_LINE:
      hwr_line(&clip,x,y,w+x,h+y,list->param.c);
      break;
    case GROP_RECT:
      hwr_rect(&clip,x,y,w,h,list->param.c);
      break;
    case GROP_DIM:
      hwr_dim(&clip);
      break;
    case GROP_FRAME:
      hwr_frame(&clip,x,y,w,h,list->param.c);
      break;
    case GROP_SLAB:
      hwr_slab(&clip,x,y,w,list->param.c);
      break;
    case GROP_BAR:
      hwr_bar(&clip,x,y,h,list->param.c);
      break;
    case GROP_TEXT:
      if (iserror(rdhandle((void**)&str,TYPE_STRING,-1,
			   list->param.text.string)) || !str) break;
      if (iserror(rdhandle((void**)&fd,TYPE_FONTDESC,-1,
			   list->param.text.fd)) || !fd) break;

      outtext(&clip,fd,x,y,list->param.text.col,str);
      break;
    case GROP_BITMAP:
      if (iserror(rdhandle((void**)&bit,TYPE_BITMAP,-1,
			   list->param.bitmap.bitmap)) || !bit) break;
      hwr_blit(&clip,list->param.bitmap.lgop,bit,0,0,NULL,x,y,w,h);
      break;
    case GROP_GRADIENT:
      /* Gradients are fun! */
      hwr_gradient(&clip,x,y,w,h,list->param.gradient.c1,
		   list->param.gradient.c2,list->param.gradient.angle,
		   list->param.gradient.translucent);      
      break;
    }
    list = list->next;
  }
}

#if 0 /* THIS IS UNTESTED */

/* Calculate a bounding box for all the nodes in a groplist */
void grop_getextent(struct gropnode *head,int *x1,int *y1,int *x2,int *y2) {
  int nx1,ny1,nx2,ny2,w,h,newbox,notfirst;   /* Extent of one node */
  struct gropnode *p;
  char *str;
  struct fontdesc *fd;

  p = head;
  *x1 = *y1 = *x2 = *y2 = notfirst = 0;

  while (p) {
    newbox=0;
    switch (p->type) {
    
    case GROP_PIXEL:
      nx1 = nx2 = p->x;
      ny1 = ny2 = p->y;
      newbox=1;
      break;

    case GROP_LINE:
    case GROP_RECT:
    case GROP_BITMAP:
    case GROP_GRADIENT:
    case GROP_FRAME:
      nx1 = p->x;
      ny1 = p->y;
      nx2 = p->x+p->w-1;
      ny2 = p->y+p->h-1;
      newbox=1;
      break;

    case GROP_SLAB:
      nx1 = p->x;
      nx2 = p->x+p->w-1;
      ny1 = ny2 = p->y;
      newbox=1;
      break;

    case GROP_BAR:
      ny1 = p->y;
      ny2 = p->y+p->h-1;
      nx1 = nx2 = p->x;
      newbox=1;
      break;

    case GROP_TEXT:
      /* Yuk. Dereference the handles, then run sizetext on it */

      if (rdhandle((void**)&str,TYPE_STRING,-1,p->param.text.string).type != 
	  ERRT_NONE || !str) break;
      if (rdhandle((void**)&fd,TYPE_FONTDESC,-1,p->param.text.fd).type != 
	  ERRT_NONE || !fd) break;
      sizetext(fd,&w,&h,str);
      nx1 = p->x;
      ny1 = p->y;
      nx2 = p->x+w-1;
      ny2 = p->y+h-1;
      newbox=1;
      break;

    }
    p = p->next;

    if (newbox) {
      if (notfirst) {
	/* Assimilate the bounding box */
	if (nx1<*x1) *x1 = nx1;
	if (ny1<*y1) *y1 = ny1;
	if (nx2>*x2) *x2 = nx2;
	if (ny2>*y2) *y2 = ny2;
      }
      else {
	/* First one */
	*x1 = nx1;
	*x2 = nx2;
	*y1 = ny1;
	*y2 = ny2;
	notfirst=1;
      }
    }
  }
}

#endif

/* Given a pointer to the groplist head pointer, this will add a new node to
   the groplist. Also sets the 'next' pointer to NULL.
*/

void grop_addnode(struct gropnode **headpp,struct gropnode *node) {
  struct gropnode *p;
  node->next = NULL;
  if (!*headpp) {
    *headpp = node;
    return;
  }
  p = *headpp;
  while (p->next) p = p->next;
  p->next = node;
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

/* Functions to emulate the hwr_* graphics primitives but store the result
   in the groplist instead of immediately drawing it. Returns a pointer
   to the new gropnode in n
*/

g_error grop_pixel(struct gropnode **headpp,
		   int x, int y, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_PIXEL;
  n->x = x;
  n->y = y;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_line(struct gropnode **headpp,
		  int x1, int y1, int x2, int y2, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_LINE;
  n->x = x1;
  n->y = y1;
  n->w = x2;
  n->h = y2;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_rect(struct gropnode **headpp,
		  int x, int y, int w, int h, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_RECT;
  n->x = x;
  n->y = y;
  n->w = w;
  n->h = h;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_dim(struct gropnode **headpp) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_DIM;
  n->x = n->y = n->w = n->h = 0;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_frame(struct gropnode **headpp,
		  int x, int y, int w, int h, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_FRAME;
  n->x = x;
  n->y = y;
  n->w = w;
  n->h = h;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_slab(struct gropnode **headpp,
		  int x, int y, int w, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_SLAB;
  n->x = x;
  n->y = y;
  n->w = w;
  n->h = 1;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_bar(struct gropnode **headpp,
		  int x, int y, int h, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_BAR;
  n->x = x;
  n->y = y;
  n->h = h;
  n->w = 1;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_text(struct gropnode **headpp,
		  int x, int y, handle fd, devcolort col, handle str) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_TEXT;
  n->x = x;
  n->y = y;
  n->w = n->h = 0;
  n->param.text.string = str;
  n->param.text.fd = fd;
  n->param.text.col = col;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_bitmap(struct gropnode **headpp,
		  int x, int y, int w, int h, handle b, int lgop) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_BITMAP;
  n->x = x;
  n->y = y;
  n->w = w;
  n->h = h;
  n->param.bitmap.bitmap = b;
  n->param.bitmap.lgop = lgop;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_gradient(struct gropnode **headpp,
		      int x, int y, int w, int h, devcolort c1, devcolort c2,
		      int angle,int translucent) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_GRADIENT;
  n->x = x;
  n->y = y;
  n->w = w;
  n->h = h;
  n->param.gradient.c1 = c1;
  n->param.gradient.c2 = c2;
  n->param.gradient.angle = angle;
  n->param.gradient.translucent = translucent;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_null(struct gropnode **headpp) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  errorcheck;
  n->type = GROP_NULL;
  n->x = n->y = n->w = n->h = 0;
  grop_addnode(headpp,n);
  return sucess;
}

/* The End */

