/* $Id: panel.c,v 1.79 2002/07/28 17:06:49 micahjd Exp $
 *
 * panel.c - Holder for applications. It uses a panelbar for resizing purposes,
 *           and optionally supplies some standard buttons for the panel.
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

struct paneldata {
  /* Container background */
  struct divnode *bg;

  /* Child widgets */
  handle hrotate, hclose, hzoom, hbar, hlabel;
};
#define DATA ((struct paneldata *)(self->data))

/**** Utilities */

/* Create a standard panel button.
 * Note that the app owns these handles, so an app may delete one or more buttons.
 * This means that we _must_ only refer to the buttons by their handle unless of
 * course we're in a callback triggered by that button.
 */
g_error panel_std_button(handle *h, struct widget *self, int thobj, int thobj_on, int thobj_hilight, int exev,
			 int (*callback)(int event, struct widget *from, s32 param, int owner, char *data)) {
  struct widget *w, *bar;
  g_error e;

  e = rdhandle((void **) &bar, PG_TYPE_WIDGET, self->owner, DATA->hbar);
  errorcheck;

  w = NULL;
  e = widget_derive(&w,PG_WIDGET_BUTTON,bar,DATA->hbar,PG_DERIVE_INSIDE,self->owner);
  errorcheck;
  e = mkhandle(h,PG_TYPE_WIDGET,self->owner,w);
  errorcheck;

  w->callback = callback;
  
  widget_set(w, PG_WP_THOBJ_BUTTON, thobj);
  widget_set(w, PG_WP_THOBJ_BUTTON_ON, thobj_on);
  widget_set(w, PG_WP_THOBJ_BUTTON_HILIGHT, thobj_hilight);
  widget_set(w, PG_WP_THOBJ_BUTTON_ON_NOHILIGHT, thobj_on);
  widget_set(w, PG_WP_EXTDEVENTS, exev);

  return success;
}

/* Used in the callbacks to get a pointer to the panel widget, given a pointer to
 * the button. Returns NULL on failure.
 */
struct widget *panel_getpanel(struct widget *button) {
  struct divnode *d;
  struct widget *bar;

  /* The button's container is the panelbar. The panelbar's container should be
   * the panel, but this isn't possible because at the time the panelbar is created
   * the panel doesn't yet have a handle. So, we take advantage of the fact that the
   * panel is the panelbar's parent in the divtree.
   *
   * We might change this later to use payloads or some other method to locate the 
   * panel. Imagine an app being able to reattach its own rotate button somewhere else :)
   */
  
  bar = widget_traverse(button, PG_TRAVERSE_CONTAINER, 1);
  if (!bar)
    return NULL;

  d = divnode_findparent(bar->dt->head, bar->in);
  if (!d)
    return NULL;
  return d->owner;
}

/**** Callbacks */

int panel_close_callback(int event, struct widget *from, s32 param, int owner, char *data) {
  struct widget *p;
  p = panel_getpanel(from);
  if (p && event==PG_WE_ACTIVATE) {

    /* Send a close event from the panel widget */
    post_event(PG_WE_CLOSE, p, 0,0,NULL);
  
  }
  return 1; /* Absorb event */
}

int panel_rotate_callback(int event, struct widget *from, s32 param, int owner, char *data) {
  struct widget *p;
  p = panel_getpanel(from);
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

int panel_zoom_callback(int event, struct widget *from, s32 param, int owner, char *data) {
  struct widget *p;
  p = panel_getpanel(from);
  if (p && event==PG_WE_ACTIVATE) {

    /* FIXME: Zoom button goes here! */

  }
  return 1; /* Absorb event */
}
  
/**** Init */

void panel_resize(struct widget *self) {
  DATA->bg->split = theme_lookup(DATA->bg->state,PGTH_P_MARGIN);
}

g_error panel_install(struct widget *self) {
  struct widget *bar, *title;
  g_error e;

  /* Allocate data structure */
  e = g_malloc(&self->data,sizeof(struct paneldata));
  errorcheck;
  memset(self->data,0,sizeof(struct paneldata));

  /* This split determines the size of the main panel area */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags &= ~(DIVNODE_SIZE_AUTOSPLIT | DIVNODE_SIZE_RECURSIVE);
  self->in->flags |= PG_S_TOP;

  /* Create the panelbar widget */
  e = widget_create(&bar,PG_WIDGET_PANELBAR,
		    self->dt,self->container,self->owner);
  errorcheck;
  e = widget_attach(bar,self->dt,&self->in->div,0,self->owner);
  errorcheck;
  e = mkhandle(&DATA->hbar,PG_TYPE_WIDGET,self->owner,bar);
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
  e = widget_derive(&title,PG_WIDGET_LABEL,bar,DATA->hbar,
		    PG_DERIVE_INSIDE,self->owner);
  errorcheck;
  e = mkhandle(&DATA->hlabel,PG_TYPE_WIDGET,self->owner,title);
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

  return success;
}

/**** Properties */

void panel_remove(struct widget *self) {
  handle_free(-1, DATA->hbar);
  g_free(self->data);
  r_divnode_free(self->in);
}

g_error panel_set(struct widget *self,int property, glob data) {
  struct widget *w;
  g_error e;
  struct app_info **app;

  switch (property) {

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

  }
  return 0;
}

/* The End */




