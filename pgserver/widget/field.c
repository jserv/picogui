/* $Id: field.c,v 1.2 2000/07/30 21:29:17 micahjd Exp $
 *
 * Single-line no-frills text editing box
 *
 * Todo: add theme support to this widget!
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

/* Buffer allocation settings */
#define FIELDBUF_DEFAULTMAX      512   /* Default setting for buffer maximum */
#define FIELDBUF_DEFAULTSIZE     20    /* Initial size for the buffer */
#define FIELDBUF_MAXCRUFT        25    /* Amount of wasted space in buffer before it is resized */
#define FIELDBUF_ALLOCSTEP       10    /* Amount to allocate when the buffer is full */

#define CURSORWIDTH 2

struct fielddata {
  handle font;
  devcolort fg,bg;
  int focus,on;

  /* Maximum size the field can hold */
  unsigned int bufmax;

  /* The pointer, handle, and size of the text buffer */
  char *buffer;
  handle hbuffer;
  unsigned int bufsize;

  /* Amount of space in the buffer that is used (including null termination) */
  unsigned int bufuse;
};
#define DATA ((struct fielddata *)(self->data))

g_error bufcheck_grow(struct widget *self);     /* Check the buffer
						   before adding a char */
g_error bufcheck_shrink(struct widget *self);   /* Check before removing */
void fieldstate(struct widget *self);

void field(struct divnode *d) {
  int x,y,w,h;
  struct fontdesc *fd;
  struct widget *self = d->owner;

  /* Center the font vertically and use the same amount of margin on the side */
  if (rdhandle((void **)&fd,TYPE_FONTDESC,-1,DATA->font).type
      != ERRT_NONE || !fd) return;
  
  /* Draw order: background, text, cursor, border */
  grop_rect(&d->grop,-1,-1,-1,-1,DATA->bg);
  grop_text(&d->grop,fd->margin,(d->h>>1) - (fd->font->h>>1),
	    DATA->font,DATA->fg,DATA->hbuffer);
  grop_rect(&d->grop,0,2,CURSORWIDTH,d->h-4,DATA->bg);
  grop_frame(&d->grop,-1,-1,-1,-1,black);

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

  /* Set up the buffer */
  e = g_malloc((void *) &DATA->buffer,FIELDBUF_DEFAULTSIZE);
  if (e.type != ERRT_NONE) return e;
  *DATA->buffer = 0;
  DATA->bufmax  = FIELDBUF_DEFAULTMAX;
  DATA->bufsize = FIELDBUF_DEFAULTSIZE;
  DATA->bufuse  = 1;
  e = mkhandle(&DATA->hbuffer,TYPE_STRING | HFLAG_NFREE,-1,DATA->buffer);
  if (e.type != ERRT_NONE) return e;

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

  self->trigger_mask = TRIGGER_UP | TRIGGER_ACTIVATE | TRIGGER_CHAR |
    TRIGGER_DEACTIVATE | TRIGGER_DOWN | TRIGGER_RELEASE;

  return sucess;
}

void field_remove(struct widget *self) {
  handle_free(-1,DATA->hbuffer);
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
    printf("Field WP_TEXT: implement me please!\n");
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
    return (glob) DATA->hbuffer;

  case WP_SCROLL:
    return -self->in->div->ty;

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
    return;

  case TRIGGER_UP:
    if (DATA->on && param->mouse.chbtn==1) {
      DATA->on=0;
      request_focus(self);
    }
    return;
    
  case TRIGGER_RELEASE:
    if (param->mouse.chbtn==1)
      DATA->on=0;
    return;
    
    /* Update visual appearance to reflect focus or lack of focus */
    
  case TRIGGER_ACTIVATE:
    DATA->focus = 1;
    break;
  case TRIGGER_DEACTIVATE:
    DATA->focus = 0;
    break;
    
    /* Keyboard input */

  case TRIGGER_CHAR:
    
    /* Misc. keys */
    switch (param->kbd.key) {
    case PGKEY_RETURN:
    case PGKEY_TAB:
      return;
    default:
    }


    /* Backspace? */
    if (param->kbd.key == PGKEY_BACKSPACE) {
      if (DATA->bufuse<=1) return;
      DATA->buffer[(--DATA->bufuse)-1] = 0;
      bufcheck_shrink(self);
      break;
    }

    /* Add a character */
    bufcheck_grow(self);
    if (DATA->bufuse>=DATA->bufsize) return;
    DATA->buffer[DATA->bufuse-1] = param->kbd.key;
    DATA->buffer[DATA->bufuse++] = 0;
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
  int tw,th;
  struct fontdesc *fd;

  /* Size the text.  If this is a problem, we could keep a running
     total of the text width as it is done, but this whole widget
     so far is a quick hack anyway...
  */
  if (rdhandle((void**)&fd,TYPE_FONTDESC,-1,DATA->font).type != 
      ERRT_NONE || !fd) return;
  sizetext(fd,&tw,&th,DATA->buffer);

  /* If the whole text fits in the field, left justify it. Otherwise, 
     right justify
  */
  if (tw<self->in->div->w) {
    self->in->div->grop->next->x = fd->margin;
    /* Move the cursor to the end of the text */
    self->in->div->grop->next->next->x = tw;
  }
  else {
    /* Right justify, cursor at right side of widget */
    self->in->div->grop->next->x = 
      (self->in->div->grop->next->next->x = 
       self->in->div->w - fd->margin - CURSORWIDTH) - tw + fd->margin;    
  }

  /* Appear or disappear the cursor depending on focus */
  if (DATA->focus)
    self->in->div->grop->next->next->param.c = DATA->fg;
  else
    self->in->div->grop->next->next->param.c = DATA->bg;
  
}

/* If the buffer doesn't have room for one more char, enlarge it */
g_error bufcheck_grow(struct widget *self) {
  g_error e;

  if (DATA->bufuse < DATA->bufsize) return sucess;

  DATA->bufsize += FIELDBUF_ALLOCSTEP;
  if (DATA->bufsize > DATA->bufmax) DATA->bufsize = DATA->bufmax;

  e = g_realloc((void *)&DATA->buffer,DATA->bufsize);
  if (e.type != ERRT_NONE) return e;

  /* Possible race condition here? */

  return rehandle(DATA->hbuffer,DATA->buffer);
}

/* If there's too much wasted space, shrink the buffer */
g_error bufcheck_shrink(struct widget *self) {
  g_error e;

  if ((DATA->bufsize-DATA->bufuse)<FIELDBUF_MAXCRUFT) return sucess;

  DATA->bufsize -= FIELDBUF_MAXCRUFT;

  e = g_realloc((void *)&DATA->buffer,DATA->bufsize);
  if (e.type != ERRT_NONE) return e;

  /* Possible race condition here? */

  return rehandle(DATA->hbuffer,DATA->buffer);
}

/* The End */











