/* $Id$
 *
 * managedwindow.c - A root widget representing a window managed by a host GUI
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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

struct managedwindowdata {
  struct divtree *my_dt;
  handle text;            /* Stored handle to the text, so we can get PG_WP_TEXT later */
  handle icon;

  /* These values are used to detect whether the window should automatically resize */
  int last_w, last_h;
  unsigned int already_sized : 1;

  /* Allow the app to override margin */
  int margin;
  unsigned int margin_override:1;

};
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(managedwindowdata)


g_error managedwindow_install(struct widget *self) {
  g_error e;

  WIDGET_ALLOC_DATA(managedwindowdata);

  /* New divtree. This will cause the new window to be created in the host GUI */
  e = dts_push();
  errorcheck;
  DATA->my_dt = dts->top;
  
  /* Take up the entire size of the divtree root */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= DIVNODE_SPLIT_BORDER;
  self->in->build = &build_bgfill_only;
  self->in->state = PGTH_O_MANAGEDWINDOW;

  self->out = &self->in->next;
  self->sub = &self->in->div;

  /* Attach ourselves as a root widget in the new divtree */
  e = widget_attach(self,DATA->my_dt,&DATA->my_dt->head->next,0);  
  errorcheck;
  self->isroot = 1;

  self->trigger_mask = PG_TRIGGER_CLOSE | PG_TRIGGER_KEYUP | PG_TRIGGER_KEYDOWN |
    PG_TRIGGER_CHAR | PG_TRIGGER_DOWN;

  return success;
}

void managedwindow_remove(struct widget *self) {
  r_divnode_free(self->in);
  dts_pop(DATA->my_dt);
  g_free(DATA);
}

g_error managedwindow_set(struct widget *self,int property, glob data) {
  hwrbitmap bit;
  struct pgstring *str;
  s16 x,y;

  /* All of these pass through to the video driver's implementation */
  switch (property) {
    
  case PG_WP_IMAGE:
    if (iserror(rdhandle((void **)&bit,PG_TYPE_BITMAP,-1,data)))
       return mkerror(PG_ERRT_HANDLE,33);
     
    DATA->icon = handle_canonicalize((handle) data);
    break;

  case PG_WP_TEXT:
    if (iserror(rdhandle((void **)&str,PG_TYPE_PGSTRING,self->owner,data))) 
      return mkerror(PG_ERRT_HANDLE,13);
    DATA->text = data;
    VID(window_set_title)(self->dt->display,str);
    break;

    /* FIXME: Combine the x and y calls, so the window won't move 
     *        to an incorrect intermediate position when setting both.
     */

  case PG_WP_ABSOLUTEX:
    /* If this is a menu, give it a menu theme and get the
     * window manager to leave it alone.
     */
    if (data==PG_POPUP_ATCURSOR || data==PG_POPUP_ATEVENT) {
      struct widget *snap;
      struct conbuf *cb;
      s16 winx, winy;

      self->in->state = PGTH_O_POPUP_MENU;
      self->in->split = theme_lookup(self->in->state,PGTH_P_MARGIN);
      VID(window_set_flags)(self->dt->display, PG_WINDOW_UNMANAGED | PG_WINDOW_GRAB);

      /* Pop it up at the cursor? */
      if (data==PG_POPUP_ATCURSOR) {
	struct cursor *c;
	c = cursor_get_default();
	if ((!c) || iserror(rdhandle((void**)&snap,
				     PG_TYPE_WIDGET, -1, c->ctx.widget_last_clicked)))
	  snap = NULL;
      }
      /* Or at the last event? */
      else if (data==PG_POPUP_ATEVENT) {
	cb = find_conbuf(self->owner);
	if ((!cb) || iserror(rdhandle((void**)&snap, 
				      PG_TYPE_WIDGET, -1, cb->lastevent_from)))
	  snap = NULL;
      }

      if (snap && snap->type == PG_WIDGET_BUTTON) {
	/* snap to a button edge */
	VID(window_get_position)(snap->dt->display,&winx,&winy);

	x = snap->in->div->r.x + winx;
	y = snap->in->div->r.y + snap->in->div->r.h + self->in->split + winy;

	/* Flip over if near the bottom */
	if ((y+self->in->child.h)>=vid->yres)
	  y = snap->in->div->r.y - self->in->child.h - self->in->split + winy;
      }
      else if (snap && snap->type == PG_WIDGET_MENUITEM) {
	/* snap to a menuitem edge */
	x = snap->in->div->r.x + snap->in->div->r.w;
	y = snap->in->div->r.y;
      }
      else {
	/* exactly at the cursor */
	int ix,iy;
	cursor_getposition(NULL,&ix,&iy,NULL);
	x = ix;
	y + iy;
      }       

      /* Account for the margin */
      x -= self->in->split;
      y -= self->in->split;

      VID(window_set_position)(self->dt->display, x, y);
    }
    else {
      VID(window_get_position)(self->dt->display, &x, &y);
      VID(window_set_position)(self->dt->display, data, y);
    }
    break;

  case PG_WP_ABSOLUTEY:
    if (!(data==PG_POPUP_ATCURSOR || data==PG_POPUP_ATEVENT)) {
      VID(window_get_position)(self->dt->display, &x, &y);
      VID(window_set_position)(self->dt->display, x, data);
    }
    break;

  case PG_WP_WIDTH:
    DATA->already_sized = 1;
    VID(window_get_size)(self->dt->display, &x, &y);
    VID(window_set_size)(self->dt->display, data, y);
    break;

  case PG_WP_HEIGHT:
    DATA->already_sized = 1;
    VID(window_get_size)(self->dt->display, &x, &y);
    VID(window_set_size)(self->dt->display, x, data);
    break;

  case PG_WP_MARGIN:
    DATA->margin = data;
    DATA->margin_override = data >= 0;
    resizewidget(self);
    break;

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob managedwindow_get(struct widget *self,int property) {
  s16 x,y;

  switch (property) {

  case PG_WP_IMAGE:
    return (glob) DATA->icon;

  case PG_WP_TEXT:
    return DATA->text;

  case PG_WP_ABSOLUTEX:
    VID(window_get_position)(self->dt->display, &x, &y);
    return x;

  case PG_WP_ABSOLUTEY:
    VID(window_get_position)(self->dt->display, &x, &y);
    return y;

  case PG_WP_WIDTH:
    VID(window_get_size)(self->dt->display, &x, &y);
    return x;

  case PG_WP_HEIGHT:
    VID(window_get_size)(self->dt->display, &x, &y);
    return y;

  case PG_WP_MARGIN:
    return DATA->margin;

  }
  return widget_base_get(self,property);
}

void managedwindow_resize(struct widget *self) {
  s16 w,h,maxw,maxh;
  VID(window_get_size)(DATA->my_dt->display,&w,&h);
  
  if (DATA->margin_override)
    self->in->split = DATA->margin;
  else
    self->in->split = theme_lookup(self->in->state,PGTH_P_MARGIN);

  /* Detect whether the size has been changed by some
   * external force (the user probably) since the last time
   */
  if (self->in->div && self->in->div->child.w && self->in->div->child.h && 
      ((!DATA->already_sized) || (w==DATA->last_w && h==DATA->last_h))) {
    w = self->in->child.w;
    h = self->in->child.h;

    /* Make sure the window size is reasonable */
    maxw = vid->lxres * 80/100;
    maxh = vid->lyres * 80/100;
    if (w < 10)
      w = 10;
    if (h < 10)
      h = 10;
    if (w > maxw)
      w = maxw;
    if (h > maxh)
      h = maxh;

    VID(window_set_size)(DATA->my_dt->display,w,h);
    DATA->already_sized = 1;
    DATA->last_w = w;
    DATA->last_h = h;
  }
}

void managedwindow_trigger(struct widget *self,s32 type,union trigparam *param) {
  switch (type) {

  case PG_TRIGGER_CLOSE:
    post_event(PG_WE_CLOSE, self, 0,0,NULL);
    break;

  }
}

/* The End */


