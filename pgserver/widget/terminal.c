/* $Id: terminal.c,v 1.13 2001/01/13 10:45:45 micahjd Exp $
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

/* Default text attribute */
#define ATTR_DEFAULT 0x07

/* Cursor attribute */
#define ATTR_CURSOR  0xA0

/* On and off times in milliseconds */
#define FLASHTIME_ON   250
#define FLASHTIME_OFF  125

/* Inactivity time before cursor flashes */
#define CURSOR_WAIT    500

/* Size of buffer for VT102 escape codes */
#define ESCAPEBUF_SIZE 32

/* Maximum # of params for a CSI escape code.
 * Same limit imposed by the linux console, should be fine */
#define CSIARGS_SIZE   16

struct termdata {
  /* Time of the last update, used with CURSOR_WAIT */
  unsigned long update_time;

  /* character info */
  handle font;

  /* Text buffer */
  handle hbuffer;
  char *buffer;
  int bufferw,bufferh,buffersize;
  int crsrx,crsry;
  unsigned char attr_under_crsr;
  unsigned char attr;             /* Default attribute for new characters */

  /* Escape code buffer */
  char escapebuf[ESCAPEBUF_SIZE];
  int escbuf_pos;   /* Position in buffer */

  /* Parameter buffer for processed CSI codes */
  int csiargs[CSIARGS_SIZE];
  int num_csiargs;

  /* Background */
  struct gropnode *bg,*bginc;
  handle bitmap;

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

  /* Handling an escape code? */
  unsigned int escapemode : 1;
};
#define DATA ((struct termdata *)(self->data))

/**** Internal functions */

/* Handle a single key event as received from the server */
void kbd_event(struct widget *self, int pgkey,int mods);

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
void term_clearbuf(struct widget *self,int fromx,int fromy,int chars);

/* Handle a parsed ECMA-48 SGR sequence */
void term_ecma48sgr(struct widget *self);

/* Handle a parsed CSI sequence other than ECMA-48, ending
   with the specified character */
void term_othercsi(struct widget *self,char c);

/* Copy a rectangle between two text buffers */
void textblit(char *src,char *dest,int src_x,int src_y,int src_w,
	      int dest_x,int dest_y,int dest_w,int w,int h);

/********************************************** Widget functions */

void build_terminal(struct gropctxt *c,unsigned short state,struct widget *self) {
  struct fontdesc *fd;
  int neww,newh;

  /************** Size calculations */

  /* Calculate character cell size */
  if (iserror(rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,
		       DATA->font)) || !fd)
    return;
  DATA->celw = fd->font->vwtab['W'];
  DATA->celh = fd->font->h;

  /* Using our grop context and character cell size,
   * calculate a good size for us */
  neww = c->w / DATA->celw - 1;   /* A little margin */
  newh = c->h / DATA->celh - 1;

  /* If we're rolled up, don't bother */
  if ((neww>0) && (newh>0)) {

    /* Resize the buffer if necessary */
    if (neww != DATA->bufferw || newh != DATA->bufferh) {
      char *newbuffer,*p;
      long newbuffer_size,i;
      /* Blit parameters */
      int src_y,dest_y,w,h;
      
      /* Allocate */
      newbuffer_size = (neww*newh) << 1;
      if (iserror(g_malloc((void **) &newbuffer,newbuffer_size+1)))
	return;
      newbuffer[newbuffer_size] = 0;  /* Null past the buffer itself */
      
      /* Clear the new buffer */
      for (p=newbuffer,i=newbuffer_size;i;i-=2) {
	*(p++) = DATA->attr;
	*(p++) = ' ';
      }

      /* Cursor off */
      term_setcursor(self,0);
      
      /* Start off assuming the new buffer is smaller, then perform clipping */

      dest_y = 0;
      w = neww;
      h = newh;
      src_y = DATA->bufferh - newh;

      if (src_y<0) {
	h += src_y;
	src_y = 0;
      }
      if (w>DATA->bufferw)
	w = DATA->bufferw;
      if (h>DATA->bufferh) {
	src_y += h - DATA->bufferh;
	h = DATA->bufferh;
      }
      
      /* Move the cursor too */
      DATA->crsry -= src_y;
      if (DATA->crsry < 0) {
	src_y += DATA->crsry;
	DATA->crsry = 0;
      }
 
      /* Blit! */
      textblit(DATA->buffer,newbuffer,0,src_y,
	       DATA->bufferw,0,dest_y,neww,w,h);
      
      /* Free the old buffer and update the handle */
      g_free(DATA->buffer);
      rehandle(DATA->hbuffer,DATA->buffer = newbuffer);
      
      /* Update the rest of our data */
      DATA->bufferw = neww;
      DATA->bufferh = newh;
      DATA->buffersize = newbuffer_size;

      /* Notify the application */
      post_event(PG_WE_RESIZE,self,(neww << 16) | newh,0,NULL);
    }
  }

  /************** Gropnodes */

  /* Background (solid color or bitmap) */
  addgrop(c,DATA->bitmap ? PG_GROP_BITMAP : PG_GROP_RECT,c->x,c->y,c->w,c->h);
  c->current->param[0] = DATA->bitmap;
  c->current->param[1] = PG_LGOP_NONE;
  c->current->param[2] = 0;
  c->current->param[3] = 0;
  DATA->bg = c->current;

  /* Incremental grop for the background */
  addgrop(c,DATA->bitmap ? PG_GROP_BITMAP : PG_GROP_RECT,0,0,0,0);
  c->current->flags   |= PG_GROPF_INCREMENTAL;
  c->current->param[0] = DATA->bitmap;
  c->current->param[1] = PG_LGOP_NONE;
  c->current->param[2] = 0;
  c->current->param[3] = 0;
  DATA->bginc = c->current;

  /* For the margin we figured in earlier */
  c->x += DATA->celw >> 1;
  c->y += DATA->celh >> 1;

  /* Non-incremental text grid */   
  addgrop(c,PG_GROP_TEXTGRID,c->x,c->y,c->w,c->h);
  c->current->param[0] = DATA->hbuffer;
  c->current->param[1] = DATA->font;
  c->current->param[2] = DATA->bufferw;
  c->current->param[3] = 0;

  /* Incremental grop for the text grid */
  addgrop(c,PG_GROP_TEXTGRID,0,0,0,0);
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
  DATA->attr = ATTR_DEFAULT;
   
  DATA->bufferw = 80;
  DATA->bufferh = 25;

  /* Allocate buffer */
  DATA->buffersize = (DATA->bufferw*DATA->bufferh) << 1;
  e = g_malloc((void **) &DATA->buffer,DATA->buffersize+1);
  errorcheck;
  DATA->buffer[DATA->buffersize] = 0;  /* Null past the end */
  term_clearbuf(self,0,0,DATA->bufferw * DATA->bufferh);

  e = mkhandle(&DATA->hbuffer,PG_TYPE_STRING,-1,DATA->buffer);
  errorcheck;
     
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
     TRIGGER_TIMER | TRIGGER_KEYDOWN;

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
  hwrbitmap bit;

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

  case PG_WP_BITMAP:
    if (iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)) || !bit)
      return mkerror(PG_ERRT_HANDLE,4);
    DATA->bitmap = data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  default:
    return mkerror(PG_ERRT_BADPARAM,14);
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

      /* Reset the cursor timer */
      DATA->update_time = getticks();
      return;
      
   case TRIGGER_CHAR:
     /* Normal ASCII-ish character */
     kbd_event(self,param->kbd.key,param->kbd.mods);
     return;
     
   case TRIGGER_KEYDOWN:
     /* Handle control characters that CHAR doesn't map */
     if ((param->kbd.key > 255) || 
	 (param->kbd.mods & (PGMOD_CTRL | PGMOD_ALT)))
       kbd_event(self,param->kbd.key,param->kbd.mods);
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
      
      /* Reset timer */
      install_timer(self,(DATA->cursor_on ? FLASHTIME_ON : FLASHTIME_OFF ));

      /* Flash the cursor if it should be active */
      if (getticks() > (DATA->update_time + CURSOR_WAIT))
	term_setcursor(self,!DATA->cursor_on);
      else {
	/* Make sure the cursor is on */
	if (DATA->cursor_on)
	  return;
	else
	  term_setcursor(self,1);
      }

      break;   /* Update */
      
   }
   
   /* If we used a break; above instead of a return we want an update
    * right away for cursor blinking */
   
   term_realize(self);
   update(self->in->div,1);
}

/********************************************** Keyboard input functions */

/* Handle a single key event as received from the server */
void kbd_event(struct widget *self, int pgkey,int mods) {
  char *rtn;
  static char chrstr[] = " ";      /* Must be static - if the event is put on
				      the queue it might be a while before this
				      is used. */

  /****** Modified key translations */

  if (mods & PGMOD_CTRL)
    pgkey -= PGKEY_a - 1;

  /****** Unmodified Key translation */

  /* Not sure that this is 100% correct, and I know it's not complete... */

  if (pgkey > 255)
    switch (pgkey) {
      
    case PGKEY_RETURN:        rtn = "\n";       break;
      
    case PGKEY_UP:            rtn = "\033[A";   break;
    case PGKEY_DOWN:          rtn = "\033[B";   break;
    case PGKEY_RIGHT:         rtn = "\033[C";   break;
    case PGKEY_LEFT:          rtn = "\033[D";   break;
    case PGKEY_HOME:          rtn = "\033[1~";  break;
    case PGKEY_INSERT:        rtn = "\033[2~";  break;
    case PGKEY_DELETE:        rtn = "\033[3~";  break;
    case PGKEY_END:           rtn = "\033[4~";  break;
    case PGKEY_PAGEUP:        rtn = "\033[5~";  break;
    case PGKEY_PAGEDOWN:      rtn = "\033[6~";  break;
      
    case PGKEY_F1:            rtn = "\033OP";  break;
    case PGKEY_F2:            rtn = "\033OQ";  break;
    case PGKEY_F3:            rtn = "\033OR";  break;
    case PGKEY_F4:            rtn = "\033OS";  break;
      
    case PGKEY_F5:            rtn = "\033[15~";break;
    case PGKEY_F6:            rtn = "\033[17~";break;
    case PGKEY_F7:            rtn = "\033[18~";break;
    case PGKEY_F8:            rtn = "\033[19~";break;
    case PGKEY_F9:            rtn = "\033[20~";break;
    case PGKEY_F10:           rtn = "\033[21~";break;
    case PGKEY_F11:           rtn = "\033[23~";break;
    case PGKEY_F12:           rtn = "\033[24~";break;
      
    default:
      return;
    }
    
  else {
    /**** Send an untranslated key */
    *chrstr = pgkey;
    rtn = chrstr;
  }

  /* After translating a key, send back the string. */
  post_event(PG_WE_DATA,self,strlen(rtn),0,rtn); 
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
 
   /* Gropnode position (background and text) */
   DATA->bginc->x = DATA->inc->x = DATA->x + DATA->updx * DATA->celw;
   DATA->bginc->y = DATA->inc->y = DATA->y + DATA->updy * DATA->celh;
   DATA->bginc->w = DATA->inc->w = DATA->updw * DATA->celw;
   DATA->bginc->h = DATA->inc->h = DATA->updh * DATA->celh;
   DATA->bginc->param[2] = DATA->bginc->x;
   DATA->bginc->param[3] = DATA->bginc->y;

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
   unsigned char *p = DATA->buffer + ((x + y*DATA->bufferw)<<1);

   p[0] = DATA->attr;
   p[1] = c;

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
  
  /* Handling an escape code? */
  if (DATA->escapemode) {

    /* Yer supposed to ignore nulls in escape codes */
    if (!c)
      return;

    /* Too much? */
    if (DATA->escbuf_pos >= ESCAPEBUF_SIZE) {
      DATA->escapemode = 0;
#ifdef BOTHERSOME_TERMINAL
      {
	char *p;

	/* Keep this from messing up the debug terminal! */
	DATA->escapebuf[ESCAPEBUF_SIZE-1] = 0;
	p = DATA->escapebuf;
	while (*p) {
	  if (*p == '\033')
	    *p = '^';
	  p++;
	}

	printf("term: buffer overflowed before escape was recognized\nterm: buffer = \"%s\"\n",
	       DATA->escapebuf);
      }
#endif
      return;
    }

    /* Toss it on */
    DATA->escapebuf[DATA->escbuf_pos++] = c;

    /* Depending on the type of escape code, look for a different termination
     * character and handle the code appropriately
     */

    /* Starts with a CSI? (ESC [) 
     * CSI codes start with CSI, an optional question mark, then optionally list numerical arguments
     * separated by a semicolon. A char other than semicolon or a digit identifies
     * and terminates the code
     */
    if (*DATA->escapebuf == '[' && DATA->escbuf_pos>1) {
      char *p,*endp,*num_end;

      /* Ignore question mark after CSI */
      if (DATA->escbuf_pos==2 && c=='?') {
	DATA->escbuf_pos--;
	return;
      }

      /* If there's more, come back later */
      if (isdigit(c) || c==';')
	return;

      /* We're ending the code */
      DATA->escapemode = 0;

      /* Numericalize the arguments */
      DATA->num_csiargs = 0;
      /* Unspecified params must be zero */
      memset(&DATA->csiargs,0,sizeof(int) * CSIARGS_SIZE);
      p    = DATA->escapebuf+1;                  /* Start after the CSI */
      endp = DATA->escapebuf+DATA->escbuf_pos-1; /* Stop before the end */
      while (p<endp) {
	/* Convert the number */
	DATA->csiargs[DATA->num_csiargs] = strtol(p,&num_end,10);

	/* Got nothing good? */
	if (p==num_end)
	  break;

	/* Increment but check for overflow */
	DATA->num_csiargs++;
	if (DATA->num_csiargs >= CSIARGS_SIZE)
	  break;

	/* Skip to the next one */
	p = num_end+1;
      }

      /* If it ended with 'm', we got an ECMA-48 CSI sequence (colors) */
      if (c=='m')
	term_ecma48sgr(self);

      /* Other CSI sequence (mainly cursor control) */
      else
	term_othercsi(self,c);
    }
  }


  /* Is it a control character? */
  else if (c <= '\033')
    switch (c) {
    case '\033':        /* Escape */
      DATA->escapemode = 1;
      DATA->escbuf_pos = 0;
      return;
      
    case '\a':
      /* Pass on the bell to our terminal */
      write(2,&c,1);
      return;
      
    case '\n':
      DATA->crsrx = 0;
      DATA->crsry++;
      break;
      
    case '\t':
      /* Not sure this is right, but it's consistant with observed behavior */
      DATA->crsrx += 8 - (DATA->crsrx & 7);
      break;
      
    case '\b':
      if (!DATA->crsrx)  /* Can't backspace past the edge */
	return;
      term_plot(self,--DATA->crsrx,DATA->crsry,' ');
      break;
    }

  /* Normal character */
  else
    term_plot(self,DATA->crsrx++,DATA->crsry,c);
  
  /* Handle screen edges */
  if (DATA->crsrx < 0)
    DATA->crsrx = 0;
  else if (DATA->crsrx >= DATA->bufferw) {  /* Wrap around the side */
    DATA->crsrx = 0;
    DATA->crsry++;
  }
  if (DATA->crsry < 0)
    DATA->crsry = 0;
  else if (DATA->crsry >= DATA->bufferh) {  /* Scroll vertically */
    DATA->crsry = DATA->bufferh-1;
    memcpy(DATA->buffer,DATA->buffer + (DATA->bufferw<<1),
	   DATA->buffersize-(DATA->bufferw<<1));
    term_clearbuf(self,0,DATA->bufferh-1,DATA->bufferw);

    /* Two methods here - just redraw the screen or try a scroll blit */

    term_updrect(self,0,0,DATA->bufferw,DATA->bufferh);

    //self->in->div->flags |= DIVNODE_SCROLL_ONLY;
    //self->in->div->oty = DATA->celh;
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
void term_clearbuf(struct widget *self,int fromx,int fromy,int chars) {
   unsigned char *p;
   int i;
   int size = chars<<1;
   long offset = (fromx + fromy * DATA->bufferw)<<1;
   
   /* Clear the buffer: set attribute bytes to ATTR_DEFAULT
    * and character bytes to blanks */
   for (p=DATA->buffer+offset,i=size;i;i-=2) {
      *(p++) = DATA->attr;
      *(p++) = ' ';
   }

   /* Add update rectangle for the effected lines */
   i = (chars+fromx) / DATA->bufferh;
   if ((chars+fromx) % DATA->bufferh)
     i++;
   term_updrect(self,0,fromy,DATA->bufferw,i);
}

/* Handle a parsed ECMA-48 CSI sequence */
void term_ecma48sgr(struct widget *self) {
  int *arg = DATA->csiargs;
  for (;DATA->num_csiargs;DATA->num_csiargs--,arg++)
    switch (*arg) {

      /* 0 - reset to normal */
    case 0:
      DATA->attr = ATTR_DEFAULT;
      break;

      /* 1 - bold */
      /* 4 - underline (we treat it like a bold) */
    case 1:
    case 4:
      DATA->attr |= 0x08;
      break;

      /* 5 - blink (not really, it's like bold for backgrounds) */
    case 5:
      DATA->attr |= 0x80;
      break;

      /* 7 - reverse video on */
      /* 27 - reverse video off */
    case 7:
    case 27:
      DATA->attr = (DATA->attr >> 4) | (DATA->attr << 4);
      break;

      /* 21 or 22 - normal intensity */
      /* 24 - underline off */
    case 21:
    case 22:
    case 24:
      DATA->attr &= 0xF7;
      break;

      /* 25 - blink off */
    case 25:
      DATA->attr &= 0x7F;
      break;
      
      /* 30 through 37 - foreground colors */
    case 30: DATA->attr = (DATA->attr & 0xF8) | 0x00; break;  /* These are kinda funky, */
    case 31: DATA->attr = (DATA->attr & 0xF8) | 0x04; break;  /* because VT100 uses BGR */
    case 32: DATA->attr = (DATA->attr & 0xF8) | 0x02; break;  /* for the colors instead */
    case 33: DATA->attr = (DATA->attr & 0xF8) | 0x06; break;  /* of RGB */
    case 34: DATA->attr = (DATA->attr & 0xF8) | 0x01; break;
    case 35: DATA->attr = (DATA->attr & 0xF8) | 0x05; break;
    case 36: DATA->attr = (DATA->attr & 0xF8) | 0x03; break;
    case 37: DATA->attr = (DATA->attr & 0xF8) | 0x07; break;

      /* 38 - set default foreground, underline on */
    case 38:
      DATA->attr = (DATA->attr & 0xF0) | ((ATTR_DEFAULT & 0x0F) | 0x08);
      break;

      /* 39 - set default foreground, underline off */
    case 39:
      DATA->attr = (DATA->attr & 0xF0) | (ATTR_DEFAULT & 0x0F);
      break;

      /* 40 through 47 - background colors */
    case 40: DATA->attr = (DATA->attr & 0x1F) | 0x00; break;
    case 41: DATA->attr = (DATA->attr & 0x1F) | 0x40; break;
    case 42: DATA->attr = (DATA->attr & 0x1F) | 0x20; break;
    case 43: DATA->attr = (DATA->attr & 0x1F) | 0x60; break;
    case 44: DATA->attr = (DATA->attr & 0x1F) | 0x10; break;
    case 45: DATA->attr = (DATA->attr & 0x1F) | 0x50; break;
    case 46: DATA->attr = (DATA->attr & 0x1F) | 0x30; break;
    case 47: DATA->attr = (DATA->attr & 0x1F) | 0x70; break;

      /* 49 - default background */
    case 49:
      DATA->attr = (DATA->attr & 0x0F) | (ATTR_DEFAULT & 0xF0);
      break;

#ifdef BOTHERSOME_TERMINAL
    default:
      printf("term: Unknown ECMA-48 SGR number = %d\n",*arg);
#endif

    }

}

/* Handle a parsed CSI sequence other than ECMA-48 SGR, ending
   with the specified character */
void term_othercsi(struct widget *self,char c) {
  int i;
  switch (c) {

    /* @ - Insert the indicated # of blank characters */
  case '@':
    for (i=0;i<DATA->csiargs[0];i++)
      term_char(self,' ');
    break;

    /* A - Move cursor up */
  case 'A':
    DATA->crsry -= DATA->csiargs[0];
    break;

    /* B - Move cursor down */
  case 'B':
    DATA->crsry += DATA->csiargs[0];
    break;

    /* C - Move cursor right */
  case 'C':
    DATA->crsrx += DATA->csiargs[0];
    break;

    /* D - Move cursor left */
  case 'D':
    DATA->crsrx -= DATA->csiargs[0];
    break;

    /* E - Move cursor down and to column 1 */
  case 'E':
    DATA->crsry += DATA->csiargs[0];
    DATA->crsrx = 0;
    break;

    /* F - Move cursor up and to column 1 */
  case 'F':
    DATA->crsry -= DATA->csiargs[0];
    DATA->crsrx = 0;
    break;

    /* G - Set column */
  case 'G':
    DATA->crsrx = DATA->csiargs[0] - 1;
    break;

    /* H - Set row,column */
  case 'H':
    DATA->crsry = DATA->csiargs[0] - 1;
    DATA->crsrx = DATA->csiargs[1] - 1;
    break;

    /* J - Erase */
  case 'J':
    switch (DATA->csiargs[0]) {
    case 1:
      term_clearbuf(self,0,0,DATA->crsry * DATA->bufferw + DATA->crsrx);
      break;
    case 2:
      term_clearbuf(self,0,0,DATA->bufferw * DATA->bufferh);
      break;
    default:
      term_clearbuf(self,DATA->crsrx,DATA->crsry,
		    (DATA->bufferw * DATA->bufferh) -
		    (DATA->crsrx + DATA->crsry * DATA->bufferw));
    }
    break;

    /* K - Erase Line */
  case 'K':
    switch (DATA->csiargs[0]) {
    case 1:
      term_clearbuf(self,0,DATA->crsry,DATA->crsrx);
      break;
    case 2:
      term_clearbuf(self,0,DATA->crsry,DATA->bufferw);
      break;
    default:
      term_clearbuf(self,DATA->crsrx,DATA->crsry,DATA->bufferw-DATA->crsrx);
    }
    break;

#ifdef BOTHERSOME_TERMINAL
    default:
      printf("term: Unknown final character in CSI escape = %c\n",c);
#endif


  }
}

/* Copy a rectangle between two text buffers */
void textblit(char *src,char *dest,int src_x,int src_y,int src_w,
	      int dest_x,int dest_y,int dest_w,int w,int h) {
  int srcoffset  = src_w << 1;
  int destoffset = dest_w << 1;

  src  += (src_x + src_y * src_w) << 1;
  dest += (dest_x + dest_y * dest_w) << 1;
  w <<= 1;

  for (;h;h--,src+=srcoffset,dest+=destoffset)
    memcpy(dest,src,w);
}

/* The End */
