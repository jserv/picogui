/* $Id: terminal_frontend.c,v 1.1 2002/09/26 14:11:03 micahjd Exp $
 *
 * terminal.c - a character-cell-oriented display widget for terminal
 *              emulators and things.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/terminal.h>
#define DATA WIDGET_DATA(0,terminaldata)

void load_terminal_theme(struct widget *self);
void build_terminal(struct gropctxt *c,u16 state,struct widget *self);

/********************************************** Widget functions */

g_error terminal_install(struct widget *self) {
  g_error e;

  /* Make divnodes */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_ALL;
  self->in->split = 0;
  self->out = &self->in->next;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->state = PGTH_O_TERMINAL;
  self->in->div->build = &build_terminal;
  self->in->div->flags |= DIVNODE_HOTSPOT;
   
  WIDGET_ALLOC_DATA(0,terminaldata);

  /* Get initial theme info */
  load_terminal_theme(self);

  DATA->attr_under_crsr = DATA->attr_default;
  DATA->current.attr = DATA->attr_default;
   
  DATA->bufferw = 80;
  DATA->bufferh = 25;
  DATA->pref_lines = 25;

  /* Allocate buffer */
  e = pgstring_new(&DATA->buffer, PGSTR_ENCODE_TERM16, 2*DATA->bufferw*DATA->bufferh, NULL);
  errorcheck;
  term_clearbuf(self,0,0,DATA->bufferw * DATA->bufferh);

  e = mkhandle(&DATA->hbuffer,PG_TYPE_PGSTRING,-1,DATA->buffer);
  errorcheck;
     
  /* Default terminal font */
  e = findfont(&DATA->deffont,-1,NULL,0,PG_FSTYLE_FIXED | PG_FSTYLE_DEFAULT);
  errorcheck;
  terminal_set(self,PG_WP_FONT,DATA->deffont);
   
  self->trigger_mask = PG_TRIGGER_STREAM | PG_TRIGGER_CHAR | PG_TRIGGER_DOWN |
     PG_TRIGGER_UP | PG_TRIGGER_RELEASE | PG_TRIGGER_ACTIVATE | PG_TRIGGER_DEACTIVATE |
     PG_TRIGGER_TIMER | PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP;

  return success;
}

void terminal_remove(struct widget *self) {
  handle_free(-1,DATA->hbuffer);   /* Free our system handles */
  handle_free(-1,DATA->deffont);
  g_free(DATA);
  r_divnode_free(self->in);
}

g_error terminal_set(struct widget *self,int property, glob data) {
  struct fontdesc *fd;

  switch (property) {

  case PG_WP_FONT:
    if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,data)) || !fd) 
      return mkerror(PG_ERRT_HANDLE,12);
    DATA->font = (handle) data;
    
    /* Get the character cell size, we need it early 
     * on to calculate preferred size */
    sizetext(fd,&DATA->celw,&DATA->celh,NULL);
    DATA->fontmargin = fd->margin;
    set_widget_rebuild(self);
    resizewidget(self);
    break;

  case PG_WP_LINES:
    if (data>0)
      DATA->pref_lines = data;
    resizewidget(self);
    break;

  case PG_WP_AUTOSCROLL:
    DATA->autoscroll = data;
    break;

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob terminal_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_FONT:
    return (glob) DATA->font;

  case PG_WP_LINES:
    return DATA->bufferh;
    
  case PG_WP_AUTOSCROLL:
    return DATA->autoscroll;
    
  default:
    return 0;
  }
}

/* Preferred size, 80*25 characters */
void terminal_resize(struct widget *self) {
  self->in->div->pw = DATA->celw * 80;
  self->in->div->ph = DATA->celh * DATA->pref_lines;

  /* Remember, we must account for the terminal's margin */
  if (DATA->fontmargin) {
    self->in->div->pw += DATA->celw;
    self->in->div->ph += DATA->celh;
  }

  load_terminal_theme(self);
}
   
void build_terminal(struct gropctxt *c,u16 state,struct widget *self) {
  int neww,newh;
  struct gropnode *grid;
  pghandle bitmap = theme_lookup(self->in->div->state,PGTH_P_BITMAP1);
  pghandle htextcolors = theme_lookup(self->in->div->state,PGTH_P_TEXTCOLORS);
  u32 *textcolors;
  g_error e;

  /* Get a pointer to the text palette */
  e = rdhandle((void**)&textcolors,PG_TYPE_PALETTE,-1,htextcolors);
  if (iserror(e)) {
    /* Maybe it's not a palette yet? */
    if (e==mkerror(PG_ERRT_HANDLE,28)) {   /* Wrong type? */
      e = array_palettize(htextcolors,-1);
      if (iserror(e))
	return;
      e = rdhandle((void**)&textcolors,PG_TYPE_PALETTE,-1,htextcolors);
    }
    if (iserror(e) || textcolors[0]<16) {
      /* Try the default textcolors */
      e = rdhandle((void**)&textcolors,PG_TYPE_PALETTE,-1,res[PGRES_DEFAULT_TEXTCOLORS]);
      if (iserror(e) || textcolors[0]<16)
	return;
    }
  }
  textcolors++;

  /************** Size calculations */

  /* Using our grop context and character cell size,
   * calculate a good size for us */
  neww = c->w / DATA->celw - (DATA->fontmargin ? 1 : 0);   /* A little margin */
  newh = c->h / DATA->celh - (DATA->fontmargin ? 1 : 0);

  /* If we're rolled up, don't bother */
  if ((neww>0) && (newh>0)) {

    /* Resize the buffer if necessary */
    if (neww != DATA->bufferw || newh != DATA->bufferh) {
      struct pgstring *newbuffer;
      struct pgstr_iterator p = PGSTR_I_NULL;
      s32 newbuffer_size,i;
      /* Blit parameters */
      int src_y,dest_y,w,h;
      
      /* Allocate */
      newbuffer_size = neww * newh * pgstring_encoded_length(DATA->buffer,' ');
      if (iserror(pgstring_new(&newbuffer,DATA->buffer->flags,newbuffer_size,NULL)))
	return;
      
      /* Clear the new buffer */
      do {
	pgstring_encode_meta(newbuffer, &p, ' ', (void*)(u32) DATA->current.attr);
      } while (!p.invalid);

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
      DATA->current.crsry -= src_y;
      if (DATA->current.crsry < 0) {
	src_y += DATA->current.crsry;
	DATA->current.crsry = 0;
      }
 
      /* Blit! */
      textblit(DATA->buffer,newbuffer,0,src_y,
	       DATA->bufferw,0,dest_y,neww,w,h);
      
      /* Free the old buffer and update the handle */
      pgstring_delete(DATA->buffer);
      rehandle(DATA->hbuffer,DATA->buffer = newbuffer,PG_TYPE_PGSTRING);
      
      /* Update the rest of our data */
      DATA->bufferw = neww;
      DATA->bufferh = newh;
    }
  }

  /************** Gropnodes */

  /* Set font */
  addgrop(c,PG_GROP_SETFONT);
  c->current->param[0] = DATA->font;
  c->current->flags   |= PG_GROPF_UNIVERSAL;
   
  /* Set source position */
  addgrop(c,PG_GROP_SETSRC);
  c->current->flags |= PG_GROPF_UNIVERSAL;
  DATA->bgsrc = c->current;

  /* Background (solid color or bitmap) */
  addgropsz(c,bitmap ? PG_GROP_BITMAP : PG_GROP_RECT,c->x,c->y,c->w,c->h);
  c->current->flags   |= PG_GROPF_COLORED;
  c->current->param[0] = bitmap ? bitmap : textcolors[0];
  DATA->bg = c->current;

  /* Incremental grop for the background */
  addgropsz(c,bitmap ? PG_GROP_BITMAP : PG_GROP_RECT,0,0,0,0);
  c->current->flags   |= PG_GROPF_INCREMENTAL | PG_GROPF_COLORED;
  c->current->param[0] = DATA->bg->param[0];
  DATA->bginc = c->current;

  /* For the margin we figured in earlier */
  if (DATA->fontmargin) {
     c->x += DATA->celw >> 1;
     c->y += DATA->celh >> 1;
  }
   
  /* Non-incremental text grid */   
  addgropsz(c,PG_GROP_TEXTGRID,c->x,c->y,c->w,c->h);
  c->current->param[0] = DATA->hbuffer;
  c->current->param[1] = DATA->bufferw << 16;
  c->current->param[2] = htextcolors;
  grid = c->current;

  /* Incremental grop for the text grid */
  addgrop(c,PG_GROP_TEXTGRID);
  c->current->flags   |= PG_GROPF_INCREMENTAL;
  c->current->param[0] = DATA->hbuffer;
  c->current->param[2] = grid->param[2];
  DATA->inc = c->current;
  DATA->x = c->x;
  DATA->y = c->y;

  /* Notify the application */
  post_event(PG_WE_RESIZE,self,(neww << 16) | newh,0,NULL);
}

void terminal_trigger(struct widget *self,s32 type,union trigparam *param) {
  term_rectprepare(self);   /* Changes must be enclosed
			     * by term_rectprepare and term_realize */
  
  switch (type) {
    
    /* When clicked, request keyboard focus */
    
  case PG_TRIGGER_DOWN:
    if (param->mouse.chbtn==1)
      DATA->on=1;
    return;
    
  case PG_TRIGGER_UP:
    if (DATA->on && param->mouse.chbtn==1) {
      DATA->on=0;
      request_focus(self);
    }
    return;
      
  case PG_TRIGGER_RELEASE:
    if (param->mouse.chbtn==1)
      DATA->on=0;
    return;
    
  case PG_TRIGGER_STREAM:
    /* Output each character */
    for (;*param->stream.data;param->stream.data++)
      term_char(self,*param->stream.data);
    
    /* If we're autoscrolling, make sure the new cursor position is scrolled in */
    if (DATA->autoscroll) {
      /* More trickery... we'd like to be able to use scroll_to_divnode() on this,
       * but unfortunately the cursor isn't a divnode. It isn't even a gropnode,
       * just a couple bytes set in the terminal's textgrid. So, here we manufacture
       * a termporary divnode to feed to scroll_to_divnode.
       */
      
      struct divnode fakediv = *self->in->div;
      fakediv.x += DATA->x + DATA->celw * DATA->current.crsrx;
      fakediv.y += DATA->y + DATA->celh * DATA->current.crsry;
      fakediv.w  = DATA->celw;
      fakediv.h  = DATA->celh;
      
	scroll_to_divnode(&fakediv);
    }
    
    term_realize(self);  /* Realize rects, but don't do a full update */
    
    /* Reset the cursor timer */
    DATA->update_time = getticks();
    return;
    
  case PG_TRIGGER_CHAR:
    if (!(param->kbd.flags & PG_KF_FOCUSED))
      return;
    param->kbd.consume++;
    
    /* Normal ASCII-ish character */
    kbd_event(self,param->kbd.key,param->kbd.mods);
    /* Terminal always consumes normal characters */
    return;
     
  case PG_TRIGGER_KEYDOWN:
    if (!(param->kbd.flags & PG_KF_FOCUSED))
      return;
    param->kbd.consume++;
    
    /* Handle control characters that CHAR doesn't map */
    if ((param->kbd.key > 255) || 
	(param->kbd.mods & (PGMOD_CTRL | PGMOD_ALT))) {
      kbd_event(self,param->kbd.key,param->kbd.mods);
    }
    return;
    
  case PG_TRIGGER_KEYUP:
    if (!(param->kbd.flags & PG_KF_FOCUSED))
      return;
    param->kbd.consume++;
    return;
    
  case PG_TRIGGER_DEACTIVATE:
    DATA->focus = 0;
    term_setcursor(self,1);  /* Show cursor */
    break;    /* Update */
    
  case PG_TRIGGER_ACTIVATE:
    DATA->focus = 1;
    /* No break; fall through to PG_TRIGGER_TIMER to set up timer */
    
  case PG_TRIGGER_TIMER:
    if (!DATA->focus) return;
    
    /* Reset timer */
    install_timer(self,(DATA->cursor_on ? 
			DATA->flashtime_off : DATA->flashtime_on ));
    
    /* Flash the cursor if it should be active */
    if (getticks() > (DATA->update_time + DATA->cursor_wait))
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

/********************************************** Internal utilitites */

void load_terminal_theme(struct widget *self) {
  handle f;

  /* Load theme parameters */
  DATA->attr_default = theme_lookup(self->in->div->state,PGTH_P_ATTR_DEFAULT);
  DATA->attr_cursor = theme_lookup(self->in->div->state,PGTH_P_ATTR_CURSOR);
  DATA->flashtime_on = theme_lookup(self->in->div->state,PGTH_P_TIME_ON);
  DATA->flashtime_off = theme_lookup(self->in->div->state,PGTH_P_TIME_OFF);
  DATA->cursor_wait = theme_lookup(self->in->div->state,PGTH_P_TIME_DELAY);
  
  /* Allow setting the font in the theme or with PG_WP_FONT, but default
   * to our fixed-width font instead of the global res[PGRES_DEFAULT_FONT]
   */
  f = theme_lookup(self->in->div->state,PGTH_P_FONT);
  if (DATA->font == DATA->deffont && f != res[PGRES_DEFAULT_FONT])
    terminal_set(self,PG_WP_FONT,f);
}

/* The End */
