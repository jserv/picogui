/* $Id: button.c,v 1.66 2001/08/03 14:56:11 micahjd Exp $
 *
 * button.c - generic button, with a string or a bitmap
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
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/appmgr.h>

struct btndata {
   unsigned int on : 1;
   unsigned int over : 1;
   unsigned int toggle : 1;

   /* These keep track of when one of the parameters was customized by
    * a theme (as opposed to the app) so it can be appropriately unloaded */
   unsigned int theme_bitmap : 1;
   unsigned int theme_bitmask : 1;
   unsigned int theme_text : 1;
   
   handle bitmap,bitmask,text,font;
   
   /* Hooks for embedding a button in another widget */
   int state,state_on,state_hilight;
   struct widget *extra;  /* the owner of a customized button */
   void (*event)(struct widget *extra,struct widget *button);
   
   /* Mask of extended (other than ACTIVATE) events to send
    * and other flags*/
   int extdevents;
};
#define DATA ((struct btndata *)(self->data))

/* Customizes the button's appearance
   (used by other widgets that embed buttons in themeselves) */
void customize_button(struct widget *self,int state,int state_on,int state_hilight,
		      void *extra, void (*event)(struct widget *extra,struct widget *button)) {
  self->in->div->state = DATA->state = state;
  DATA->state_on = state_on;
  DATA->state_hilight = state_hilight;
  DATA->extra = extra;
  DATA->event = event;

  resizewidget(self);
}

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
    addgropsz(c,PG_GROP_BITMAP,bp.x+bp.bx,bp.y+bp.by,bp.bw,bp.bh);
    c->current->param[0] = DATA->bitmap;
    if (DATA->bitmask) {
       addgrop(c,PG_GROP_SETLGOP);
       c->current->param[0] = PG_LGOP_NONE;
    }
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

  /* Default states */
  DATA->state = PGTH_O_BUTTON;
  DATA->state_on = PGTH_O_BUTTON_ON;
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
  self->in->div->flags |= DIVNODE_HOTSPOT;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | TRIGGER_HOTKEY |
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_DIRECT |
    TRIGGER_KEYUP | TRIGGER_KEYDOWN | TRIGGER_DEACTIVATE;

  self->out = &self->in->next;
  self->sub = &self->in->div->div;

  return sucess;
}

void button_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error button_set(struct widget *self,int property, glob data) {
  g_error e;
  hwrbitmap bit;
  char *str;
  struct fontdesc *fd;

  switch (property) {

  case PG_WP_BITMAP:
    if (iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)))
       return mkerror(PG_ERRT_HANDLE,33);
     
    DATA->bitmap = (handle) data;
    resizewidget(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_BITMASK:
    if (iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)))
       return mkerror(PG_ERRT_HANDLE,34);

    DATA->bitmask = (handle) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_FONT:
    if (iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,data))) 
	 return mkerror(PG_ERRT_HANDLE,35);
    DATA->font = (handle) data;
    resizewidget(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_TEXT:
    if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,data))) 
       return mkerror(PG_ERRT_HANDLE,36);
    DATA->text = (handle) data;
    resizewidget(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_EXTDEVENTS:
    DATA->extdevents = data;
    break;

  case PG_WP_ON:
    DATA->on = data;
    /* Fake a trigger to redraw the button */
    button_trigger(self,0,NULL);
    break;
     
   default:
     return mkerror(ERRT_PASS,0);
  }
  return sucess;
}

glob button_get(struct widget *self,int property) {
  g_error e;
  handle h;

  switch (property) {

  case PG_WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case PG_WP_BITMAP:
    return DATA->bitmap;

  case PG_WP_BITMASK:
    return DATA->bitmask;

  case PG_WP_EXTDEVENTS:
    return DATA->extdevents;

  case PG_WP_FONT:
    return (glob) DATA->font;

  case PG_WP_TEXT:
    return (glob) DATA->text;

  case PG_WP_HOTKEY:
    return (glob) self->hotkey;
     
  case PG_WP_ON:
    return (glob) DATA->on;

  default:
    return 0;
  }
}

void button_trigger(struct widget *self,long type,union trigparam *param) {
  int event=-1;

  /* Figure out the button's new state */
  switch (type) {

  case TRIGGER_ENTER:
    DATA->over=1;
    break;
    
  case TRIGGER_LEAVE:
    DATA->over=0;
    break;
   
  case TRIGGER_KEYDOWN:
    if (param->kbd.key != PGKEY_SPACE) {
      global_hotkey(param->kbd.key,param->kbd.mods,type);
      return;
    }
    param->mouse.chbtn = 1;
  case TRIGGER_DOWN:
    if (DATA->extdevents & PG_EXEV_PNTR_DOWN)
      post_event(PG_WE_PNTR_DOWN,self,param->mouse.chbtn,0,NULL);
    if (param->mouse.chbtn==1 && !(DATA->extdevents & PG_EXEV_NOCLICK)) {
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
    if (param->kbd.key != PGKEY_SPACE) {
      global_hotkey(param->kbd.key,param->kbd.mods,type);
      return;
    }
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

  case TRIGGER_HOTKEY:
  case TRIGGER_DIRECT:
    /* No graphical interaction here, so just
       post the event and get on with it */
    post_event(PG_WE_ACTIVATE,self,2,0,NULL);
    return;
    
  }

  /* Update, THEN send the event. */

  if (DATA->on)
    div_setstate(self->in->div,DATA->state_on,0);
  else if (DATA->over)
    div_setstate(self->in->div,DATA->state_hilight,0);
  else
    div_setstate(self->in->div,DATA->state,0);

  if (event>=0) {
    if (DATA->event)
      (*DATA->event)(DATA->extra,self);
    else
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
  }
  else if ((self->in->flags & PG_S_LEFT) ||
	   (self->in->flags & PG_S_RIGHT)) {

     /* Horizontal */
     w += theme_lookup(DATA->state,PGTH_P_SPACING);
  }

  self->in->div->pw = w;
  self->in->div->ph = h;
}

/* Code to generate the button coordinates, needed to resize or build the button */
void position_button(struct widget *self,struct btnposition *bp) {
  hwrbitmap bit = NULL,bitmask = NULL;
  struct fontdesc *fd = NULL;
  char *text = NULL;

  /* These shouldn't fail! If we're debugging, do some sanity checks */
#ifdef DEBUG_KEYS
  if (iserror(rdhandle((void **) &bit,PG_TYPE_BITMAP,-1,DATA->bitmap)))
     guru("Error dereferencing bitmap handle in position_button()\n0x%08X",
	  DATA->bitmap);
  if (iserror(rdhandle((void **) &bitmask,PG_TYPE_BITMAP,-1,DATA->bitmask)))
     guru("Error dereferencing bitmask handle in position_button()\n0x%08X",
	  DATA->bitmask);
  if (iserror(rdhandle((void **) &text,PG_TYPE_STRING,-1,DATA->text)))
     guru("Error dereferencing text handle in position_button()\n0x%08X",
	  DATA->text);
#else
  if (iserror(rdhandle((void **) &bit,PG_TYPE_BITMAP,-1,DATA->bitmap)))
     return;
  if (iserror(rdhandle((void **) &bitmask,PG_TYPE_BITMAP,-1,DATA->bitmask)))
     return;
  if (iserror(rdhandle((void **) &text,PG_TYPE_STRING,-1,DATA->text)))
     return;
#endif
  bp->font = DATA->font ? DATA->font : theme_lookup(self->in->div->state,PGTH_P_FONT);
  rdhandle((void **) &fd,PG_TYPE_FONTDESC,-1,bp->font);

  /* Find sizes */
  if (text)
    sizetext(fd,&bp->tw,&bp->th,text);
  if (bit)
    VID(bitmap_getsize) (bit,&bp->bw,&bp->bh);
  
  /* Position the text and bitmap relative to each other */
  if (text && bit) {
    int s = theme_lookup(self->in->div->state,PGTH_P_BITMAPSIDE);
    int m = theme_lookup(self->in->div->state,PGTH_P_BITMAPMARGIN);

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
}

/* The End */



