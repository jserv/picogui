/* $Id: field.c,v 1.36 2001/12/16 20:23:15 micahjd Exp $
 *
 * field.c - Single-line no-frills text editing box
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
 * Shane R. Nay <shane@minirl.com> 
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

#define CURSORWIDTH 1
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
  if (font != defaultfont) {
     addgrop(c,PG_GROP_SETFONT);
     c->current->param[0] = font;
  }
  addgrop(c,PG_GROP_SETCOLOR);
  c->current->param[0] = VID(color_pgtohwr) 
     (theme_lookup(state,PGTH_P_FGCOLOR)); 
  addgropsz(c,PG_GROP_TEXT,fd->margin,((c->h - fd->font->ascent - 
					fd->font->descent)>>1) + 
	    c->y,1,1);
  c->current->param[0] = DATA->hbuffer;
  DATA->text = c->current;

  /* Cursor 
   * FIXME: The cursor doesn't use themes! (much)
   */
  addgropsz(c,PG_GROP_RECT,0,c->y,CURSORWIDTH,c->h);
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
  self->in->div->flags |= DIVNODE_HOTSPOT;

  self->trigger_mask = TRIGGER_UP | TRIGGER_ACTIVATE | TRIGGER_CHAR |
    TRIGGER_DEACTIVATE | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_TIMER;

  return success;
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

  case PG_WP_FONT:
    if (iserror(rdhandle((void **)&fd,
			 PG_TYPE_FONTDESC,-1,data)) || !fd) 
      return mkerror(PG_ERRT_HANDLE,44); 
    DATA->font = (handle) data;
    psplit = self->in->split;
    if (self->in->split != psplit) {
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    resizewidget(self); 
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

   case PG_WP_TEXT:
     /* Copy the provided data into our private buffer */

     if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,data)) || !str) 
       return mkerror(PG_ERRT_HANDLE,13);
      
     /* Need resize? */
     DATA->bufuse = strlen(str) + 1;
     if (DATA->bufsize < DATA->bufuse) {
	DATA->bufsize = DATA->bufuse + FIELDBUF_ALLOCSTEP;
	e = g_realloc((void *)&DATA->buffer,DATA->bufsize);
	errorcheck;
	e = rehandle(DATA->hbuffer,DATA->buffer,PG_TYPE_STRING);
	errorcheck;
     }
     
     /* Update text */
     strcpy(DATA->buffer,str);
     resizewidget(self); 
     self->in->flags |= DIVNODE_NEED_RECALC;
     self->dt->flags |= DIVTREE_NEED_RECALC;
     break;

   default:
     return mkerror(ERRT_PASS,0);
  }
  return success;
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

    case PGKEY_ESCAPE:
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
  
  /* Always redraw the field */
  div_setstate(self->in->div,self->in->div->state, 1);
}

/* Apply the current visual state */
void fieldstate(struct widget *self) {
  s16 tw,th;
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
    DATA->text->r.x = fd->margin;
    /* Move the cursor to the end of the text */
    DATA->cursor->r.x = tw;
  }
  else {
    /* Right justify, cursor at right side of widget */
    DATA->text->r.x = (DATA->cursor->r.x = 
       self->in->div->w - fd->margin - CURSORWIDTH) - tw + fd->margin;    
  }

  /* Appear or disappear the cursor depending on focus and cursor
     flashing state */
  DATA->cursor->type = DATA->flash_on ? PG_GROP_RECT : PG_GROP_NOP; 
}

/* If the buffer doesn't have room for one more char, enlarge it */
g_error bufcheck_grow(struct widget *self) {
  g_error e;

  if (DATA->bufuse < DATA->bufsize) return success;

  DATA->bufsize += FIELDBUF_ALLOCSTEP;
  if (DATA->bufsize > DATA->bufmax) DATA->bufsize = DATA->bufmax;

  e = g_realloc((void *)&DATA->buffer,DATA->bufsize);
  errorcheck;

  return rehandle(DATA->hbuffer,DATA->buffer,PG_TYPE_STRING);
}

/* If there's too much wasted space, shrink the buffer */
g_error bufcheck_shrink(struct widget *self) {
  g_error e;

  if ((DATA->bufsize-DATA->bufuse)<FIELDBUF_MAXCRUFT) return success;

  DATA->bufsize -= FIELDBUF_MAXCRUFT;

  e = g_realloc((void *)&DATA->buffer,DATA->bufsize);
  errorcheck;

  return rehandle(DATA->hbuffer,DATA->buffer,PG_TYPE_STRING);
}

void field_resize(struct widget *self) { 
  s16 w,h,m = theme_lookup(self->in->div->state,PGTH_P_MARGIN); 
  struct fontdesc *fd; 
  handle font = DATA->font ? DATA->font :  
    theme_lookup(self->in->div->state, PGTH_P_FONT); 
 
  if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,font)) 
          || !fd) return; 
  sizetext(fd,&w,&h,DATA->buffer); 
  w += m<<1;

  self->in->div->pw = theme_lookup(self->in->div->state,PGTH_P_WIDTH);
  self->in->div->ph = theme_lookup(self->in->div->state,PGTH_P_HEIGHT);

  if (w>self->in->div->pw)
    self->in->div->pw = w;
} 
/* The End */











