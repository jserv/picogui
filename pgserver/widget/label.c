/*
 * label.c - simple text widget with a filled background
 * good for titlebars, status info
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <widget.h>
#include <divtree.h>
#include <font.h>
#include <g_malloc.h>

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

  switch (property) {

  case WP_LABEL_SIDE:
    if ((data != S_LEFT) && (data != S_RIGHT) && (data != S_TOP) &&
	(data != S_BOTTOM)) return mkerror(ERRT_BADPARAM,
	"WP_LABEL_SIDE param is not a valid side value");
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC;
    resizelabel(self);
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_LABEL_COLOR:
    self->in->div->param.text.fill = cnvcolor(data);
    self->in->div->param.text.transparent = 0;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_LABEL_TRANSPARENT:
    self->in->div->param.text.transparent = (data != 0);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_LABEL_ALIGN:
    if (data > AMAX) return mkerror(ERRT_BADPARAM,
		     "WP_LABEL_ALIGN param is not a valid align value");
    self->in->div->param.text.align = (alignt) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_LABEL_FONT:
    if (rdhandle((void **)&fd,TYPE_FONTDESC,-1,data).type!=ERRT_NONE || !fd) 
      return mkerror(ERRT_HANDLE,"WP_LABEL_FONT invalid font handle");
    self->in->div->param.text.fd = (handle) data;
    resizelabel(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_LABEL_TEXT:
    if (rdhandle((void **)&str,TYPE_STRING,-1,data).type!=ERRT_NONE || !str) 
      return mkerror(ERRT_HANDLE,"WP_LABEL_TEXT invalid string handle");
    self->in->div->param.text.string = (handle) data;
    resizelabel(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;
  }
  return sucess;
}

glob label_get(struct widget *self,int property) {
  g_error e;
  handle h;

  switch (property) {

  case WP_LABEL_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_LABEL_COLOR:
    return self->in->div->param.text.fill;

  case WP_LABEL_TRANSPARENT:
    return self->in->div->param.text.transparent;

  case WP_LABEL_ALIGN:
    return self->in->div->param.text.align;

  case WP_LABEL_FONT:
    return (glob) self->in->div->param.text.fd;

  case WP_LABEL_TEXT:
    return (glob) self->in->div->param.text.string;
  }
}
 
void resizelabel(struct widget *self) {
  int w,h;
  struct fontdesc *fd;
  char *str;

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

/* The End */
