/* $Id: button.c,v 1.104 2002/05/20 19:18:38 micahjd Exp $
 *
 * button.c - generic button, with a string or a bitmap
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

#include <string.h>
#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/appmgr.h>

struct btndata {
  unsigned int on : 1;
  unsigned int over : 1;
  unsigned int toggle : 1;
  unsigned int disabled : 1;
  
  /* These keep track of when one of the parameters was customized by
   * a theme (as opposed to the app) so it can be appropriately unloaded */
  unsigned int theme_bitmap : 1;
  unsigned int theme_bitmask : 1;
  unsigned int theme_text : 1;

  /* Normally the side is specified manually, but if it isn't,
   * get it from the theme.
   */
  unsigned int theme_side : 1;

  /* Use the PG_LGOP_ALPHA operation to draw the bitmap */
  unsigned int has_alpha : 1;

  /* This flag is set when our hotkey is received */
  unsigned int hotkey_received : 1;
  
  handle bitmap,bitmask,text,font;
  
  /* Values set by PG_WP_THOBJ_BUTTON_* */
  int state,state_on,state_hilight,state_on_nohilight,bitmap_side;

  int hotkey;
  int hotkey_flags;
  int hotkey_consume;

  /* Mask of extended (other than ACTIVATE) events to send
   * and other flags*/
  int extdevents;
};
#define DATA ((struct btndata *)(self->data))

struct btnposition {
  /* Coordinates calculated in position_button */
  s16 x,y,w,h;  /* Coordinates of bitmap and text combined */
  s16 bx,by,bw,bh;  /* Bitmap, relative to bitmap+text */
  s16 tx,ty,tw,th;  /* Text, relative to bitmap+text */ 

  /* position_button looks this up anyway */
  handle font;
};

/* Code to generate the button coordinates, needed to resize or build the button */
void position_button(struct widget *self,struct btnposition *bp);

/* Determine the current state of the button and draw it */
void button_setstate(struct widget *self) {
  int state;

  if (DATA->on && DATA->over)
    state = DATA->state_on;
  else if (DATA->on)
    state = DATA->state_on_nohilight;
  else if (DATA->over)
    state = DATA->state_hilight;
  else
    state = DATA->state;

  /* If the button is already visible, use div_setstate for a proper redraw.
   * Otherwise optimize for the common case of the button state being changed
   * during initialization by the app.
   */
  if (!(self->in->div->w && self->in->div->h))
    self->in->div->state = state;
  else
    div_setstate(self->in->div,state,0);
}

void build_button(struct gropctxt *c,unsigned short state,struct widget *self) {
  struct btnposition bp;
  int sp = theme_lookup(DATA->state,PGTH_P_SPACING);

  /* Shave off the space between buttons */
  switch (self->in->flags & (~SIDEMASK)) {
   case PG_S_BOTTOM: c->y += sp; c->h -= sp; break;
   case PG_S_RIGHT:  c->x += sp; c->w -= sp; break;
   case PG_S_LEFT:   c->w -= sp; break;
   case PG_S_TOP:    c->h -= sp; break;
  }
   
  /* Background */
  exec_fillstyle(c,state,PGTH_P_BGFILL);

  position_button(self,&bp);

  /* Align the whole thing */
  align(c,theme_lookup(state,PGTH_P_ALIGN),&bp.w,&bp.h,&bp.x,&bp.y);

  /* AND the mask, then OR the bitmap. Yay for transparency effects! */
  if (DATA->bitmask) {
    addgrop(c,PG_GROP_SETLGOP);
    c->current->param[0] = PG_LGOP_AND;
    addgropsz(c,PG_GROP_BITMAP,bp.x+bp.bx,bp.y+bp.by,bp.bw,bp.bh);
    c->current->param[0] = DATA->bitmask;
  }
  if (DATA->bitmap) {
    if (DATA->bitmask) {
       addgrop(c,PG_GROP_SETLGOP);
       c->current->param[0] = PG_LGOP_OR;
    }

    /* Automatically use alpha blending if necessary */
    if (DATA->has_alpha) {
      addgrop(c,PG_GROP_SETLGOP);
      c->current->param[0] = PG_LGOP_ALPHA;
    }

    addgropsz(c,PG_GROP_BITMAP,bp.x+bp.bx,bp.y+bp.by,bp.bw,bp.bh);
    c->current->param[0] = DATA->bitmap;
    if (DATA->bitmask) {
       addgrop(c,PG_GROP_SETLGOP);
       c->current->param[0] = PG_LGOP_NONE;
    }
  }

  /* FIXME: Allow theme to control how buttons are disabled */
  if (DATA->disabled) {
    addgrop(c,PG_GROP_SETLGOP);
    c->current->param[0] = PG_LGOP_STIPPLE;
  }

  /* Text */
  if (DATA->text) {
    addgrop(c,PG_GROP_SETCOLOR);
    c->current->param[0] = VID(color_pgtohwr) 
       (theme_lookup(state,PGTH_P_FGCOLOR));
    if (bp.font != defaultfont) {
       addgrop(c,PG_GROP_SETFONT);
       c->current->param[0] = bp.font;
    }
    addgropsz(c,PG_GROP_TEXT,bp.x+bp.tx,bp.y+bp.ty,bp.tw,bp.th);
    c->current->param[0] = DATA->text;
  }
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error button_install(struct widget *self) {
  g_error e;

  e = g_malloc(&self->data,sizeof(struct btndata));
  errorcheck;
  memset(self->data,0,sizeof(struct btndata));

  DATA->theme_side = 1;
  DATA->bitmap_side = -1;
  DATA->hotkey_flags = PG_KF_ALWAYS;
  DATA->hotkey_consume = 1;

  /* Default states */
  DATA->state = PGTH_O_BUTTON;
  DATA->state_on = PGTH_O_BUTTON_ON;
  DATA->state_on_nohilight = PGTH_O_BUTTON_ON;
  DATA->state_hilight = PGTH_O_BUTTON_HILIGHT;

  /* Main split */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_LEFT;

  /* Visible node */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->build = &build_button;
  self->in->div->state = DATA->state;
  self->in->div->flags |= DIVNODE_HOTSPOT | DIVNODE_SPLIT_BORDER;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | TRIGGER_CHAR |
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_DIRECT |
    TRIGGER_KEYUP | TRIGGER_KEYDOWN | TRIGGER_DEACTIVATE | TRIGGER_ACTIVATE |
    TRIGGER_KEY_START;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;

  return success;
}

void button_remove(struct widget *self) {
  g_free(self->data);
  r_divnode_free(self->in);
}

g_error button_set(struct widget *self,int property, glob data) {
  hwrbitmap bit;
  char *str;
  struct fontdesc *fd;

  switch (property) {

  case PG_WP_BITMAP:
    if (iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)))
       return mkerror(PG_ERRT_HANDLE,33);
     
    DATA->bitmap = (handle) data;
    resizewidget(self);
    set_widget_rebuild(self);
    break;

  case PG_WP_BITMASK:
    if (iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)))
       return mkerror(PG_ERRT_HANDLE,34);

    DATA->bitmask = (handle) data;
    set_widget_rebuild(self);
    break;

  case PG_WP_BITMAPSIDE:
    DATA->bitmap_side = (int) data;
    resizewidget(self);
    set_widget_rebuild(self);
    break;

  case PG_WP_FONT:
    if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,self->owner,data))) 
	 return mkerror(PG_ERRT_HANDLE,35);
    DATA->font = (handle) data;
    resizewidget(self);
    set_widget_rebuild(self);
    break;

  case PG_WP_TEXT:
    if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,self->owner,data))) 
       return mkerror(PG_ERRT_HANDLE,13);
    DATA->text = (handle) data;
    resizewidget(self);
    set_widget_rebuild(self);
    break;

  case PG_WP_EXTDEVENTS:
    DATA->extdevents = data;

    /* Go ahead and set the hotspot flag */
    if (data & PG_EXEV_NO_HOTSPOT)
      self->in->div->flags &= ~DIVNODE_HOTSPOT;
    else
      self->in->div->flags |= DIVNODE_HOTSPOT;
    break;

  case PG_WP_ON:
    DATA->on = DATA->toggle = data;
    /* Fake a trigger to redraw the button */
    button_trigger(self,0,NULL);
    break;

  case PG_WP_DISABLED:
    DATA->disabled = data;
    set_widget_rebuild(self);
    break;

  case PG_WP_THOBJ_BUTTON:
    DATA->state = data;
    button_setstate(self);
    button_resize(self);
    break;
     
  case PG_WP_THOBJ_BUTTON_HILIGHT:
    DATA->state_hilight = data;
    button_setstate(self);
    button_resize(self);
    break;
     
  case PG_WP_THOBJ_BUTTON_ON:
    DATA->state_on = data;
    button_setstate(self);
    button_resize(self);
    break;
     
  case PG_WP_THOBJ_BUTTON_ON_NOHILIGHT:
    DATA->state_on_nohilight = data;
    button_setstate(self);
    button_resize(self);
    break;

  case PG_WP_SIDE:
    /* Remember that the side was set manually, then pass it on */
    DATA->theme_side = 0;
    return mkerror(ERRT_PASS,0);

  case PG_WP_HOTKEY_FLAGS:
    DATA->hotkey_flags = data;
    break;

  case PG_WP_HOTKEY_CONSUME:
    DATA->hotkey_consume = data;
    break;

  case PG_WP_HOTKEY:
    /* Process PGTH_P_HIDEHOTKEYS if it is set */
    switch (theme_lookup(widget_get(self,PG_WP_STATE),PGTH_P_HIDEHOTKEYS)) {
      
    case PG_HHK_RETURN_ESCAPE:
      if (data == PGKEY_RETURN || data == PGKEY_ESCAPE)
	widget_set(self,PG_WP_SIZE,0);
      break;
      
    }
    DATA->hotkey = data;
    break;

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob button_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_BITMAP:
    return (glob) DATA->bitmap;

  case PG_WP_BITMASK:
    return (glob) DATA->bitmask;

  case PG_WP_BITMAPSIDE:
    return (glob) DATA->bitmap_side;
    
  case PG_WP_EXTDEVENTS:
    return (glob) DATA->extdevents;

  case PG_WP_FONT:
    return (glob) DATA->font;

  case PG_WP_TEXT:
    return (glob) DATA->text;

  case PG_WP_ON:
    return (glob) DATA->on;

  case PG_WP_DISABLED:
    return (glob) DATA->disabled;

  case PG_WP_THOBJ_BUTTON:
    return (glob) DATA->state;

  case PG_WP_THOBJ_BUTTON_HILIGHT:
    return (glob) DATA->state_hilight;

  case PG_WP_THOBJ_BUTTON_ON:
    return (glob) DATA->state_on;

  case PG_WP_THOBJ_BUTTON_ON_NOHILIGHT:
    return (glob) DATA->state_on_nohilight;

  case PG_WP_HOTKEY:
    return (glob) DATA->hotkey;

  case PG_WP_HOTKEY_FLAGS:
    return (glob) DATA->hotkey_flags;

  case PG_WP_HOTKEY_CONSUME:
    return (glob) DATA->hotkey_consume;

  default:
    return 0;
  }
}

void button_trigger(struct widget *self,s32 type,union trigparam *param) {
  int event=-1;
  union trigparam tp;

  /* If it's disabled, don't allow anything except
   * hilighting and global keys
   */
  if (DATA->disabled && type!=TRIGGER_ENTER && type!=TRIGGER_LEAVE &&
      !(type==TRIGGER_KEYDOWN && param->kbd.key!=hotkey_activate))
    return;

  /* Figure out the button's new state */
  switch (type) {

  case TRIGGER_ENTER:
    DATA->over=1;
    break;
    
  case TRIGGER_LEAVE:
    DATA->over=0;
    break;
   
  case TRIGGER_CHAR:
    if (param->kbd.key == hotkey_activate && (param->kbd.flags & PG_KF_FOCUSED))
      param->kbd.consume++;
    if (DATA->hotkey_consume && param->kbd.key == DATA->hotkey && (param->kbd.flags & DATA->hotkey_flags))
      param->kbd.consume++;
    return;
    
  case TRIGGER_KEYDOWN:
    /* We want to consume the hotkey's KEYDOWN, but only act on KEYUP.
     */
    if (DATA->hotkey_consume && param->kbd.key == DATA->hotkey && (param->kbd.flags & DATA->hotkey_flags)) {
      param->kbd.consume++;      
      return;
    }
    
    /* Do the fake mouseclick if it's focused and they pressed the activate key
     */
    if (param->kbd.key == hotkey_activate && (param->kbd.flags & PG_KF_FOCUSED))
      param->kbd.consume++;
    else
      return;

    /* Fake a mouse click so popups stick to buttons
     * when triggered by keyboard
     */
    tp = *param;
    param = &tp;
    param->mouse.chbtn = 1;
    lastclicked = self;
  case TRIGGER_DOWN:
    if (DATA->extdevents & PG_EXEV_PNTR_DOWN)
      post_event(PG_WE_PNTR_DOWN,self,param->mouse.chbtn,0,NULL);
    if (param->mouse.chbtn==1 && 
	!(DATA->extdevents & PG_EXEV_NOCLICK)) {
      /* If this is a toggle button, and we're toggling it on, send the
       * activate now!
       */
      if ((!DATA->on) && (DATA->extdevents & PG_EXEV_TOGGLE)) {
	event = 0;

	/* Mutually exclusive too? */
	if (DATA->extdevents & PG_EXEV_EXCLUSIVE) {
	  struct widget *box;

	  /* Get a pointer to our container */
	  if (!iserror(rdhandle((void**)&box,PG_TYPE_WIDGET,-1,
				self->container)) && box) {
	    struct widget *old;

	    /* If another button is active, disable it */
	    if ((!iserror(rdhandle((void**)&old,PG_TYPE_WIDGET,-1,
				  box->activemutex))) && old) {

	      /* Turn it off */
	      ((struct btndata *)(old->data))->on = 0;
	      div_setstate(old->in->div,
			   ((struct btndata *)(old->data))->over ?
			   ((struct btndata *)(old->data))->state_hilight :
			   ((struct btndata *)(old->data))->state,0);
	    }
	    
	    /* We're the new active widget */
	    box->activemutex = hlookup(self,NULL);
	  }
	}
      }
      DATA->on=1;
    }
    else
      return;
    break;

  case TRIGGER_KEYUP:

#ifdef DEBUG_EVENT
      printf("TRIGGER_KEYUP: button %p, received\n",self);
#endif

    /* Hotkey was pressed, simulate a keypress
     */
    if (param->kbd.key == DATA->hotkey && (param->kbd.flags & DATA->hotkey_flags)) {
      if (DATA->hotkey_consume)
	param->kbd.consume++;

#ifdef DEBUG_EVENT
      printf("TRIGGER_KEYUP: button %p, hotkey_received was %d\n",self, DATA->hotkey_received);
#endif

      /* Make sure we don't do this twice */
      if (DATA->hotkey_received)
	return;
      DATA->hotkey_received = 1;
      
      /* If it's a toggle button, go ahead and make it change state. Otherwise
       * send the event and get out of here without redrawing anything
       */
      
      if (DATA->extdevents & PG_EXEV_TOGGLE) {
	union trigparam mytrig;
	
	/* Simulate a mouse press/release */
	memset(&mytrig,0,sizeof(mytrig));
	mytrig.mouse.chbtn = 1;
	button_trigger(self,TRIGGER_DOWN,&mytrig);
	button_trigger(self,TRIGGER_UP,&mytrig);
      }
      else {
	/* No graphical interaction here, 
	 * so just post the event and get on with it 
	 */
	post_event(PG_WE_ACTIVATE,self,2,0,NULL);
	if (DATA->extdevents & PG_EXEV_PNTR_DOWN)
	  post_event(PG_WE_PNTR_DOWN,self,1,0,NULL);
	if (DATA->extdevents & PG_EXEV_PNTR_UP)
	  post_event(PG_WE_PNTR_UP,self,1,0,NULL);
      }
      return;
    }

    /* Do the fake mouseclick if it's focused and they pressed the activate key
     */
    if (param->kbd.key == hotkey_activate && (param->kbd.flags & PG_KF_FOCUSED))
      param->kbd.consume++;
    else
      return;

    /* Fake a mouse click so popups stick to buttons
     * when triggered by keyboard
     */
    tp = *param;
    param = &tp;
    param->mouse.chbtn = 1;
  case TRIGGER_UP:
    if (DATA->extdevents & PG_EXEV_PNTR_UP)
      post_event(PG_WE_PNTR_UP,self,param->mouse.chbtn,0,NULL);
    if (DATA->on && param->mouse.chbtn==1) {
      if (DATA->extdevents & PG_EXEV_TOGGLE) {
	if (!(DATA->extdevents & PG_EXEV_EXCLUSIVE))
	  DATA->on = (DATA->toggle^=1);
	if (!DATA->on)
	  event = 0;
      }
      else {
	event = 0;
	DATA->on = 0;
      }
    }
    else
      return;
    break;

  case TRIGGER_RELEASE:
    if (param->mouse.chbtn==1 && !(DATA->extdevents & PG_EXEV_TOGGLE))
      DATA->on=0;
    else
      return;
    break;

    /* When the widget is forcibly defocused, go ahead and un-push it */
  case TRIGGER_DEACTIVATE:
    if (DATA->on && !(DATA->extdevents & PG_EXEV_TOGGLE)) {
      DATA->on = 0;
      break;
    }
    else
      return;

  case TRIGGER_ACTIVATE:
    if (DATA->extdevents & PG_EXEV_FOCUS)
      post_event(PG_WE_FOCUS,self,1,0,NULL);
    return;

  case TRIGGER_KEY_START:
#ifdef DEBUG_EVENT
    printf("button %p got TRIGGER_KEY_START\n",self);
#endif
    DATA->hotkey_received = 0;
    break;

  }

#if 0   /*** This probably isn't that necessary, and it is causing
	 *   a 'shifting' in the formatting due to resizes when the hilighted
	 *   state is set.
	 */
  /* Update subwidgets, update this widget, then send an event */
  w = widget_traverse(self, PG_TRAVERSE_CHILDREN, 0);
  while (w) {
    widget_set(w, PG_WP_HILIGHTED, DATA->over);
    w = widget_traverse(w, PG_TRAVERSE_FORWARD,1);
  }
#endif

  button_setstate(self);

  if (event>=0) {
#ifdef CONFIG_BUTTON_BEEP
    /* Little compile-time hack to make the button beep */
    drivermessage(PGDM_SOUNDFX,PG_SND_SHORTBEEP,NULL);
#endif
    post_event(PG_WE_ACTIVATE,self,event,0,NULL);
  }
}

void button_resize(struct widget *self) {
  struct btnposition bp;
  int w,h,m;
  u32 t;
  static u8 lock = 0;

  /* Must prevent reentrancy to avoid an infinite
   * recursion when widget_set() is called */
  if (lock) return;
  lock = 1;

  /* Do theme updates */

  t = theme_lookup(DATA->state,PGTH_P_TEXT);
  if (t) {
     widget_set(self,PG_WP_TEXT,t);
     DATA->theme_text = 1;
  }
  else if (DATA->theme_text) {
     widget_set(self,PG_WP_TEXT,0);
     DATA->theme_text = 0;
  }

  t = theme_lookup(DATA->state,PGTH_P_WIDGETBITMAP);
  if (t) {
     widget_set(self,PG_WP_BITMAP,t);
     DATA->theme_bitmap = 1;
  }
  else if (DATA->theme_bitmap) {
     widget_set(self,PG_WP_BITMAP,0);
     DATA->theme_bitmap = 0;
  }

  t = theme_lookup(DATA->state,PGTH_P_WIDGETBITMASK);
  if (t) {
     widget_set(self,PG_WP_BITMASK,t);
     DATA->theme_bitmask = 1;
  }
  else if (DATA->theme_bitmask) {
     widget_set(self,PG_WP_BITMASK,0);
     DATA->theme_bitmask = 0;
  }

  if (DATA->theme_side) {
    widget_set(self,PG_WP_SIDE, theme_lookup(DATA->state, PGTH_P_SIDE));

    /* widget_set will set theme_side to zero since it thinks we're 
     * doing it manually. Prove widget_set wrong:
     */
    DATA->theme_side = 1;
  }

  lock = 0;
   
  /* Minimum size and margin */
  w = theme_lookup(DATA->state,PGTH_P_WIDTH);
  h = theme_lookup(DATA->state,PGTH_P_HEIGHT);
  m = theme_lookup(DATA->state,PGTH_P_MARGIN) << 1;

  /* Calculate everything */
  position_button(self,&bp);

  if (bp.h > h)
     h = bp.h + m;
  if ((bp.w+m) > w)
     w = bp.w + m;
   
  /* Add spacing between buttons */
  if ((self->in->flags & PG_S_TOP) ||
      (self->in->flags & PG_S_BOTTOM)) {

     /* Vertical */
     h += theme_lookup(DATA->state,PGTH_P_SPACING);

     /* Overridden with PG_WP_SIZE? */
     if (!(self->in->flags & DIVNODE_SIZE_AUTOSPLIT))
       h = self->in->split;
  }
  else if ((self->in->flags & PG_S_LEFT) ||
	   (self->in->flags & PG_S_RIGHT)) {

     /* Horizontal */
     w += theme_lookup(DATA->state,PGTH_P_SPACING);

     /* Overridden with PG_WP_SIZE? */
     if (!(self->in->flags & DIVNODE_SIZE_AUTOSPLIT))
       w = self->in->split;
  }

  /* If one dimension is zero, hide the button completely */
  if (!(w && h))
    w = h = 0;

  self->in->div->pw = w;
  self->in->div->ph = h;
}

/* Code to generate the button coordinates, needed to resize or build the button */
void position_button(struct widget *self,struct btnposition *bp) {
  hwrbitmap bit = NULL,bitmask = NULL;
  struct fontdesc *fd = NULL;
  char *text = NULL;

  bp->font = DATA->font ? DATA->font : 
    theme_lookup(self->in->div->state,PGTH_P_FONT);

  /* If any of our handles have been deleted, set them to zero */

  if (iserror(rdhandle((void **) &bit,PG_TYPE_BITMAP,-1,DATA->bitmap))) {
    DATA->bitmap = 0;
    bit = NULL;
  }
  if (iserror(rdhandle((void **) &bitmask,PG_TYPE_BITMAP,-1,DATA->bitmask))) {
    DATA->bitmask = 0;
    bitmask = NULL;
  }
  if (iserror(rdhandle((void **) &text,PG_TYPE_STRING,-1,DATA->text))) {
    DATA->text = 0;
    text = NULL;
  }
  if (iserror(rdhandle((void **) &fd,PG_TYPE_FONTDESC,-1,bp->font))) {
    /* Turn off our custom font and try the theme's font */
    DATA->font = 0;
    bp->font = theme_lookup(self->in->div->state,PGTH_P_FONT);    
    if (iserror(rdhandle((void **) &fd,PG_TYPE_FONTDESC,-1,bp->font))) {
      /* Still bad? 'Tis a bug in the theme. Fall back on the default font */
      rdhandle((void **) &fd,PG_TYPE_FONTDESC,-1,defaultfont);
    }
  }

  /* Find sizes */
  if (text)
    sizetext(fd,&bp->tw,&bp->th,text);
  if (bit)
    VID(bitmap_getsize) (bit,&bp->bw,&bp->bh);
  
  /* Position the text and bitmap relative to each other */
  if (text && bit) {
    int s = theme_lookup(self->in->div->state,PGTH_P_BITMAPSIDE);
    int m = theme_lookup(self->in->div->state,PGTH_P_BITMAPMARGIN);

    if(DATA->bitmap_side >= 0) {
      s = DATA->bitmap_side;
    }

    if (s & (PG_S_TOP | PG_S_BOTTOM)) {
      /* Horizontal positioning for top/bottom */
      if (bp->bw>bp->tw) {
	bp->w = bp->bw;
	bp->bx = 0;
	bp->tx = (bp->bw-bp->tw)>>1;
      }
      else {
	bp->w = bp->tw;
	bp->tx = 0;
	bp->bx = (bp->tw-bp->bw)>>1;
      }

      /* Vertical size */
      bp->h = bp->th+bp->bh+m;

      if (s & PG_S_TOP) {
	/* Vertical positioning for top */
	bp->by = 0;
	bp->ty = bp->bh+m;
      }
      else {
	/* Vertical positioning for bottom */
	bp->ty = 0;
	bp->by = bp->th+m;
      }
    }
    else {
      /* Vertical positioning for left/right */
      if (bp->bh>bp->th) {
	bp->h = bp->bh;
	bp->by = 0;
	bp->ty = (bp->bh-bp->th)>>1;
      }
      else {
	bp->h = bp->th;
	bp->ty = 0;
	bp->by = (bp->th-bp->bh)>>1;
      }

      /* Horizontal size */
      bp->w = bp->tw+bp->bw+m;

      if (s & PG_S_LEFT) {
	/* Horizontal positioning for left */
	bp->bx = 0;
	bp->tx = bp->bw+m;
      }
      else {
	/* Horizontal positioning for right */
	bp->tx = 0;
	bp->bx = bp->tw+m;
      }
    }      
  }

  /* Only one? */
  else if (text) {
    bp->w = bp->tw;
    bp->h = bp->th;
    bp->tx = bp->ty = 0;
  }
  else if (bit) {
    bp->w = bp->bw;
    bp->h = bp->bh;
    bp->bx = bp->by = 0;
  }
  else {
    bp->w = 0;
    bp->h = 0;
  }

  /* Remember if the bitmap has an alpha channel */
  DATA->has_alpha = bit && bp->bw && ( VID(getpixel)(bit,0,0) & PGCF_ALPHA );
}

/* The End */



