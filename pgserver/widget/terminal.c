/* $Id: terminal.c,v 1.3 2000/12/31 23:18:18 micahjd Exp $
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

  /* Text buffer */
  handle hbuffer;
  char *buffer;
  int bufferw,bufferh,buffersize;
  int crsrx,crsry;
  unsigned char attr_under_crsr;
   
  /* The incremental gropnode */
  struct gropnode *inc;
  int x,y;              /* Base coordinates */
  int celw,celh;        /* Character cel size */

  /* Update rectangle (in characters) */
  int updx,updy,updw,updh;

  /* Mouse button down? */
  unsigned int on : 1;
  
  /* Cursor visible? */
  unsigned int cursor_on : 1;

  /* Do we have keyboard focus? */
  unsigned int focus : 1;
};
#define DATA ((struct termdata *)(self->data))

/* Default text attribute */
#define ATTR_DEFAULT 0x07

/* Cursor attribute */
#define ATTR_CURSOR  0xA0

#define FLASHTIME_ON   250
#define FLASHTIME_OFF  150

/**** Internal functions */

/* Create an incremental gropnode for the update rectangle */
void term_realize(struct widget *self);

/* Prepare for adding more update rectangles */
void term_rectprepare(struct widget *self);

/* Output formatted char */
void term_char(struct widget *self,char c);

/* Add update rectangle */
void term_updrect(struct widget *self,int x,int y,int w,int h);

/* Plot a character at an x,y position, add update rect */
void term_plot(struct widget *self,int x,int y,char c);

/* Change attribute at an x,y position, return old attribute */
unsigned char term_chattr(struct widget *self,int x,int y,char c);

/* Hide/show cursor */
void term_setcursor(struct widget *self,int flag);

/* Clear a chunk of buffer */
void term_clearbuf(struct widget *self,int offset,int size);

/********************************************** Widget functions */

void build_terminal(struct gropctxt *c,unsigned short state,struct widget *self) {
  struct fontdesc *fd;
   
  addgrop(c,PG_GROP_TEXTGRID,c->x,c->y,c->w,c->h);
  c->current->param[0] = DATA->hbuffer;
  c->current->param[1] = DATA->font;
  c->current->param[2] = DATA->bufferw;
  c->current->param[3] = 0;

  /* Calculate character cell size */
  if (!iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
		       DATA->font)) && fd) {
     DATA->celw = fd->font->vwtab['W'];
     DATA->celh = fd->font->h;
  }

  /* This gropnode used for incremental updating */
  addgrop(c,PG_GROP_TEXTGRID,0,0,DATA->celw,DATA->celh);
  c->current->flags   |= PG_GROPF_INCREMENTAL;
  c->current->param[0] = DATA->hbuffer;
  c->current->param[1] = DATA->font;
  DATA->inc = c->current;
  DATA->x = c->x;
  DATA->y = c->y;
}

g_error terminal_install(struct widget *self) {
  g_error e;
   
  e = g_malloc(&self->data,sizeof(struct termdata));
  errorcheck;
  memset(self->data,0,sizeof(struct termdata));

  DATA->attr_under_crsr = ATTR_DEFAULT;
   
  DATA->bufferw = 38;
  DATA->bufferh = 18;

  /* Allocate buffer */
  DATA->buffersize = (DATA->bufferw*DATA->bufferh) << 1;
  e = g_malloc((void **) &DATA->buffer,DATA->buffersize+1);
  errorcheck;
  DATA->buffer[DATA->buffersize] = 0;  /* Null past the end */
  e = mkhandle(&DATA->hbuffer,PG_TYPE_STRING,-1,DATA->buffer);
  errorcheck;

  term_clearbuf(self,0,DATA->buffersize);
     
  /* Default terminal font */
  e = findfont(&DATA->font,-1,"Console",0,PG_FSTYLE_FIXED);
  errorcheck;

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  self->in->split = 0;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_terminal;

  self->trigger_mask = TRIGGER_STREAM | TRIGGER_CHAR | TRIGGER_DOWN |
     TRIGGER_UP | TRIGGER_RELEASE | TRIGGER_ACTIVATE | TRIGGER_DEACTIVATE |
     TRIGGER_TIMER;

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
   term_rectprepare(self);   /* Changes must be enclosed
			      * by term_rectprepare and term_realize */
   
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
      
    case TRIGGER_STREAM:
      /* Output each character */
      for (;*param->stream.data;param->stream.data++)
	term_char(self,*param->stream.data);
      
      term_realize(self);  /* Realize rects, but don't do a full update */
      return;
      
    case TRIGGER_CHAR:
      /* A spoonful of translation */
      if (param->kbd.key == PGKEY_RETURN)
	param->kbd.key = '\n';
      
      /* Send the keypress back to the app */
      post_event(PG_WE_ACTIVATE,self,param->kbd.key,0);
      return;
      
    case TRIGGER_DEACTIVATE:
      DATA->focus = 0;
      term_setcursor(self,1);  /* Show cursor */
      break;    /* Update */
      
    case TRIGGER_ACTIVATE:
      DATA->focus = 1;
      /* No break; fall through to TRIGGER_TIMER to set up timer */
      
    case TRIGGER_TIMER:
      if (!DATA->focus) return;
      
      /* Flash the cursor */
      term_setcursor(self,!DATA->cursor_on);
      
      /* Reset timer */
      install_timer(self,(DATA->cursor_on ? FLASHTIME_ON : FLASHTIME_OFF ));
      break;   /* Update */
      
   }
   
   /* If we used a break; above instead of a return we want an update
    * right away for cursor blinking */
   
   term_realize(self);
   update(self->in->div,1);
}

/********************************************** Terminal output functions */

/* Build an incremental gropnode from the update rectangle */
void term_realize(struct widget *self) {
   
   /* Is an update unnecessary? */
   if (!DATA->updw)
     return;
   
   /* Go ahead and set the master redraw flag */
   self->dt->flags |= DIVTREE_NEED_REDRAW;
   
   /* Are we forced to use a full update? (first time) */
   if (!DATA->inc) {
      self->in->div->flags |= DIVNODE_NEED_REDRAW;
      return;
   }
   
   /**** Set up an incremental update for the update rectangle */
   
   /* If this is more than one line, load the buffer width */
   if (DATA->updh > 1)
     DATA->inc->param[2] = DATA->bufferw;
   else
     DATA->inc->param[2] = DATA->updw;
      
   /* Set the buffer offset */
   DATA->inc->param[3] = (DATA->updx + DATA->updy * DATA->bufferw) << 1;
   
/*
   guru("Incremental terminal update:\nx = %d\ny = %d\nw = %d\nh = %d"
	"\nbufferw = %d\nbufferh = %d",
	DATA->updx,DATA->updy,DATA->updw,DATA->updh,
	DATA->bufferw,DATA->bufferh);
*/
 
   /* Gropnode position */
   DATA->inc->x = DATA->x + DATA->updx * DATA->celw;
   DATA->inc->y = DATA->y + DATA->updy * DATA->celh;
   DATA->inc->w = DATA->updw * DATA->celw;
   DATA->inc->h = DATA->updh * DATA->celh;
   
   /* Set the incremental update flag */
   self->in->div->flags |= DIVNODE_INCREMENTAL;
}

/* Add an update rectangle in */
void term_updrect(struct widget *self,int x,int y,int w,int h) {
  if (DATA->updw) {
    if (x < DATA->updx) {
      DATA->updw += DATA->updx - x;
      DATA->updx = x;
    }
    if (y < DATA->updy) {
      DATA->updh += DATA->updy - y;
      DATA->updy = y;
    }
    if ((w+x) > (DATA->updx+DATA->updw))
      DATA->updw = w+x-DATA->updx;
    if ((h+y) > (DATA->updy+DATA->updh))
      DATA->updh = h+y-DATA->updy;
  }
  else {
    DATA->updx = x;
    DATA->updy = y;
    DATA->updw = w;
    DATA->updh = h;
  }
}

/* Plot a character at an x,y position */
void term_plot(struct widget *self,int x,int y,char c) {
   DATA->buffer[((x + y*DATA->bufferw)<<1) + 1] = c;
   term_updrect(self,x,y,1,1);
}

/* Change attribute at an x,y position */
unsigned char term_chattr(struct widget *self,int x,int y,char c) {
   unsigned char old;
   unsigned char *p;
    
   p = DATA->buffer + ((x + y*DATA->bufferw)<<1);
   old = *p;
   *p = c;
   term_updrect(self,x,y,1,1);
   return old;
}

/* Format and/or draw one incoming character. 
 * Handle escape codes if present. */
void term_char(struct widget *self,char c) {
   term_setcursor(self,0);  /* Hide */
   
   switch (c) {
      
    case '\n':
    case '\r':
      DATA->crsrx = 0;
      DATA->crsry++;
      break;
      
    case PGKEY_BACKSPACE:
      if (!DATA->crsrx)  /* Can't backspace past the edge */
	return;
      term_plot(self,--DATA->crsrx,DATA->crsry,' ');
      break;
      
    default:      /* Normal character */
      term_plot(self,DATA->crsrx++,DATA->crsry,c);
      break;
		
   }
   
   /* Handle screen edges */

   if (DATA->crsrx >= DATA->bufferw) {  /* Wrap around the side */
      DATA->crsrx = 0;
      DATA->crsry++;
   }
   
   if (DATA->crsry >= DATA->bufferh) {  /* Scroll vertically */
      DATA->crsry = DATA->bufferh-1;
      memcpy(DATA->buffer,DATA->buffer + (DATA->bufferw<<1),
	      DATA->buffersize-(DATA->bufferw<<1));
      term_clearbuf(self,(DATA->bufferw<<1)*(DATA->bufferh-1),DATA->bufferw<<1);
      term_updrect(self,0,0,DATA->bufferw,DATA->bufferh);
   }
      
   term_setcursor(self,1);  /* Show cursor */
}

/* Hide/show cursor */
void term_setcursor(struct widget *self,int flag) {
   if (flag == DATA->cursor_on)
     return;
   
   if (flag)
     /* Show cursor */
     DATA->attr_under_crsr = term_chattr(self,DATA->crsrx,
					 DATA->crsry,ATTR_CURSOR);
   else
     /* Hide cursor */
     term_chattr(self,DATA->crsrx,DATA->crsry,DATA->attr_under_crsr);
   
   DATA->cursor_on = flag;
}

/* Prepare for adding more update rectangles.
 * This prevents losing updates- if the divnode still has its
 * incremental flag, don't clear the update rectangle */
void term_rectprepare(struct widget *self) {
   if (!(self->in->div->flags & DIVNODE_INCREMENTAL))
     DATA->updw = 0;
}

/* Clear a chunk of buffer */
void term_clearbuf(struct widget *self,int offset,int size) {
   unsigned char *p;
   int i;
   
   /* Clear the buffer: set attribute bytes to ATTR_DEFAULT
    * and character bytes to blanks */
   for (p=DATA->buffer+offset,i=size;i;i-=2) {
      *(p++) = ATTR_DEFAULT;
      *(p++) = ' ';
   }
}

/* The End */
