/* $Id$
 *
 * panel.c - Resizable container with decorations. It uses a panelbar for resizing purposes,
 *           and optionally supplies some standard buttons for the panel.
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

#include <string.h>
#include <pgserver/common.h>
#include <pgserver/widget.h>

struct paneldata {
  /* Container background */
  struct divnode *bg;

  /* Child widgets */
  handle hrotate, hclose, hzoom, hbar, hlabel;

  /* Allow the app to override margin */
  int margin;
  unsigned int margin_override:1;
};
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(paneldata)

/**** Utilities */

/* Create a standard panel button.
 * Note that the app owns these handles, so an app may delete one or more buttons.
 * This means that we _must_ only refer to the buttons by their handle unless of
 * course we're in a callback triggered by that button.
 */
g_error panel_std_button(handle *h, struct widget *self, int thobj, int thobj_on, int thobj_hilight, int exev,
			 int (*callback)(int event, struct widget *from, s32 param, int owner, const u8 *data)) {
  struct widget *w, *bar;
  g_error e;

  e = rdhandle((void **) &bar, PG_TYPE_WIDGET, self->owner, DATA->hbar);
  errorcheck;

  w = NULL;
  e = widget_derive(&w,h,PG_WIDGET_BUTTON,bar,DATA->hbar,PG_DERIVE_INSIDE,self->owner);
  errorcheck;

  w->callback = callback;
  w->callback_owner = self;

  widget_set(w, PG_WP_THOBJ_BUTTON, thobj);
  widget_set(w, PG_WP_THOBJ_BUTTON_ON, thobj_on);
  widget_set(w, PG_WP_THOBJ_BUTTON_HILIGHT, thobj_hilight);
  widget_set(w, PG_WP_THOBJ_BUTTON_ON_NOHILIGHT, thobj_on);
  widget_set(w, PG_WP_EXTDEVENTS, exev);

  return success;
}


/**** Callbacks */

int panel_close_callback(int event, struct widget *from, s32 param, int owner, const u8 *data) {
  struct widget *p;
  p = from->callback_owner;
  if (p && event==PG_WE_ACTIVATE) {

    /* Send a close event from the panel widget */
    post_event(PG_WE_CLOSE, p, 0,0,NULL);
  
  }
  return 1; /* Absorb event */
}

int panel_rotate_callback(int event, struct widget *from, s32 param, int owner, const u8 *data) {
  struct widget *p;
  p = from->callback_owner;
  if (p && event==PG_WE_ACTIVATE) {

    switch (widget_get(p,PG_WP_SIDE)) {
    case PG_S_TOP:    widget_set(p,PG_WP_SIDE,PG_S_RIGHT);  break;
    case PG_S_RIGHT:  widget_set(p,PG_WP_SIDE,PG_S_BOTTOM); break;
    case PG_S_BOTTOM: widget_set(p,PG_WP_SIDE,PG_S_LEFT);   break;
    case PG_S_LEFT:   widget_set(p,PG_WP_SIDE,PG_S_TOP);    break; 
    }

    update(NULL,1);
  }
  return 1; /* Absorb event */
}

int panel_zoom_callback(int event, struct widget *from, s32 param, int owner, const u8 *data) {
  struct widget *p;
  p = from->callback_owner;
  if (p && event==PG_WE_ACTIVATE) {

    /* FIXME: Zoom button goes here! */

  }
  return 1; /* Absorb event */
}

/* Draw the border.fill from our main theme object */
void build_panel_border(struct gropctxt *c, u16 state, struct widget *self) {
  exec_fillstyle(c,DATA->bg->state,PGTH_P_BORDER_FILL);
}

/**** Init */

void panel_resize(struct widget *self) {
  struct widget *bar = NULL;
  rdhandle((void **) &bar, PG_TYPE_WIDGET, self->owner, DATA->hbar);

  if (DATA->margin_override)
    DATA->bg->split = DATA->margin;
  else
    DATA->bg->split = theme_lookup(DATA->bg->state,PGTH_P_MARGIN);
  self->in->div->split = theme_lookup(DATA->bg->state,PGTH_P_BORDER_SIZE);

  /* The minimum setting on the panelbar needs to leave room for the margin
   * on both sides, and the panelbar width itself.
   */
  if (bar)
    widget_set(bar, PG_WP_MINIMUM, widget_get(bar, PG_WP_SIZE) + 
	       (self->in->div->split << 1));
}

g_error panel_install(struct widget *self) {
  struct widget *bar, *title;
  g_error e;

  WIDGET_ALLOC_DATA(paneldata);

  /* This split determines the size of the main panel area */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags &= ~(DIVNODE_SIZE_AUTOSPLIT | DIVNODE_SIZE_RECURSIVE);
  self->in->flags |= PG_S_TOP;

  /* An optional border inside that main panel area */
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags &= ~(DIVNODE_SIZE_AUTOSPLIT | DIVNODE_SIZE_RECURSIVE);
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->build = &build_panel_border;

  /* Create the panelbar widget */
  e = widget_create(&bar,&DATA->hbar,PG_WIDGET_PANELBAR,
		    self->dt,self->container,self->owner);
  errorcheck;
  e = widget_attach(bar,self->dt,&self->in->div->div,0);
  errorcheck;
  e = widget_set(bar,PG_WP_BIND,self->h);
  errorcheck;

  /* This draws the panel background  */
  e = newdiv(bar->out,self);
  errorcheck;
  DATA->bg = *bar->out;
  DATA->bg->flags |= DIVNODE_SPLIT_BORDER;
  DATA->bg->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  DATA->bg->build = &build_bgfill_only;
  DATA->bg->state = PGTH_O_PANEL;

  /* Set up us the container! */
  self->out = &self->in->next;
  self->sub = &DATA->bg->div;

  /* Firstly, create a label widget in the panelbar to present the title */

  title = NULL;
  e = widget_derive(&title,&DATA->hlabel,PG_WIDGET_LABEL,bar,DATA->hbar,
		    PG_DERIVE_INSIDE,self->owner);
  errorcheck;
  widget_set(title,PG_WP_SIDE,PG_S_ALL);
  widget_set(title,PG_WP_THOBJ,PGTH_O_PANELBAR);

  /* Nextly, create the standard buttons for a panel app */

  e = panel_std_button(&DATA->hzoom, self, 
		       PGTH_O_ZOOMBTN,
		       PGTH_O_ZOOMBTN_ON,
		       PGTH_O_ZOOMBTN_HILIGHT,
		       PG_EXEV_TOGGLE,
		       &panel_zoom_callback);
  errorcheck;

  e = panel_std_button(&DATA->hrotate, self, 
		       PGTH_O_ROTATEBTN,
		       PGTH_O_ROTATEBTN_ON,
		       PGTH_O_ROTATEBTN_HILIGHT,
		       0,
		       &panel_rotate_callback);
  errorcheck;

  e = panel_std_button(&DATA->hclose, self, 
		       PGTH_O_CLOSEBTN,
		       PGTH_O_CLOSEBTN_ON,
		       PGTH_O_CLOSEBTN_HILIGHT,
		       0,
		       &panel_close_callback);
  errorcheck;

  /* Make sure we default to our minimum rolled-up size */
  widget_set(self, PG_WP_SIZE, 0);

  return success;
}

/**** Properties */

void panel_remove(struct widget *self) {
  handle_free(self->owner, DATA->hrotate);
  handle_free(self->owner, DATA->hzoom);
  handle_free(self->owner, DATA->hclose);
  handle_free(self->owner, DATA->hlabel);
  handle_free(self->owner, DATA->hbar);
  g_free(DATA);
  r_divnode_free(self->in);
}

g_error panel_set(struct widget *self,int property, glob data) {
  struct widget *w;
  g_error e;
  struct app_info **app;

  switch (property) {

  case PG_WP_SIZE:
    /* Alias 0 to our minimum rolled-up size */
    e = rdhandle((void **) &w, PG_TYPE_WIDGET, self->owner, DATA->hbar);
    errorcheck;
    if (data==0)
      data = w->in->split;
    widget_base_set(self,property,data);
    break;

  case PG_WP_SIDE:
    e = rdhandle((void **) &w, PG_TYPE_WIDGET, self->owner, DATA->hbar);
    errorcheck;
    switch (data) {    /* Invert the side for the panelbar */
    case PG_S_TOP:    widget_set(w,PG_WP_SIDE,PG_S_BOTTOM); break;
    case PG_S_BOTTOM: widget_set(w,PG_WP_SIDE,PG_S_TOP);    break;
    case PG_S_LEFT:   widget_set(w,PG_WP_SIDE,PG_S_RIGHT);  break;
    case PG_S_RIGHT:  widget_set(w,PG_WP_SIDE,PG_S_LEFT);   break;
    }
    return mkerror(ERRT_PASS,0);

  case PG_WP_THOBJ:
    DATA->bg->state = data;
    resizewidget(self);
    self->in->flags |= DIVNODE_NEED_RECALC;
    self->dt->flags |= DIVTREE_NEED_RECALC;
    break;
    
  case PG_WP_TEXT:
    e = rdhandle((void **) &w, PG_TYPE_WIDGET, self->owner, DATA->hlabel);
    errorcheck;
    app = appmgr_findapp(self);
    if (app && *app)
      (*app)->name = data;
    return widget_set(w,property,data);

  case PG_WP_IMAGE:
    e = rdhandle((void **) &w, PG_TYPE_WIDGET, self->owner, DATA->hlabel);
    errorcheck;
    return widget_set(w,property,data);

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

glob panel_get(struct widget *self,int property) {
  struct widget *w;
  g_error e;

  switch (property) {

  case PG_WP_THOBJ:
    return DATA->bg->state;

  case PG_WP_TEXT:
    e = rdhandle((void **) &w, PG_TYPE_WIDGET, self->owner, DATA->hlabel);
    errorcheck;
    return widget_get(w,property);

  case PG_WP_IMAGE:
    e = rdhandle((void **) &w, PG_TYPE_WIDGET, self->owner, DATA->hlabel);
    errorcheck;
    return widget_get(w,property);

  case PG_WP_PANELBAR:
    return DATA->hbar;

  case PG_WP_PANELBAR_LABEL:
    return DATA->hlabel;

  case PG_WP_PANELBAR_CLOSE:
    return DATA->hclose;

  case PG_WP_PANELBAR_ROTATE:
    return DATA->hrotate;

  case PG_WP_PANELBAR_ZOOM:
    return DATA->hzoom;

  case PG_WP_MARGIN:
    return DATA->margin;

  }
  return widget_base_get(self,property);
}

/* The End */




