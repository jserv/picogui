/* $Id: button.c,v 1.28 2000/08/14 19:35:45 micahjd Exp $
 *
 * button.c - generic button, with a string or a bitmap
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
#include <theme.h>

/* Button content offsetted in X and Y coords by this much when on */
#define ON_OFFSET 1

struct btndata {
  int on,over;
  int x,y;
  int dxt;    /* X of the text, relative to the x above */
  int dyt;    /* Ditto, for the text's Y */
  handle bitmap,bitmask,text,font;
  int align;
  devcolort textcolor;
};
#define DATA ((struct btndata *)(self->data))

void resizebutton(struct widget *self);
void buttonstate(struct widget *self);

void button(struct divnode *d) {
  int ex,ey,ew,eh,x,y,txt_h;
  int w=0,h=0; /* Size of the bitmap and text combined */
  struct bitmap *bit=NULL,*bitmask=NULL;
  char *text=NULL;
  struct fontdesc *fd=NULL;
  struct widget *self = d->owner;

  ex=ey=0; ew=d->w; eh=d->h;

#ifdef DEBUG
  printf("Button recalc\n");
#endif

  addelement(d,&current_theme[E_BUTTON_BORDER],&ex,&ey,&ew,&eh);
  addelement(d,&current_theme[E_BUTTON_FILL],&ex,&ey,&ew,&eh);

  /* Dereference the handles */
  rdhandle((void **) &bit,TYPE_BITMAP,-1,DATA->bitmap);
  rdhandle((void **) &bitmask,TYPE_BITMAP,-1,DATA->bitmask);
  rdhandle((void **) &text,TYPE_STRING,-1,DATA->text);
  rdhandle((void **) &fd,TYPE_FONTDESC,-1,DATA->font);

  /* Find the total width and height */
  if (text) {
    sizetext(fd,&w,&h,text);
    txt_h = h;
  }
  if (bit) {
    w += bit->w;
    if (bit->h>h) h = bit->h;
  }
  if (bit && text)
    w += HWG_MARGIN;

  /* Align, and save the upper-left coord for later */
  align(d,DATA->align,&w,&h,&x,&y);

  /* If the text is bigger than the bitmap, center the bitmap in the text */
  if (text && bit && bit->h<txt_h)
      y-=(DATA->dyt=(bit->h>>1)-(txt_h>>1));

  DATA->x = x;
  DATA->y = y;

  /* AND the mask, then OR the bitmap. Yay for transparency effects! */
  if (DATA->on) {
    x+=ON_OFFSET;
    y+=ON_OFFSET;
  }
  if (DATA->bitmask)
    grop_bitmap(&d->grop,x,y,bitmask->w,bitmask->h,DATA->bitmask,LGOP_AND);
  else
    grop_null(&d->grop);
  if (DATA->bitmap)
    grop_bitmap(&d->grop,x,y,bit->w,bit->h,DATA->bitmap,LGOP_OR);
  else
    grop_null(&d->grop);

  /* Put the text to the right of the bitmap, and vertically center them
   * independently
   */
  if (DATA->text) {
    if (bit) {
      x+=(DATA->dxt=bit->w+HWG_MARGIN);
      /* If the bitmap is bigger, center the text relative to the bitmap */
      if (bit->h>txt_h) 
	DATA->dyt=(bit->h>>1)-(txt_h>>1);
      y+=DATA->dyt;
    }
    else
      DATA->dxt = DATA->dyt = 0;
    grop_text(&d->grop,x,y,DATA->font,DATA->textcolor,DATA->text);
  }
  else
    grop_null(&d->grop);
  
  addelement(d,&current_theme[E_BUTTON_OVERLAY],&ex,&ey,&ew,&eh);

  buttonstate(self);
}

/* Find the current visual state and apply it */
void buttonstate(struct widget *self) {
  int state,x,y;

  /* This code for updating the button's appearance modifies
     the grops directly because it does not need a recalc, only
     a single-node redraw. Recalcs propagate like a virus, and
     require recreating grop-lists.
     Redraws don't spread to other nodes, and they are very fast.
  */
  x = DATA->x;
  y = DATA->y;
  if (DATA->on && DATA->over) {
    state = STATE_ACTIVATE;
    x += ON_OFFSET;
    y += ON_OFFSET;
  }
  else if (DATA->over)
    state = STATE_HILIGHT;
  else
    state = STATE_NORMAL;
  applystate(self->in->div->grop,
	     &current_theme[E_BUTTON_BORDER],state);
  applystate(self->in->div->grop->next,
	     &current_theme[E_BUTTON_FILL],state);
  applystate(self->in->div->grop->next->next->next->next->next,
	     &current_theme[E_BUTTON_OVERLAY],state);
  self->in->div->grop->next->next->x = x;
  self->in->div->grop->next->next->next->x = x;
  self->in->div->grop->next->next->y = y;
  self->in->div->grop->next->next->next->y = y;
  self->in->div->grop->next->next->next->next->x = x+DATA->dxt;
  self->in->div->grop->next->next->next->next->y = y+DATA->dyt;
}

/* Pointers, pointers, and more pointers. What's the point?
   Set up some divnodes!
*/
g_error button_install(struct widget *self) {
  g_error e;

  e = g_malloc(&self->data,sizeof(struct btndata));
  errorcheck;
  memset(self->data,0,sizeof(struct btndata));

  DATA->font = defaultfont;
  DATA->align = A_CENTER;

  /* Main split */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= S_LEFT;
  self->in->split = HWG_BUTTON;

  /* Visible node */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->on_recalc = &button;

  /* Spacer (between buttons) */
  e = newdiv(&self->in->next,self);
  errorcheck;
  self->in->next->flags |= S_LEFT;
  self->in->next->split = HWG_MARGIN;
  self->out = &self->in->next->next;

  self->trigger_mask = TRIGGER_ENTER | TRIGGER_LEAVE | TRIGGER_HOTKEY |
    TRIGGER_UP | TRIGGER_DOWN | TRIGGER_RELEASE | TRIGGER_DIRECT;

  return sucess;
}

void button_remove(struct widget *self) {
  g_free(self->data);
  if (!in_shutdown)
    r_divnode_free(self->in);
}

g_error button_set(struct widget *self,int property, glob data) {
  g_error e;
  struct bitmap *bit;
  char *str;
  struct fontdesc *fd;
  int psplit;

  switch (property) {

  case WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(ERRT_BADPARAM,31);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    if (data!=S_ALL) {
      self->in->next->flags &= SIDEMASK;
      self->in->next->flags |= ((sidet)data);
      resizebutton(self);
    }
    self->dt->flags |= DIVTREE_NEED_RECALC;
    redraw_bg(self);
    break;

  case WP_ALIGN:
    if (data > AMAX) return mkerror(ERRT_BADPARAM,32);
    DATA->align = (alignt) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_COLOR:
    DATA->textcolor = cnvcolor(data);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_BITMAP:
    if (!iserror(rdhandle((void **)&bit,TYPE_BITMAP,-1,data)) && bit) {
      DATA->bitmap = (handle) data;
      psplit = self->in->split;
      resizebutton(self);
      if (self->in->split != psplit) {
	redraw_bg(self);
	self->in->flags |= DIVNODE_PROPAGATE_RECALC;
      }
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(ERRT_HANDLE,33);
    break;

  case WP_BITMASK:
    if (!iserror(rdhandle((void **)&bit,TYPE_BITMAP,-1,data)) && bit) {
      DATA->bitmask = (handle) data;
      self->in->flags |= DIVNODE_NEED_RECALC;
      self->dt->flags |= DIVTREE_NEED_RECALC;
    }
    else return mkerror(ERRT_HANDLE,34);
    break;

  case WP_FONT:
    if (!iserror(rdhandle((void **)&fd,TYPE_FONTDESC,-1,data)) || !fd) 
      return mkerror(ERRT_HANDLE,35);
    DATA->font = (handle) data;
    psplit = self->in->split;
    resizebutton(self);
    if (self->in->split != psplit) {
      redraw_bg(self);
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_TEXT:
    if (iserror(rdhandle((void **)&str,TYPE_STRING,-1,data)) || !str) 
      return mkerror(ERRT_HANDLE,36);
    DATA->text = (handle) data;
    psplit = self->in->split;
    resizebutton(self);
    if (self->in->split != psplit) {
      redraw_bg(self);
      self->in->flags |= DIVNODE_PROPAGATE_RECALC;
    }
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case WP_HOTKEY:
    install_hotkey(self,data);
    break;

  default:
    return mkerror(ERRT_BADPARAM,37);
  }
  return sucess;
}

glob button_get(struct widget *self,int property) {
  g_error e;
  handle h;

  switch (property) {

  case WP_SIDE:
    return self->in->flags & (~SIDEMASK);

  case WP_ALIGN:
    return DATA->align;

  case WP_COLOR:
    return DATA->textcolor;

  case WP_BITMAP:
    return DATA->bitmap;

  case WP_BITMASK:
    return DATA->bitmask;

  case WP_FONT:
    return (glob) DATA->font;

  case WP_TEXT:
    return (glob) DATA->text;

  case WP_HOTKEY:
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
    if (param->mouse.chbtn==1)
      DATA->on=1;
    else
      return;
    break;

  case TRIGGER_UP:
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
    post_event(WE_ACTIVATE,self,2,0);
    return;
    
  }

  /* If we're busy rebuilding the grop list, don't bother poking
     at the individual nodes */
  if (self->in->div->grop_lock || !self->in->div->grop)
    return;

  buttonstate(self);

  /* Update, THEN send the event. */

  self->in->div->flags |= DIVNODE_NEED_REDRAW;
  self->dt->flags |= DIVTREE_NEED_REDRAW;   
  if (self->dt==dts->top) update();
  if (event>=0)
    post_event(WE_ACTIVATE,self,event,0);
}

/* HWG_BUTTON is the minimum size (either dimension) for a button.
   This function resizes if the text or bitmap goes over that minimum.
*/
void resizebutton(struct widget *self) {
  int w=0,h=0; /* Size of the bitmap and text combined */
  struct bitmap *bit=NULL,*bitmask=NULL;
  char *text=NULL;
  struct fontdesc *fd=NULL;

  /* With S_ALL we'll get ignored anyway... */
  if (self->in->flags & S_ALL) return;
  
  /* Dereference the handles */
  rdhandle((void **) &bit,TYPE_BITMAP,-1,DATA->bitmap);
  rdhandle((void **) &bitmask,TYPE_BITMAP,-1,DATA->bitmask);
  rdhandle((void **) &text,TYPE_STRING,-1,DATA->text);
  rdhandle((void **) &fd,TYPE_FONTDESC,-1,DATA->font);

#ifdef DEBUG
  printf("Resize button.  Text: '%s'\n",text);
#endif

  /* Find the total width and height */
  if (text)
    sizetext(fd,&w,&h,text);
  if (bit) {
    w += bit->w;
    if (bit->h>h) h = bit->h;
  }
  if (bit && text)
    w += HWG_MARGIN;

  /* Set split to w or h depending on split orientation */
  if ((self->in->flags & S_TOP) ||
      (self->in->flags & S_BOTTOM))
    self->in->split = h;
  else if ((self->in->flags & S_LEFT) ||
	   (self->in->flags & S_RIGHT))
    self->in->split = w;

  /* HWG_BUTTON is the minimum */
  if (self->in->split < HWG_BUTTON)
    self->in->split = HWG_BUTTON;
  else {
    /* If we're going over the limit anyway, add some margin */
    self->in->split += HWG_MARGIN<<1;
  }
}

/* The End */



