/* $Id: label.c,v 1.38 2001/08/29 17:06:49 micahjd Exp $
 *
 * label.c - simple text widget with a filled background
 * good for titlebars, status info
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
#include <pgserver/appmgr.h>

struct labeldata {
  handle text,font;
  unsigned char transparent;
  short int align,direction;
};
#define DATA ((struct labeldata *)(self->data))

void build_label(struct gropctxt *c,unsigned short state,struct widget *self) {
  s16 x,y,w,h;
  struct fontdesc *fd;
  char *str;
  handle font = DATA->font ? DATA->font : theme_lookup(state,PGTH_P_FONT);


  if (!DATA->transparent)
     exec_fillstyle(c,state,PGTH_P_BGFILL);
   else
     /* Redraw the containing widget if we're transparent */
     redraw_bg(self);

  /* Measure the exact width and height of the text and align it */
  if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,font))
      || !fd) return;
  if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,DATA->text))
      || !str) return;
  if (DATA->direction == PG_DIR_VERTICAL)
    sizetext(fd,&h,&w,str);
  else
    sizetext(fd,&w,&h,str);
  if (w>c->w) w = c->w;
  if (h>c->h) h = c->h;
  align(c,DATA->align,&w,&h,&x,&y);

  if (font != defaultfont) {
     addgrop(c,PG_GROP_SETFONT);
     c->current->param[0] = font;
  }
  addgrop(c,PG_GROP_SETCOLOR);
  c->current->param[0] = VID(color_pgtohwr) 
     (theme_lookup(state,PGTH_P_FGCOLOR));
   
  if (DATA->direction) {
     addgrop(c,PG_GROP_SETANGLE);
     c->current->param[0] = DATA->direction; 
     y+=h;
  }
   
  addgropsz(c,PG_GROP_TEXT,x,y,w,h);
  c->current->param[0] = DATA->text;
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
  self->in->flags |= PG_S_TOP;
  self->in->split = 0;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_label;
  self->in->div->state = PGTH_O_LABEL;
  DATA->align = PG_A_CENTER;

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

  switch (property) {

  case PG_WP_TRANSPARENT:
    DATA->transparent = (data != 0);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_ALIGN:
    if (data > PG_AMAX) return mkerror(PG_ERRT_BADPARAM,11);
    DATA->align = (alignt) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_DIRECTION:
    DATA->direction = data;
    resizewidget(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_FONT:
    if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,data)) || !fd) 
      return mkerror(PG_ERRT_HANDLE,12);
    DATA->font = (handle) data;
    resizewidget(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_TEXT:
    if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,data)) || !str) 
      return mkerror(PG_ERRT_HANDLE,13);
    DATA->text = (handle) data;
    resizewidget(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_STATE:
    self->in->div->state = data;
    break;
       
  default:
    return mkerror(ERRT_PASS,0);
  }
  return sucess;
}

glob label_get(struct widget *self,int property) {
  g_error e;

  switch (property) {

  case PG_WP_TRANSPARENT:
    return DATA->transparent;

  case PG_WP_DIRECTION:
    return DATA->direction;

  case PG_WP_ALIGN:
    return DATA->align;

  case PG_WP_FONT:
    return (glob) DATA->font;

  case PG_WP_TEXT:
    return (glob) DATA->text;

  default:
    return 0;
  }
}
 
void label_resize(struct widget *self) {
  s16 w,h,m;
  struct fontdesc *fd;
  char *str;
  handle font = DATA->font ? DATA->font : 
    theme_lookup(self->in->div->state,PGTH_P_FONT);

  if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,font))
	      || !fd) return;
  if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,DATA->text))
	      || !str) return;

  if (DATA->direction == PG_DIR_VERTICAL)
    sizetext(fd,&h,&w,str);
  else
    sizetext(fd,&w,&h,str);

  m = theme_lookup(self->in->div->state,PGTH_P_MARGIN);

  self->in->div->ph = h+m;
  self->in->div->pw = w+m;
}

/* The End */
