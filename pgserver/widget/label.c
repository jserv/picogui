/* $Id: label.c,v 1.19 2000/08/28 03:30:43 micahjd Exp $
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

struct labeldata {
  handle text,font;
  int transparent,align;
  pgcolor bg,fg;
};
#define DATA ((struct labeldata *)(self->data))

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
  struct widget *self = d->owner;

  /* Measure the exact width and height of the text and align it */
  if (iserror(rdhandle((void **)&fd,TYPE_FONTDESC,-1,DATA->font))
      || !fd) return;
  if (iserror(rdhandle((void **)&str,TYPE_STRING,-1,DATA->text))
      || !str) return;
  sizetext(fd,&w,&h,str);
  if (w>d->w) w = d->w;
  if (h>d->h) h = d->h;
  align(d,DATA->align,&w,&h,&x,&y);

  if (!DATA->transparent)
    grop_rect(&d->grop,-1,-1,-1,-1,DATA->bg);

  grop_text(&d->grop,x,y,DATA->font,DATA->fg,DATA->text);
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error label_install(struct widget *self) {
  g_error e;

  e = g_malloc(&self->data,sizeof(struct labeldata));
  errorcheck;
  memset(self->data,0,sizeof(struct labeldata));
  DATA->transparent = 1;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= S_TOP;
  self->in->split = 0;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->on_recalc = &text;
  DATA->bg = 0xFFFFFF;
  DATA->align = A_CENTER;
  DATA->font = defaultfont;

  return sucess;
}

void label_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error label_set(struct widget *self,int property, glob data) {
  g_error e;
  struct fontdesc *fd;
  char *str;
  int psplit;

  switch (property) {

  case WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(ERRT_BADPARAM,11);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    resizelabel(self);
    if (DATA->transparent)
      redraw_bg(self);
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_COLOR:
    DATA->fg = data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BGCOLOR:
    DATA->bg = data;
    DATA->transparent = 0;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_TRANSPARENT:
    DATA->transparent = (data != 0);
    if (DATA->transparent)
      redraw_bg(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_ALIGN:
    if (data > AMAX) return mkerror(ERRT_BADPARAM,11);
    DATA->align = (alignt) data;
    if (DATA->transparent)
      redraw_bg(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_FONT:
    if (iserror(rdhandle((void **)&fd,TYPE_FONTDESC,-1,data)) || !fd) 
      return mkerror(ERRT_HANDLE,12);
    DATA->font = (handle) data;
    psplit = self->in->split;
    resizelabel(self);
    if (self->in->split != psplit) {
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    if (DATA->transparent)
      redraw_bg(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_TEXT:
    if (iserror(rdhandle((void **)&str,TYPE_STRING,-1,data)) || !str) 
      return mkerror(ERRT_HANDLE,13);
    DATA->text = (handle) data;
    psplit = self->in->split;
    resizelabel(self);
    if (self->in->split != psplit) {
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    if (DATA->transparent)
      redraw_bg(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_SCROLL:
    self->in->div->ty = -data;
    self->in->div->flags |= DIVNODE_SCROLL_ONLY;
    self->dt->flags |= DIVTREE_NEED_REDRAW;
    break;

  default:
    return mkerror(ERRT_BADPARAM,14);
  }
  return sucess;
}

glob label_get(struct widget *self,int property) {
  g_error e;
  handle h;
  int tw,th;
  char *str;
  struct fontdesc *fd;

  switch (property) {

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_BGCOLOR:
    return DATA->bg;

  case WP_COLOR:
    return DATA->fg;

  case WP_TRANSPARENT:
    return DATA->transparent;

  case WP_ALIGN:
    return DATA->align;

  case WP_FONT:
    return (glob) DATA->font;

  case WP_TEXT:
    return (glob) DATA->text;

  case WP_SCROLL:
    return -self->in->div->ty;

  case WP_VIRTUALH:
    if (iserror(rdhandle((void**)&str,TYPE_STRING,-1,DATA->text)) 
        || !str) break;
    if (iserror(rdhandle((void**)&fd,TYPE_FONTDESC,-1,DATA->font)) 
        || !fd) break;
    sizetext(fd,&tw,&th,str);
    return th;

  default:
    return 0;
  }
}
 
void resizelabel(struct widget *self) {
  int w,h;
  struct fontdesc *fd;
  char *str;

  /* With S_ALL we'll get ignored anyway... */
  if (self->in->flags & S_ALL) return;

  if (iserror(rdhandle((void **)&fd,TYPE_FONTDESC,-1,DATA->font))
	      || !fd) return;
  if (iserror(rdhandle((void **)&str,TYPE_STRING,-1,DATA->text))
	      || !str) return;
  
  sizetext(fd,&w,&h,str);
  
  if ((self->in->flags & S_TOP) ||
      (self->in->flags & S_BOTTOM))
    self->in->split = h;
  else if ((self->in->flags & S_LEFT) ||
	   (self->in->flags & S_RIGHT))
    self->in->split = w;
}

/* The End */
