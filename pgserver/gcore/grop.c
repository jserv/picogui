/*
 * grop.c - rendering and creating grop-lists
 * $Revision: 1.2 $
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
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
  struct cliprect clip;
  struct fontdesc *fd;
  struct bitmap *bit;
  int x,y,w,h;
  char *str;

  if (!div) return;
  list = div->grop;

  /* TODO: Implement proper scrolling by blitting, then setting clip
     rectangle to the small strip that needs redraw
   TODO: Other redraw types, like new gropnodes only, and scrolled.
  
   */

  clip.x = div->x;
  clip.y = div->y;
  clip.x2 = div->x+div->w-1;
  clip.y2 = div->y+div->h-1;

  while (list) {
    x = list->x+div->x+div->tx;
    y = list->y+div->y+div->ty;
    w = list->w;
    h = list->h;

    switch (list->type) {
    case GROP_PIXEL:
      hwr_pixel(&clip,x,y,list->param.c);
      break;
    case GROP_LINE:
      hwr_line(&clip,x,y,w,h,list->param.c);
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
      if (rdhandle((void**)&str,TYPE_STRING,-1,list->param.text.string).type != 
	  ERRT_NONE || !str) break;
      if (rdhandle((void**)&fd,TYPE_FONTDESC,-1,list->param.text.fd).type != 
	  ERRT_NONE || !fd) break;
      outtext(&clip,fd,x,y,list->param.text.col,str);
      break;
    case GROP_BITMAP:
      if (rdhandle((void**)&bit,TYPE_BITMAP,-1,list->param.bitmap.bitmap).type != 
	  ERRT_NONE || !bit) break;
      hwr_blit(&clip,list->param.bitmap.lgop,bit,0,0,NULL,x,y,w,h);
      break;
    }
    list = list->next;
  }
}

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
  if (e.type != ERRT_NONE) return e;
  n->type = GROP_PIXEL;
  n->x = x;
  n->y = y;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_line(struct gropnode **headpp,
		  int x, int y, int w, int h, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  if (e.type != ERRT_NONE) return e;
  n->type = GROP_LINE;
  n->x = x;
  n->y = y;
  n->w = w;
  n->h = h;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_rect(struct gropnode **headpp,
		  int x, int y, int w, int h, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  if (e.type != ERRT_NONE) return e;
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
  if (e.type != ERRT_NONE) return e;
  n->type = GROP_DIM;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_frame(struct gropnode **headpp,
		  int x, int y, int w, int h, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  if (e.type != ERRT_NONE) return e;
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
  if (e.type != ERRT_NONE) return e;
  n->type = GROP_SLAB;
  n->x = x;
  n->y = y;
  n->w = w;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_bar(struct gropnode **headpp,
		  int x, int y, int h, devcolort c) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  if (e.type != ERRT_NONE) return e;
  n->type = GROP_BAR;
  n->x = x;
  n->y = y;
  n->h = h;
  n->param.c = c;
  grop_addnode(headpp,n);
  return sucess;
}

g_error grop_text(struct gropnode **headpp,
		  int x, int y, handle fd, devcolort col, handle str) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  if (e.type != ERRT_NONE) return e;
  n->type = GROP_TEXT;
  n->x = x;
  n->y = y;
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
  if (e.type != ERRT_NONE) return e;
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

g_error grop_null(struct gropnode **headpp) {
  struct gropnode *n;
  g_error e;
  e = g_malloc((void **) &n,sizeof(struct gropnode));
  if (e.type != ERRT_NONE) return e;
  n->type = GROP_NULL;
  grop_addnode(headpp,n);
  return sucess;
}

/* The End */

