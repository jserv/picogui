/* $Id: bitmap.c,v 1.29 2001/04/07 22:41:45 micahjd Exp $
 *
 * bitmap.c - just displays a bitmap, similar resizing and alignment to labels
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
#include <pgserver/widget.h>

struct bitmapdata {
  handle bitmap,bitmask;
  int align,lgop,transparent;
};
#define DATA ((struct bitmapdata *)(self->data))

void build_bitmap(struct gropctxt *c,unsigned short state,struct widget *self) {
  hwrbitmap bit;
  int x,y,w,h;

  if (!DATA->transparent)
    exec_fillstyle(c,state,PGTH_P_BGFILL);
  
  /* Size and add the bitmap itself */
  if (DATA->bitmap && !iserror(rdhandle((void **) &bit,PG_TYPE_BITMAP,-1,
      DATA->bitmap)) && bit) {
    VID(bitmap_getsize) (bit,&w,&h);
    align(c,DATA->align,&w,&h,&x,&y);

    /* Optional bitmask */
    if (DATA->bitmask && !iserror(rdhandle((void **) &bit,PG_TYPE_BITMAP,-1,
					   DATA->bitmask)) && bit) {
      addgrop(c,PG_GROP_BITMAP,x,y,w,h);
      c->current->param[0] = DATA->bitmask;
      c->current->param[1] = PG_LGOP_AND;
      c->current->param[2] = 0;
      c->current->param[3] = 0;
    }

    addgrop(c,PG_GROP_BITMAP,x,y,w,h);
    c->current->param[0] = DATA->bitmap;
    c->current->param[1] = DATA->lgop;
    c->current->param[2] = 0;
    c->current->param[3] = 0;
  }
}

void resizebitmap(struct widget *self);

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error bitmap_install(struct widget *self) {
  g_error e;

  e = g_malloc(&self->data,sizeof(struct bitmapdata));
  errorcheck;
  memset(self->data,0,sizeof(struct bitmapdata));
  DATA->transparent = 1;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  self->in->split = 0;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_bitmap;
  self->in->div->state = PGTH_O_BITMAP;
  DATA->align = PG_A_CENTER;
  DATA->lgop = PG_LGOP_NONE;

  return sucess;
}

void bitmap_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error bitmap_set(struct widget *self,int property, glob data) {
  hwrbitmap bit;
  int psplit;

  switch (property) {

  case PG_WP_TRANSPARENT:
    DATA->transparent = (data != 0);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_ALIGN:
    if (data > PG_AMAX) return mkerror(PG_ERRT_BADPARAM,2);
    DATA->align = (alignt) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_LGOP:
    if (data > PG_LGOPMAX) return mkerror(PG_ERRT_BADPARAM,3);
    DATA->lgop = data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_BITMAP:
    if (!data) {
      self->in->div->flags |= DIVNODE_NEED_REDRAW;
      self->dt->flags |= DIVTREE_NEED_REDRAW;
    }
    else if (!iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)) && bit) {
      DATA->bitmap = (handle) data;
      psplit = self->in->split;
      resizebitmap(self);
      if (self->in->split != psplit)
	self->in->flags |= DIVNODE_PROPAGATE_RECALC;
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(PG_ERRT_HANDLE,4);
    break;

  case PG_WP_BITMASK:
    if (!iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)) && bit) {
      DATA->bitmask = (handle) data;
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(PG_ERRT_HANDLE,5);
    break;

   default:
     return mkerror(ERRT_PASS,0);
  }

  return sucess;
}

glob bitmap_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case PG_WP_TRANSPARENT:
    return DATA->transparent;

  case PG_WP_ALIGN:
    return DATA->align;

  case PG_WP_LGOP:
    return DATA->lgop;

  case PG_WP_BITMAP:
    return (glob) DATA->bitmap;

  case PG_WP_BITMASK:
    return (glob) DATA->bitmask;
  }
  return 0;
}
 
void resizebitmap(struct widget *self) {
  hwrbitmap bit;
  int w,h;
 
  if (self->sizelock) return;
   
  if (iserror(rdhandle((void **) &bit,PG_TYPE_BITMAP,-1,DATA->bitmap)))
    return;
  if (!bit) return;
  VID(bitmap_getsize) (bit,&w,&h);

  if ((self->in->flags & DIVNODE_SPLIT_TOP) ||
      (self->in->flags & DIVNODE_SPLIT_BOTTOM))
    self->in->split = h;
  else if ((self->in->flags & DIVNODE_SPLIT_LEFT) ||
	   (self->in->flags & DIVNODE_SPLIT_RIGHT))
    self->in->split = w;
}

/* The End */











