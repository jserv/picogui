/* $Id: label.c,v 1.6 2000/06/03 16:57:59 micahjd Exp $
 *
 * label.c - simple text widget with a filled background
 * good for titlebars, status info
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
#include <divtree.h>
#include <font.h>
#include <g_malloc.h>
#include <appmgr.h>

void resizelabel(struct widget *self);

/* param.text */
void text(struct divnode *d) {
  /* This aligns the text in the available rectangle
     using the value of d->param.text.align. It is seperated
     from the edge by the font's hspace/vspace
  */
  int x,y,w,h;
  struct fontdesc *fd;
  char *str;

  /* Measure the exact width and height of the text and align it */
  if (rdhandle((void **)&fd,TYPE_FONTDESC,-1,d->param.text.fd).type
      != ERRT_NONE || !fd) return;
  if (rdhandle((void **)&str,TYPE_STRING,-1,d->param.text.string).type
      != ERRT_NONE || !str) return;
  sizetext(fd,&w,&h,str);
  if (w>d->w) w = d->w;
  if (h>d->h) h = d->h;
  align(d,d->param.text.align,&w,&h,&x,&y);

  if (!d->param.text.transparent)
    grop_rect(&d->grop,0,0,d->w,d->h,d->param.text.fill);

  grop_text(&d->grop,x,y,d->param.text.fd,d->param.text.col,
	    d->param.text.string);
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error label_install(struct widget *self) {
  g_error e;

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_TOP;
  self->in->split = 0;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &text;
  self->in->div->param.text.fill = white;
  self->in->div->param.text.align = A_CENTER;
  self->in->div->param.text.transparent = 0;
  self->in->div->param.text.fd = defaultfont;
  self->in->div->param.text.string = 0;

  return sucess;
}

void label_remove(struct widget *self) {
  r_divnode_free(self->in);
}

g_error label_set(struct widget *self,int property, glob data) {
  g_error e;
  struct fontdesc *fd;
  char *str;
  int psplit;

  switch (property) {

  case WP_SIDE:
    if ((data != S_LEFT) && (data != S_RIGHT) && (data != S_TOP) &&
	(data != S_BOTTOM)) return mkerror(ERRT_BADPARAM,
	"WP_SIDE param is not a valid side value (label)");
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    resizelabel(self);
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_COLOR:
    self->in->div->param.text.col = cnvcolor(data);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BGCOLOR:
    self->in->div->param.text.fill = cnvcolor(data);
    self->in->div->param.text.transparent = 0;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_TRANSPARENT:
    self->in->div->param.text.transparent = (data != 0);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_ALIGN:
    if (data > AMAX) return mkerror(ERRT_BADPARAM,
		     "WP_ALIGN param is not a valid align value (label)");
    if (data==A_ALL || data==A_CENTER || 
	self->in->div->param.text.align==A_ALL ||
	self->in->div->param.text.align==A_CENTER) {
      self->in->div->param.text.align = (alignt) data;
      resizelabel(self);
    }
    else {
      self->in->div->param.text.align = (alignt) data;
    }
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_FONT:
    if (rdhandle((void **)&fd,TYPE_FONTDESC,-1,data).type!=ERRT_NONE || !fd) 
      return mkerror(ERRT_HANDLE,"WP_FONT invalid font handle (label)");
    self->in->div->param.text.fd = (handle) data;
    psplit = self->in->split;
    resizelabel(self);
    if (self->in->split != psplit)
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_TEXT:
    if (rdhandle((void **)&str,TYPE_STRING,-1,data).type!=ERRT_NONE || !str) 
      return mkerror(ERRT_HANDLE,"WP_TEXT invalid string handle (label)");
    self->in->div->param.text.string = (handle) data;
    psplit = self->in->split;
    resizelabel(self);
    if (self->in->split != psplit)
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for label");
  }
  return sucess;
}

glob label_get(struct widget *self,int property) {
  g_error e;
  handle h;

  switch (property) {

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_BGCOLOR:
    return self->in->div->param.text.fill;

  case WP_COLOR:
    return self->in->div->param.text.col;

  case WP_TRANSPARENT:
    return self->in->div->param.text.transparent;

  case WP_ALIGN:
    return self->in->div->param.text.align;

  case WP_FONT:
    return (glob) self->in->div->param.text.fd;

  case WP_TEXT:
    return (glob) self->in->div->param.text.string;

  default:
    return 0;
  }
}
 
void resizelabel(struct widget *self) {
  int w,h;
  struct fontdesc *fd;
  char *str;

  if (self->in->div->param.bitmap.align == A_ALL ||
      self->in->div->param.bitmap.align == A_CENTER)
    /* Expand to all free space (div will clip this) */
    self->in->split = (HWR_WIDTH>HWR_HEIGHT) ? HWR_WIDTH : HWR_HEIGHT;
  else {
    
    if (rdhandle((void **)&fd,TYPE_FONTDESC,-1,self->in->div->param.text.fd).
	type != ERRT_NONE || !fd) return;
    if (rdhandle((void **)&str,TYPE_STRING,-1,self->in->div->param.text.string).
	type != ERRT_NONE || !str) return;
    
    sizetext(fd,&w,&h,str);
    
    if ((self->in->flags & DIVNODE_SPLIT_TOP) ||
	(self->in->flags & DIVNODE_SPLIT_BOTTOM))
      self->in->split = h;
    else if ((self->in->flags & DIVNODE_SPLIT_LEFT) ||
	     (self->in->flags & DIVNODE_SPLIT_RIGHT))
      self->in->split = w;
  }
}

/* The End */
