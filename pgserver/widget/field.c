/* $Id: field.c,v 1.18 2001/02/14 05:13:19 micahjd Exp $
 *
 * Single-line no-frills text editing box
 *
 * Todo: add theme support to this widget!
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

#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/appmgr.h>

/* Buffer allocation settings */
#define FIELDBUF_DEFAULTMAX      512   /* Default setting for buffer maximum */
#define FIELDBUF_DEFAULTSIZE     20    /* Initial size for the buffer */
#define FIELDBUF_MAXCRUFT        25    /* Amount of wasted space in buffer before it is resized */
#define FIELDBUF_ALLOCSTEP       10    /* Amount to allocate when the buffer is full */

#define CURSORWIDTH 2
#define FLASHTIME_ON   250
#define FLASHTIME_OFF  150

struct fielddata {
  handle font;
  int focus,on,flash_on;

  /* Maximum size the field can hold */
  unsigned int bufmax;

  /* The pointer, handle, and size of the text buffer */
  char *buffer;
  handle hbuffer;
  unsigned int bufsize;

  /* Saved grops for dynamic updates */
  struct gropnode *cursor,*text;
   
  /* Amount of space in the buffer that is used (including null termination) */
  unsigned int bufuse;

  /* A copy of the context to start drawing in */
  struct gropctxt startctxt;
};
#define DATA ((struct fielddata *)(self->data))

g_error bufcheck_grow(struct widget *self);     /* Check the buffer
						   before adding a char */
g_error bufcheck_shrink(struct widget *self);   /* Check before removing */
void fieldstate(struct widget *self);

void build_field(struct gropctxt *c,unsigned short state,struct widget *self) {
  int x,y,w,h;
  struct fontdesc *fd;
  handle font = DATA->font ? DATA->font : theme_lookup(state,PGTH_P_FONT);
  hwrcolor fg;
   
  exec_fillstyle(c,state,PGTH_P_BGFILL);

  /* Center the font vertically and use the same amount of margin on the side */
  if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,
		       font)) || !fd) return;
  
  /* Save it for later */
  DATA->startctxt = *c;

  /* The text itself */
  addgrop(c,PG_GROP_TEXT,fd->margin,(c->h>>1) - (fd->font->h>>1),1,1);
  c->current->param[0] = DATA->hbuffer;
  c->current->param[1] = font;
  c->current->param[2] = fg = (*vid->color_pgtohwr)
     (theme_lookup(state,PGTH_P_FGCOLOR)); 
  DATA->text = c->current;

  /* Cursor 
   * FIXME: The cursor doesn't use themes! (much)
   */
  addgrop(c,PG_GROP_RECT,0,c->y+2,CURSORWIDTH,c->h-4);
  c->current->param[0] = fg;
  DATA->cursor = c->current;
   
  fieldstate(self);
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error field_install(struct widget *self) {
  g_error e;
  int owner=-1;

  e = g_malloc(&self->data,sizeof(struct fielddata));
  errorcheck;
  memset(self->data,0,sizeof(struct fielddata));

  /* Set up the buffer */
  e = g_malloc((void *) &DATA->buffer,FIELDBUF_DEFAULTSIZE);
  errorcheck;
  *DATA->buffer = 0;
  DATA->bufmax  = FIELDBUF_DEFAULTMAX;
  DATA->bufsize = FIELDBUF_DEFAULTSIZE;
  DATA->bufuse  = 1;
  e = mkhandle(&DATA->hbuffer,PG_TYPE_STRING | HFLAG_NFREE,self->owner,DATA->buffer);
  errorcheck;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  self->in->split = 20;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_field;
  self->in->div->state = PGTH_O_FIELD;

  self->trigger_mask = TRIGGER_UP | TRIGGER_ACTIVATE | TRIGGER_CHAR |
    TRIGGER_DEACTIVATE | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_TIMER;

  return sucess;
}

void field_remove(struct widget *self) {
  handle_free(-1,DATA->hbuffer);
  g_free(DATA->buffer);

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

  case PG_WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,43);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_FONT:
    if (iserror(rdhandle((void **)&fd,
			 PG_TYPE_FONTDESC,-1,data)) || !fd) 
      return mkerror(PG_ERRT_HANDLE,44); 
    DATA->font = (handle) data;
    psplit = self->in->split;
    if (self->in->split != psplit) {
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

    /* FIXME: PG_WP_TEXT
       case PG_WP_TEXT:

       break;
    */

  default:
    return mkerror(PG_ERRT_BADPARAM,45);
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

  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case PG_WP_FONT:
    return (glob) DATA->font;

  case PG_WP_TEXT:
    return (glob) DATA->hbuffer;

  case PG_WP_SCROLL:
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
    
  case TRIGGER_DEACTIVATE:
    DATA->focus = 0;
    DATA->flash_on = 0;
    break;

  case TRIGGER_ACTIVATE:
    DATA->focus = 1;
    /* No break; here! Get TRIGGER_TIMER to set up the flash timer*/
    
  case TRIGGER_TIMER:
    if (DATA->focus==0) break;

    DATA->flash_on = !DATA->flash_on;

    /* Set it again... */
    install_timer(self,(DATA->flash_on ? 
			FLASHTIME_ON : FLASHTIME_OFF ));
    break;

    /* Keyboard input */

  case TRIGGER_CHAR:
    
    /* Misc. keys */
    switch (param->kbd.key) {

    case PGKEY_RETURN:
      /* Pass on a return to the app */
      post_event(PG_WE_ACTIVATE,self,0,0,NULL);
      return;

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


  /* If we're busy rebuilding the grop list, don't bother poking
     at the individual nodes */
//  if (self->in->div->grop_lock || !self->in->div->grop)
//    return;

  /* Update stuff */
//  fieldstate(self);
  div_setstate(self->in->div,self->in->div->state);
}

/* Apply the current visual state */
void fieldstate(struct widget *self) {
  int tw,th;
  struct fontdesc *fd;
  handle font = DATA->font ? DATA->font : 
   theme_lookup(self->in->div->state,PGTH_P_FONT);
   
  /* Size the text.  If this is a problem, we could keep a running
     total of the text width as it is done, but this whole widget
     so far is a quick hack anyway...
  */
  if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
		       font)) || !fd) return;
  sizetext(fd,&tw,&th,DATA->buffer);

  /* If the whole text fits in the field, left justify it. Otherwise, 
     right justify
  */
  if (tw<self->in->div->w) {
    DATA->text->x = fd->margin;
    /* Move the cursor to the end of the text */
    DATA->cursor->x = tw;
  }
  else {
    /* Right justify, cursor at right side of widget */
    DATA->text->x = (DATA->cursor->x = 
       self->in->div->w - fd->margin - CURSORWIDTH) - tw + fd->margin;    
  }

  /* Appear or disappear the cursor depending on focus and cursor
     flashing state */
  DATA->cursor->type = DATA->flash_on ? PG_GROP_RECT : PG_GROP_NULL; 
}

/* If the buffer doesn't have room for one more char, enlarge it */
g_error bufcheck_grow(struct widget *self) {
  g_error e;

  if (DATA->bufuse < DATA->bufsize) return sucess;

  DATA->bufsize += FIELDBUF_ALLOCSTEP;
  if (DATA->bufsize > DATA->bufmax) DATA->bufsize = DATA->bufmax;

  e = g_realloc((void *)&DATA->buffer,DATA->bufsize);
  errorcheck;

  /* Possible race condition here? */

  return rehandle(DATA->hbuffer,DATA->buffer);
}

/* If there's too much wasted space, shrink the buffer */
g_error bufcheck_shrink(struct widget *self) {
  g_error e;

  if ((DATA->bufsize-DATA->bufuse)<FIELDBUF_MAXCRUFT) return sucess;

  DATA->bufsize -= FIELDBUF_MAXCRUFT;

  e = g_realloc((void *)&DATA->buffer,DATA->bufsize);
  errorcheck;

  /* Possible race condition here? */

  return rehandle(DATA->hbuffer,DATA->buffer);
}

/* The End */











