/* $Id: button.c,v 1.1 2000/11/06 00:31:36 micahjd Exp $
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

/* Button content offsetted in X and Y coords by this much when on */
#define ON_OFFSET 1

struct btndata {
  int on,over;
  int x,y;
  int dxt;    /* X of the text, relative to the x above */
  int dyt;    /* Ditto, for the text's Y */
  handle bitmap,bitmask,text,font;
  int align;
  hwrcolor textcolor;
};
#define DATA ((struct btndata *)(self->data))

void resizebutton(struct widget *self);
void buttonstate(struct widget *self);

void button(struct divnode *d) {
  int ex,ey,ew,eh,x,y,txt_h;
  int w=0,h=0; /* Size of the bitmap and text combined */
  struct bitmap *bit=NULL,*bitmask=NULL;
  int bw,bh;  /* Bitmap size */
  char *text=NULL;
  struct fontdesc *fd=NULL;
  struct widget *self = d->owner;

  ex=ey=0; ew=d->w; eh=d->h;

#ifdef DEBUG
  printf("Button recalc\n");
#endif

  addelement(d,&current_theme[PG_E_BUTTON_BORDER],&ex,&ey,&ew,&eh);
  addelement(d,&current_theme[PG_E_BUTTON_FILL],&ex,&ey,&ew,&eh);

  /* Dereference the handles */
  rdhandle((void **) &bit,PG_TYPE_BITMAP,-1,DATA->bitmap);
  rdhandle((void **) &bitmask,PG_TYPE_BITMAP,-1,DATA->bitmask);
  rdhandle((void **) &text,PG_TYPE_STRING,-1,DATA->text);
  rdhandle((void **) &fd,PG_TYPE_FONTDESC,-1,DATA->font);

  /* Find the total width and height */
  if (text) {
    sizetext(fd,&w,&h,text);
    txt_h = h;
  }
  if (bit) {
    (*vid->bitmap_getsize)(bit,&bw,&bh);
    w += bw;
    if (bh>h) h = bh;
  }
  if (bit && text)
    w += HWG_MARGIN;

  /* Align, and save the upper-left coord for later */
  align(d,DATA->align,&w,&h,&x,&y);

  /* If the text is bigger than the bitmap, center the bitmap in the text */
  if (text && bit && bh<txt_h)
      y-=(DATA->dyt=(bh>>1)-(txt_h>>1));

  DATA->x = x;
  DATA->y = y;

  /* AND the mask, then OR the bitmap. Yay for transparency effects! */
  if (DATA->on) {
    x+=ON_OFFSET;
    y+=ON_OFFSET;
  }
  if (DATA->bitmask)
    grop_bitmap(&d->grop,x,y,bw,bh,DATA->bitmask,PG_LGOP_AND);
  else
    grop_null(&d->grop);
  if (DATA->bitmap)
    grop_bitmap(&d->grop,x,y,bw,bh,DATA->bitmap,PG_LGOP_OR);
  else
    grop_null(&d->grop);

  /* Put the text to the right of the bitmap, and vertically center them
   * independently
   */
  if (DATA->text) {
    if (bit) {
      x+=(DATA->dxt=bw+HWG_MARGIN);
      /* If the bitmap is bigger, center the text relative to the bitmap */
      if (bh>txt_h) 
	DATA->dyt=(bh>>1)-(txt_h>>1);
      y+=DATA->dyt;
    }
    else
      DATA->dxt = DATA->dyt = 0;
    grop_text(&d->grop,x,y,DATA->font,DATA->textcolor,DATA->text);
  }
  else
    grop_null(&d->grop);
  
  addelement(d,&current_theme[PG_E_BUTTON_OVERLAY],&ex,&ey,&ew,&eh);

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
    state = PG_STATE_ACTIVATE;
    x += ON_OFFSET;
    y += ON_OFFSET;
  }
  else if (DATA->over)
    state = PG_STATE_HILIGHT;
  else
    state = PG_STATE_NORMAL;
  applystate(self->in->div->grop,
	     &current_theme[PG_E_BUTTON_BORDER],state);
  applystate(self->in->div->grop->next,
	     &current_theme[PG_E_BUTTON_FILL],state);
  applystate(self->in->div->grop->next->next->next->next->next,
	     &current_theme[PG_E_BUTTON_OVERLAY],state);
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
  DATA->align = PG_A_CENTER;

  /* Main split */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_LEFT;
  self->in->split = HWG_BUTTON;

  /* Visible node */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->on_recalc = &button;

  /* Spacer (between buttons) */
  e = newdiv(&self->in->next,self);
  errorcheck;
  self->in->next->flags |= PG_S_LEFT;
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

  case PG_WP_SIDE:
    if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,31);
    self->in->flags &= SIDEMASK;
    self->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC |
      DIVNODE_PROPAGATE_RECALC;
    if (data!=PG_S_ALL) {
      self->in->next->flags &= SIDEMASK;
      self->in->next->flags |= ((sidet)data);
      resizebutton(self);
    }
    self->dt->flags |= DIVTREE_NEED_RECALC;
    redraw_bg(self);
    break;

  case PG_WP_ALIGN:
    if (data > PG_AMAX) return mkerror(PG_ERRT_BADPARAM,32);
    DATA->align = (alignt) data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_COLOR:
    DATA->textcolor = data;
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;

  case PG_WP_BITMAP:
    if (!iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)) && bit) {
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
    resizebutton(self);
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
    resizebutton(self);
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

  case PG_WP_ALIGN:
    return DATA->align;

  case PG_WP_COLOR:
    return DATA->textcolor;

  case PG_WP_BITMAP:
    return DATA->bitmap;

  case PG_WP_BITMASK:
    return DATA->bitmask;

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
    post_event(PG_WE_ACTIVATE,self,2,0);
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
    post_event(PG_WE_ACTIVATE,self,event,0);
}

/* HWG_BUTTON is the minimum size (either dimension) for a button.
   This function resizes if the text or bitmap goes over that minimum.
*/
void resizebutton(struct widget *self) {
  int w=0,h=0; /* Size of the bitmap and text combined */
  int bw,bh; /* Bitmap size */
  struct bitmap *bit=NULL,*bitmask=NULL;
  char *text=NULL;
  struct fontdesc *fd=NULL;

  /* With PG_S_ALL we'll get ignored anyway... */
  if (self->in->flags & PG_S_ALL) return;
  
  /* Dereference the handles */
  rdhandle((void **) &bit,PG_TYPE_BITMAP,-1,DATA->bitmap);
  rdhandle((void **) &bitmask,PG_TYPE_BITMAP,-1,DATA->bitmask);
  rdhandle((void **) &text,PG_TYPE_STRING,-1,DATA->text);
  rdhandle((void **) &fd,PG_TYPE_FONTDESC,-1,DATA->font);

#ifdef DEBUG
  printf("Resize button.  Text: '%s'\n",text);
#endif

  /* Find the total width and height */
  if (text)
    sizetext(fd,&w,&h,text);
  if (bit) {
    (*vid->bitmap_getsize)(bit,&bw,&bh);
    w += bw;
    if (bh>h) h = bh;
  }
  if (bit && text)
    w += HWG_MARGIN;

  /* Set split to w or h depending on split orientation */
  if ((self->in->flags & PG_S_TOP) ||
      (self->in->flags & PG_S_BOTTOM))
    self->in->split = h;
  else if ((self->in->flags & PG_S_LEFT) ||
	   (self->in->flags & PG_S_RIGHT))
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



