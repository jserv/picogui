/* $Id: bitmap.c,v 1.13 2000/08/14 19:35:45 micahjd Exp $
 *
 * bitmap.c - just displays a bitmap, similar resizing and alignment to labels
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

#include <widget.h>

struct bitmapdata {
  handle bitmap,bitmask;
  int align,lgop,transparent;
  devcolort fill;
};
#define DATA ((struct bitmapdata *)(self->data))

/* display a bitmap, with alignment */
void bitmap(struct divnode *d) {
  int x,y,w,h;
  struct bitmap *bit;
  struct widget *self = d->owner;

  if (!DATA->transparent)
    grop_rect(&d->grop,0,0,d->w,d->h,DATA->fill);

  /* Here if the bitmap is null we don't want to be blitting from the
     screen... */
  if (DATA->bitmap && !iserror(rdhandle((void **) &bit,TYPE_BITMAP,-1,
      DATA->bitmap)) && bit) {
    w = bit->w;
    h = bit->h;
    align(d,DATA->align,&w,&h,&x,&y);

    /* Optional bitmask */
    if (DATA->bitmask && !iserror(rdhandle((void **) &bit,TYPE_BITMAP,-1,
	DATA->bitmask)) && bit)
      grop_bitmap(&d->grop,x,y,w,h,DATA->bitmask,LGOP_AND);
    else
      grop_null(&d->grop);

    grop_bitmap(&d->grop,x,y,w,h,DATA->bitmap,DATA->lgop);
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

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= S_TOP;
  self->in->split = 0;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->on_recalc = &bitmap;
  DATA->fill = white;
  DATA->align = A_CENTER;
  DATA->lgop = LGOP_NONE;

  return sucess;
}

void bitmap_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error bitmap_set(struct widget *self,int property, glob data) {
  struct bitmap *bit;
  int psplit;

  switch (property) {

  case WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(ERRT_BADPARAM,2);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
      DIVNODE_PROPAGATE_RECALC;
    resizebitmap(self);
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BGCOLOR:
    DATA->fill = cnvcolor(data);
    DATA->transparent = 0;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_TRANSPARENT:
    DATA->transparent = (data != 0);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_ALIGN:
    if (data > AMAX) return mkerror(ERRT_BADPARAM,2);
    DATA->align = (alignt) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_LGOP:
    if (data > LGOPMAX) return mkerror(ERRT_BADPARAM,3);
    DATA->lgop = data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BITMAP:
    if (!data) {
      self->in->div->flags |= DIVNODE_NEED_REDRAW;
      self->dt->flags |= DIVTREE_NEED_REDRAW;
    }
    else if (!iserror(rdhandle((void **)&bit,TYPE_BITMAP,-1,data)) && bit) {
      DATA->bitmap = (handle) data;
      psplit = self->in->split;
      resizebitmap(self);
      if (self->in->split != psplit)
	self->in->flags |= DIVNODE_PROPAGATE_RECALC;
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(ERRT_HANDLE,4);
    break;

  case WP_BITMASK:
    if (!iserror(rdhandle((void **)&bit,TYPE_BITMAP,-1,data)) && bit) {
      DATA->bitmask = (handle) data;
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(ERRT_HANDLE,5);
    break;

  default:
    return mkerror(ERRT_BADPARAM,6);
  }

  return sucess;
}

glob bitmap_get(struct widget *self,int property) {
  switch (property) {

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_COLOR:
    return DATA->fill;
    
  case WP_TRANSPARENT:
    return DATA->transparent;

  case WP_ALIGN:
    return DATA->align;

  case WP_LGOP:
    return DATA->lgop;

  case WP_BITMAP:
    return (glob) DATA->bitmap;

  case WP_BITMASK:
    return (glob) DATA->bitmask;
  }
  return 0;
}
 
void resizebitmap(struct widget *self) {
  struct bitmap *bit;
 
  if (iserror(rdhandle((void **) &bit,TYPE_BITMAP,-1,DATA->bitmap)))
    return;
  if (!bit) return;

  if ((self->in->flags & DIVNODE_SPLIT_TOP) ||
      (self->in->flags & DIVNODE_SPLIT_BOTTOM))
    self->in->split = bit->h;
  else if ((self->in->flags & DIVNODE_SPLIT_LEFT) ||
	   (self->in->flags & DIVNODE_SPLIT_RIGHT))
    self->in->split = bit->w;
}

/* The End */











