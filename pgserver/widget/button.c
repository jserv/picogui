/* $Id: button.c,v 1.47 2001/01/05 06:42:28 micahjd Exp $
 *
 * button.c - generic button, with a string or a bitmap
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
#include <pgserver/appmgr.h>

struct btndata {
  int on,over;
  handle bitmap,bitmask,text,font;

  /* Hooks for embedding a button in another widget */
  int state,state_on,state_hilight;
  struct widget *extra;  /* the owner of a customized button */
  void (*event)(struct widget *extra,struct widget *button);

  /* Mask of extended (other than ACTIVATE) events to send */
  int extdevents;
};
#define DATA ((struct btndata *)(self->data))

void resize_button(struct widget *self);

/* Customizes the button's appearance
   (used by other widgets that embed buttons in themeselves) */
void customize_button(struct widget *self,int state,int state_on,int state_hilight,
		      void *extra, void (*event)(struct widget *extra,struct widget *button)) {
  self->in->div->state = DATA->state = state;
  DATA->state_on = state_on;
  DATA->state_hilight = state_hilight;
  DATA->extra = extra;
  DATA->event = event;

  button_set(self,PG_WP_TEXT,theme_lookup(state,PGTH_P_TEXT));
  button_set(self,PG_WP_BITMAP,theme_lookup(state,PGTH_P_WIDGETBITMAP));
  button_set(self,PG_WP_BITMASK,theme_lookup(state,PGTH_P_WIDGETBITMASK));

  resize_button(self);
}

struct btnposition {
  /* Coordinates calculated in position_button */
  int x,y,w,h;  /* Coordinates of bitmap and text combined */
  int bx,by,bw,bh;  /* Bitmap, relative to bitmap+text */
  int tx,ty,tw,th;  /* Text, relative to bitmap+text */ 

  /* position_button looks this up anyway */
  handle font;
};

/* Code to generate the button coordinates, needed to resize or build the button */
void position_button(struct widget *self,struct btnposition *bp);

void build_button(struct gropctxt *c,unsigned short state,struct widget *self) {
  struct btnposition bp;

  /* Background */
  exec_fillstyle(c,state,PGTH_P_BGFILL);

  position_button(self,&bp);

  /* Align the whole thing */
  align(c,theme_lookup(state,PGTH_P_ALIGN),&bp.w,&bp.h,&bp.x,&bp.y);

  /* AND the mask, then OR the bitmap. Yay for transparency effects! */
  if (DATA->bitmask) {
    addgrop(c,PG_GROP_BITMAP,bp.x+bp.bx,bp.y+bp.by,bp.bw,bp.bh);
    c->current->param[0] = DATA->bitmask;
    c->current->param[1] = PG_LGOP_AND;
    c->current->param[2] = 0;
    c->current->param[3] = 0;
  }
  if (DATA->bitmap) {
    addgrop(c,PG_GROP_BITMAP,bp.x+bp.bx,bp.y+bp.by,bp.bw,bp.bh);
    c->current->param[0] = DATA->bitmap;
    c->current->param[1] = DATA->bitmask ? PG_LGOP_OR : PG_LGOP_NONE;
    c->current->param[2] = 0;
    c->current->param[3] = 0; 
  }

  /* Text */
  if (DATA->text) {
    addgrop(c,PG_GROP_TEXT,bp.x+bp.tx,bp.y+bp.ty,bp.tw,bp.th);
    c->current->param[0] = DATA->text;
    c->current->param[1] = bp.font;
    c->current->param[2] = (*vid->color_pgtohwr)
       (theme_lookup(state,PGTH_P_FGCOLOR));
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

  /* Spacer (between buttons) */
  e = newdiv(&self->in->next,self);
  errorcheck;
  self->in->next->flags |= PG_S_LEFT;
  self->out = &self->in->next->next;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | TRIGGER_HOTKEY |
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_DIRECT;

  self->resize = &resize_button;

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
  int psplit;

  switch (property) {

  case PG_WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,31);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    if (data!=PG_S_ALL) {
      self->in->next->flags &= SIDEMASK;
      self->in->next->flags |= ((sidet)data);
      resize_button(self);
    }
    self->dt->flags |= DIVTREE_NEED_RECALC;
    redraw_bg(self);
    break;

  case PG_WP_BITMAP:
    if (!iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)) && bit) {
      DATA->bitmap = (handle) data;
      psplit = self->in->split;
      resize_button(self);
      if (self->in->split != psplit) {
	redraw_bg(self);
	self->in->flags |= DIVNODE_PROPAGATE_RECALC;
      }
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(PG_ERRT_HANDLE,33);
    break;

  case PG_WP_BITMASK:
    if (!iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)) && bit) {
      DATA->bitmask = (handle) data;
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(PG_ERRT_HANDLE,34);
    break;

  case PG_WP_FONT:
    if (!iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,data)) || !fd) 
      return mkerror(PG_ERRT_HANDLE,35);
    DATA->font = (handle) data;
    psplit = self->in->split;
    resize_button(self);
    if (self->in->split != psplit) {
      redraw_bg(self);
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_TEXT:
    if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,data)) || !str) 
      return mkerror(PG_ERRT_HANDLE,36);
    DATA->text = (handle) data;
    psplit = self->in->split;
    resize_button(self);
    if (self->in->split != psplit) {
      redraw_bg(self);
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_HOTKEY:
    install_hotkey(self,data);
    break;

  case PG_WP_EXTDEVENTS:
    DATA->extdevents = data;
    break;

  default:
    return mkerror(PG_ERRT_BADPARAM,37);
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
    
  case TRIGGER_DOWN:
    if (DATA->extdevents & PG_EXEV_PNTR_DOWN)
      post_event(PG_WE_PNTR_DOWN,self,param->mouse.chbtn,0,NULL);
    if (param->mouse.chbtn==1 && !(DATA->extdevents & PG_EXEV_NOCLICK))
      DATA->on=1;
    else
      return;
    break;

  case TRIGGER_UP:
    if (DATA->extdevents & PG_EXEV_PNTR_UP)
      post_event(PG_WE_PNTR_UP,self,param->mouse.chbtn,0,NULL);
    if (DATA->on && param->mouse.chbtn==1) {
      event = 0;
      DATA->on=0;
    }
    else
      return;
    break;

  case TRIGGER_RELEASE:
    if (param->mouse.chbtn==1)
      DATA->on=0;
    else
      return;
    break;

  case TRIGGER_HOTKEY:
  case TRIGGER_DIRECT:
    /* No graphical interaction here, so just
       post the event and get on with it */
    post_event(PG_WE_ACTIVATE,self,2,0,NULL);
    return;
    
  }

  /* Update, THEN send the event. */

  if (DATA->on)
    div_setstate(self->in->div,DATA->state_on);
  else if (DATA->over)
    div_setstate(self->in->div,DATA->state_hilight);
  else
    div_setstate(self->in->div,DATA->state);

  if (event>=0) {
    if (DATA->event)
      (*DATA->event)(DATA->extra,self);
    else
      post_event(PG_WE_ACTIVATE,self,event,0,NULL);
  }
}

/* HWG_BUTTON is the minimum size (either dimension) for a button.
   This function resizes if the text or bitmap goes over that minimum.
*/
void resize_button(struct widget *self) {
  struct btnposition bp;
  int w,h,m;

  /* With PG_S_ALL we'll get ignored anyway... */
  if (self->in->flags & PG_S_ALL) return;

  /* Space between buttons */
  self->in->next->split = theme_lookup(DATA->state,PGTH_P_SPACING);

  /* Minimum size and margin */
  w = theme_lookup(DATA->state,PGTH_P_WIDTH);
  h = theme_lookup(DATA->state,PGTH_P_HEIGHT);
  m = theme_lookup(DATA->state,PGTH_P_MARGIN);

  /* Calculate everything */
  position_button(self,&bp);

  /* Orientation */
  if ((self->in->flags & PG_S_TOP) ||
      (self->in->flags & PG_S_BOTTOM)) {

    /* Vertical */
    if (bp.h > h)
      self->in->split = bp.h;
    else
      self->in->split = h;
  }
  else if ((self->in->flags & PG_S_LEFT) ||
	   (self->in->flags & PG_S_RIGHT)) {

    /* Horizontal */
    bp.w += m<<1;
    if (bp.w > w)
      self->in->split = bp.w;
    else
      self->in->split = w;
  }
}

/* Code to generate the button coordinates, needed to resize or build the button */
void position_button(struct widget *self,struct btnposition *bp) {
  hwrbitmap bit = NULL,bitmask = NULL;
  struct fontdesc *fd = NULL;
  char *text = NULL;

  /* Dereference */
  rdhandle((void **) &bit,PG_TYPE_BITMAP,-1,DATA->bitmap);
  rdhandle((void **) &bitmask,PG_TYPE_BITMAP,-1,DATA->bitmask);
  rdhandle((void **) &text,PG_TYPE_STRING,-1,DATA->text);
  bp->font = DATA->font ? DATA->font : theme_lookup(self->in->div->state,PGTH_P_FONT);
  rdhandle((void **) &fd,PG_TYPE_FONTDESC,-1,bp->font);

  /* Find sizes */
  if (text)
    sizetext(fd,&bp->tw,&bp->th,text);
  if (bit)
    (*vid->bitmap_getsize)(bit,&bp->bw,&bp->bh);
  
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



