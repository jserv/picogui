/* $Id$
 *
 * terminal.c - a character-cell-oriented display widget for terminal
 *              emulators and things.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/terminal.h>
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(terminaldata)

void load_terminal_theme(struct widget *self);
void build_terminal(struct gropctxt *c,u16 state,struct widget *self);

/********************************************** Widget functions */

g_error terminal_install(struct widget *self) {
  g_error e;
  struct font_style fs;
  struct font_descriptor *fd;

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
   
  WIDGET_ALLOC_DATA(terminaldata);

  /* Get initial theme info */
  load_terminal_theme(self);

  DATA->attr_under_crsr = DATA->attr_default;

  DATA->bufferw = 80;
  DATA->bufferh = 24;
  DATA->pref_lines = 24;

  /* Allocate buffer */
  e = pgstring_new(&DATA->buffer, PGSTR_ENCODE_TERM16, 2*DATA->bufferw*DATA->bufferh, NULL);
  errorcheck;
  term_clearbuf(self,0,0,DATA->bufferw * DATA->bufferh);

  e = mkhandle(&DATA->hbuffer,PG_TYPE_PGSTRING,-1,DATA->buffer);
  errorcheck;
     
  /* Default terminal font */
  memset(&fs,0,sizeof(fs));
  fs.style = PG_FSTYLE_FIXED | PG_FSTYLE_DEFAULT;
  e = font_descriptor_create(&fd,&fs);
  errorcheck;
  e = mkhandle(&DATA->deffont,PG_TYPE_FONTDESC,-1,fd);
  errorcheck;
  terminal_set(self,PG_WP_FONT,DATA->deffont);
   
  self->trigger_mask = PG_TRIGGER_STREAM | PG_TRIGGER_CHAR | PG_TRIGGER_DOWN |
    PG_TRIGGER_UP | PG_TRIGGER_RELEASE | PG_TRIGGER_ACTIVATE | PG_TRIGGER_DEACTIVATE |
    PG_TRIGGER_TIMER | PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP | PG_TRIGGER_SCROLLWHEEL |
    PG_TRIGGER_MOVE;

  term_reset(self);

  return success;
}

void terminal_remove(struct widget *self) {
  handle_free(-1,DATA->hbuffer);   /* Free our system handles */
  handle_free(-1,DATA->deffont);
  g_free(DATA);
  r_divnode_free(self->in);
}

g_error terminal_set(struct widget *self,int property, glob data) {
  struct font_descriptor *fd;
  struct font_metrics m;

  switch (property) {

  case PG_WP_FONT:
    if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,data)) || !fd) 
      return mkerror(PG_ERRT_HANDLE,12);
    DATA->font = (handle) data;
    
    /* Get the character cell size, we need it early 
     * on to calculate preferred size */
    fd->lib->getmetrics(fd,&m);
    DATA->fontmargin = m.margin;
    DATA->celw = m.charcell.w;
    DATA->celh = m.charcell.h;
    
    /* Protection against SIGFPE if our font engine is hosed */
    if (DATA->celw < 1) DATA->celw = 1;
    if (DATA->celh < 1) DATA->celh = 1;

    set_widget_rebuild(self);
    resizewidget(self);
    break;

  case PG_WP_LINES:
    /* FIXME: Use s32 instead of s16 in the rendering engine so this overflow isn't a problem */
    if (data > 32000/DATA->celh)
      data = 32000/DATA->celh;
    
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
    return widget_base_get(self,property);
  }
}

/* Preferred size, 80*24 characters by default */
void terminal_resize(struct widget *self) {
  self->in->div->preferred.w = DATA->celw * 80;
  self->in->div->preferred.h = DATA->celh * DATA->pref_lines;

  /* Remember, we must account for the terminal's margin */
  if (DATA->fontmargin) {
    self->in->div->preferred.w += DATA->celw;
    self->in->div->preferred.h += DATA->celh;
  }

  load_terminal_theme(self);
}
   
void build_terminal(struct gropctxt *c,u16 state,struct widget *self) {
  int neww,newh;
  pghandle bitmap = theme_lookup(self->in->div->state,PGTH_P_BITMAP1);
  u32 *textcolors;
  g_error e;
  pghandle htextcolors = DATA->htextcolors ? 
    DATA->htextcolors : theme_lookup(self->in->div->state,PGTH_P_TEXTCOLORS);

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
  neww = c->r.w / DATA->celw - (DATA->fontmargin ? 1 : 0);   /* A little margin */
  newh = c->r.h / DATA->celh - (DATA->fontmargin ? 1 : 0);

  if (neww < 0) neww = 0;
  if (newh < 0) newh = 0;

  /* If we're rolled up, don't bother */
  if ((neww>0) && (newh>0)) {

    /* Resize the buffer if necessary */
    if (neww != DATA->bufferw || newh != DATA->bufferh) {
      struct pgstring *newbuffer;
      struct pgstr_iterator p;
      s32 newbuffer_size,i;
      /* Blit parameters */
      int src_y,dest_y,w,h;

      /* Allocate */
      newbuffer_size = neww * newh * pgstring_encoded_length(DATA->buffer,' ');
      if (iserror(pgstring_new(&newbuffer,DATA->buffer->flags,newbuffer_size,NULL)))
	return;
      
      /* Clear the new buffer */
      pgstring_seek(newbuffer, &p, 0, PGSEEK_SET);
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
 
      /* Move the bottom of the old buffer to the bottom of the new buffer */
      textblit(DATA->buffer,newbuffer,
	       0, src_y, DATA->bufferw,
	       0, dest_y, neww,
	       w,h);
      
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
  addgropsz(c,bitmap ? PG_GROP_BITMAP : PG_GROP_RECT,c->r.x,c->r.y,c->r.w,c->r.h);
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
     c->r.x += DATA->celw >> 1;
     c->r.y += DATA->celh >> 1;
  }
   
  /* Non-incremental text grid */   
  addgropsz(c,PG_GROP_TEXTGRID,c->r.x,c->r.y,c->r.w,c->r.h);
  c->current->param[0] = DATA->hbuffer;
  c->current->param[1] = DATA->bufferw << 16;
  c->current->param[2] = htextcolors;
  DATA->grid = c->current;

  /* Incremental grop for the text grid */
  addgrop(c,PG_GROP_TEXTGRID);
  c->current->flags   |= PG_GROPF_INCREMENTAL;
  c->current->param[0] = DATA->hbuffer;
  c->current->param[2] = DATA->grid->param[2];
  DATA->inc = c->current;
  DATA->x = c->r.x;
  DATA->y = c->r.y;

  /* Default scrolling regions */
  DATA->current.scroll_top = 0;
  DATA->current.scroll_bottom = newh-1;

  /* Notify the application */
  post_event(PG_WE_RESIZE,self,(neww << 16) | newh,0,NULL);
}

void terminal_trigger(struct widget *self,s32 type,union trigparam *param) {

  term_rectprepare(self);   /* Changes must be enclosed
			     * by term_rectprepare and term_realize */
  
  if (type & (PG_TRIGGER_UP | PG_TRIGGER_DOWN | PG_TRIGGER_MOVE)) {
    /* Get the x,y of the mouse, for mouse events */
    DATA->mouse_x = (param->mouse.x - self->in->div->r.x - DATA->grid->r.x) / DATA->celw;
    DATA->mouse_y = (param->mouse.y - self->in->div->r.y - DATA->grid->r.y) / DATA->celh;
    if (DATA->mouse_x<0) DATA->mouse_x = 0;
    if (DATA->mouse_y<0) DATA->mouse_y = 0;
    if (DATA->mouse_x>=DATA->bufferw) DATA->mouse_x = DATA->bufferw - 1;
    if (DATA->mouse_y>=DATA->bufferh) DATA->mouse_y = DATA->bufferh - 1;
    DATA->mouse_x++;
    DATA->mouse_y++;
  }

  switch (type) {
    
    /* When clicked, request keyboard focus */
    
  case PG_TRIGGER_DOWN:
    if (param->mouse.chbtn==1) {
      DATA->on=1;
      term_mouse_event(self, 1, 0);
    }
    else if (param->mouse.chbtn==2)
      term_mouse_event(self, 1, 1);
    else if (param->mouse.chbtn==4)
      term_mouse_event(self, 1, 2);
    return;
    
  case PG_TRIGGER_UP:
    if (DATA->on && param->mouse.chbtn==1) {
      DATA->on=0;
      request_focus(self);
      term_mouse_event(self, 0, 0);
    }
    else if (param->mouse.chbtn==2)
      term_mouse_event(self, 0, 1);
    else if (param->mouse.chbtn==4)
      term_mouse_event(self, 0, 2);
    return;

  case PG_TRIGGER_SCROLLWHEEL:
    if (param->mouse.y > 0) {
      term_mouse_event(self, 1, 3);
      term_mouse_event(self, 0, 3);
    }
    else {
      term_mouse_event(self, 1, 4);
      term_mouse_event(self, 0, 4);
    }
    return;
      
  case PG_TRIGGER_RELEASE:
    if (param->mouse.chbtn==1)
      DATA->on=0;
    return;
    
  case PG_TRIGGER_STREAM:
    /* Output each character
     * FIXME: this doesn't do unicode
     */
    {
      u32 bytes = param->stream.size;
      for (;bytes;param->stream.data++, bytes--)
	term_char(self,*param->stream.data);
    }    

    /* If we're autoscrolling, make sure the new cursor position is scrolled in */
    if (DATA->autoscroll) {
      /* More trickery... we'd like to be able to use scroll_to_divnode() on this,
       * but unfortunately the cursor isn't a divnode. It isn't even a gropnode,
       * just a couple bytes set in the terminal's textgrid. So, here we manufacture
       * a termporary divnode to feed to scroll_to_divnode.
       */
      
      struct divnode fakediv = *self->in->div;
      fakediv.r.x += DATA->x + DATA->celw * DATA->current.crsrx;
      fakediv.r.y += DATA->y + DATA->celh * DATA->current.crsry;
      fakediv.r.w  = DATA->celw;
      fakediv.r.h  = DATA->celh;
      
      scroll_to_divnode(&fakediv);
    }
    
    term_realize(self);  /* Realize rects, but don't do a full update */
    
    /* Reset the cursor timer */
    DATA->update_time = os_getticks();
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
    DATA->key_mods = param->kbd.mods;
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
    DATA->key_mods = param->kbd.mods;
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
    if (os_getticks() > (DATA->update_time + DATA->cursor_wait))
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
