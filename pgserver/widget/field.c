/* $Id: field.c,v 1.1 2000/07/28 10:09:15 micahjd Exp $
 *
 * Single-line no-frills text editing box
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

struct fielddata {
  handle text,font;
  devcolort fg,bg;
  int focus,on;
};
#define DATA ((struct fielddata *)(self->data))

void fieldstate(struct widget *self);

void field(struct divnode *d) {
  /* This aligns the text in the available rectangle
     using the value of d->param.text.align. It is seperated
     from the edge by the font's hspace/vspace
  */
  int x,y,w,h;
  struct fontdesc *fd;
  char *str;
  struct widget *self = d->owner;

  /* Measure the exact width and height of the text and align it */
  if (rdhandle((void **)&fd,TYPE_FONTDESC,-1,DATA->font).type
      != ERRT_NONE || !fd) return;
  if (rdhandle((void **)&str,TYPE_STRING,-1,DATA->text).type
      != ERRT_NONE || !str) return;
  sizetext(fd,&w,&h,str);
  if (w>d->w) w = d->w;
  if (h>d->h) h = d->h;
  align(d,A_LEFT,&w,&h,&x,&y);

  grop_rect(&d->grop,-1,-1,-1,-1,DATA->bg);
  grop_frame(&d->grop,-1,-1,-1,-1,black);

  grop_text(&d->grop,x,y,DATA->font,DATA->fg,DATA->text);

  fieldstate(self);
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error field_install(struct widget *self) {
  g_error e;

  e = g_malloc(&self->data,sizeof(struct fielddata));
  if (e.type != ERRT_NONE) return e;
  memset(self->data,0,sizeof(struct fielddata));

  e = newdiv(&self->in,self);
  if (e.type != ERRT_NONE) return e;
  self->in->flags |= S_TOP;
  self->in->split = 20;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  if (e.type != ERRT_NONE) return e;
  self->in->div->on_recalc = &field;
  DATA->bg = white;
  DATA->font = defaultfont;

  self->trigger_mask = TRIGGER_UP | TRIGGER_ACTIVATE |
    TRIGGER_DEACTIVATE | TRIGGER_DOWN | TRIGGER_RELEASE;

  return sucess;
}

void field_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error field_set(struct widget *self,int property, glob data) {
  g_error e;
  struct fontdesc *fd;
  char *str;
  int psplit;

  switch (property) {

  case WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(ERRT_BADPARAM,
	"WP_SIDE param is not a valid side value (field)");
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_COLOR:
    DATA->fg = cnvcolor(data);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BGCOLOR:
    DATA->bg = cnvcolor(data);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_FONT:
    if (rdhandle((void **)&fd,TYPE_FONTDESC,-1,data).type!=ERRT_NONE || !fd) 
      return mkerror(ERRT_HANDLE,"WP_FONT invalid font handle (field)");
    DATA->font = (handle) data;
    psplit = self->in->split;
    if (self->in->split != psplit) {
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_TEXT:
    if (rdhandle((void **)&str,TYPE_STRING,-1,data).type!=ERRT_NONE || !str) 
      return mkerror(ERRT_HANDLE,"WP_TEXT invalid string handle (field)");
    DATA->text = (handle) data;
    psplit = self->in->split;
    if (self->in->split != psplit) {
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_SCROLL:
    self->in->div->ty = -data;
    self->in->div->flags |= DIVNODE_SCROLL_ONLY;
    self->dt->flags |= DIVTREE_NEED_REDRAW;
    break;

  default:
    return mkerror(ERRT_BADPARAM,"Invalid property for field");
  }
  return sucess;
}

glob field_get(struct widget *self,int property) {
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

  case WP_FONT:
    return (glob) DATA->font;

  case WP_TEXT:
    return (glob) DATA->text;

  case WP_SCROLL:
    return -self->in->div->ty;

  case WP_VIRTUALH:
    if (rdhandle((void**)&str,TYPE_STRING,-1,DATA->text).type != 
	ERRT_NONE || !str) break;
    if (rdhandle((void**)&fd,TYPE_FONTDESC,-1,DATA->font).type != 
	ERRT_NONE || !fd) break;
    sizetext(fd,&tw,&th,str);
    return th;

  default:
    return 0;
  }
}

void field_trigger(struct widget *self,long type,union trigparam *param) {
  switch (type) {

    /* When clicked, request keyboard focus */

  case TRIGGER_DOWN:
    if (param->mouse.chbtn==1)
      DATA->on=1;
    break;

  case TRIGGER_UP:
    if (DATA->on && param->mouse.chbtn==1) {
      DATA->on=0;
      request_focus(self);
    }
    break;
    
  case TRIGGER_RELEASE:
    if (param->mouse.chbtn==1)
      DATA->on=0;
    break;
    
    /* Update visual appearance to reflect focus or lack of focus */
    
  case TRIGGER_ACTIVATE:
    DATA->focus = 1;
    break;
  case TRIGGER_DEACTIVATE:
    DATA->focus = 0;
    break;
    
  }

  /* Update stuff */
  fieldstate(self);

  self->in->div->flags |= DIVNODE_NEED_REDRAW;
  self->dt->flags |= DIVTREE_NEED_REDRAW;   
  if (self->dt==dts->top) update();
}

/* Apply the current visual state */
void fieldstate(struct widget *self) {

  /* Ummm.... fix this */
  if (DATA->focus)
    self->in->div->grop->next->param.c = black;
  else
    self->in->div->grop->next->param.c = white;
  
}

/* The End */


