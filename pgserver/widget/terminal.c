/* $Id: terminal.c,v 1.2 2000/12/12 00:51:47 micahjd Exp $
 *
 * terminal.c - a character-cell-oriented display widget for terminal
 *              emulators and things.
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

#include <pgserver/widget.h>

struct termdata {
  /* character info */
  handle font;
  //  int charw,charh;

  /* Text buffer */
  handle hbuffer;
  char *buffer;
  int bufferw,bufferh,buffersize;
  int yoffset;
  char *cursor;
};
#define DATA ((struct termdata *)(self->data))

/* Default text attribute */
#define ATTR_DEFAULT 0x07

void build_terminal(struct gropctxt *c,unsigned short state,struct widget *self) {
  addgrop(c,PG_GROP_TEXTGRID,c->x,c->y,c->w,c->h);
  c->current->param[0] = DATA->hbuffer;
  c->current->param[1] = DATA->font;
  c->current->param[2] = DATA->bufferw;
  c->current->param[3] = DATA->yoffset * DATA->bufferw;
}

g_error terminal_install(struct widget *self) {
  g_error e;
  char *p,*s = "Hello, World";

  e = g_malloc(&self->data,sizeof(struct termdata));
  errorcheck;
  memset(self->data,0,sizeof(struct termdata));

  DATA->bufferw = 50;
  DATA->bufferh = 25;

  DATA->buffersize = DATA->bufferw*DATA->bufferh*2;
  e = g_malloc(&DATA->buffer,DATA->buffersize+1);
  errorcheck;
  DATA->buffer[DATA->buffersize] = 0;
  DATA->cursor = DATA->buffer;
  memset(DATA->buffer,ATTR_DEFAULT,DATA->buffersize);
  e = mkhandle(&DATA->hbuffer,PG_TYPE_STRING,-1,DATA->buffer);
  errorcheck;
  e = findfont(&DATA->font,-1,"Console",0,PG_FSTYLE_FIXED);
  errorcheck;

  p = DATA->buffer+1;
  while (*s) {
    *p = *s;
    p += 2;
    s++;
  }

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  self->in->split = 0;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_terminal;

  self->trigger_mask = TRIGGER_STREAM;

  return sucess;
}

void terminal_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error terminal_set(struct widget *self,int property, glob data) {
  g_error e;
  struct fontdesc *fd;
  char *str;

  switch (property) {

  case PG_WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,11);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_FONT:
    if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,data)) || !fd) 
      return mkerror(PG_ERRT_HANDLE,12);
    DATA->font = (handle) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

    //  default:
    //    return mkerror(PG_ERRT_BADPARAM,14);
  }
  return sucess;
}

glob terminal_get(struct widget *self,int property) {
  g_error e;
  handle h;
  int tw,th;
  char *str;
  struct fontdesc *fd;

  switch (property) {

  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case PG_WP_FONT:
    return (glob) DATA->font;

  default:
    return 0;
  }
}

void terminal_trigger(struct widget *self,long type,union trigparam *param) {
  switch (type) {

  case TRIGGER_STREAM:
    while (*param->stream.data) {
      *(DATA->cursor+1) = *param->stream.data;
      *DATA->cursor ^= 0xFF;
      DATA->cursor += 2;
      *DATA->cursor ^= 0xFF;
      param->stream.data++;
      if (DATA->cursor >= DATA->buffer + DATA->buffersize)
	DATA->cursor = DATA->buffer;
    }
    self->in->div->flags |= DIVNODE_NEED_REDRAW;
    self->dt->flags |= DIVTREE_NEED_REDRAW;
    break;

  }
}
 
 
/* The End */
