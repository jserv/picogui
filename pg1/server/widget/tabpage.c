/* $Id$
 *
 * tabpage.c - A page in a tabbed book. It can be inserted into any
 *             container and automatically link with other tab pages
 *             in that container.
 *
 *      Implementation summary:
 *       
 *      The tab has several components: the page itself, the bar
 *      containing the tabs, and the tabs themselves. This
 *      widget subclasses the box widget and changes its theme, and
 *      manages automatically creating the tab and bar as necessary.
 *      The tab is a rethemed button widget, and the bar is a rethemed
 *      toolbar widget.
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

struct tabpagedata {
  handle htab, htab_bar;
};
#define WIDGET_SUBCLASS 1
#define DATA WIDGET_DATA(tabpagedata)

void tabpage_resize(struct widget *self) {
  WIDGET_PARENT->resize(self);
}

static void tabpage_show_hide(struct widget *self, int visible) {
  if (visible) {
    widget_base_set(self, PG_WP_SIDE, PG_S_ALL);
  }
  else {
    widget_base_set(self, PG_WP_SIDE, PG_S_TOP);
    widget_base_set(self, PG_WP_SIZE, 0);
  }
}

/* Called when our tab button is selected or deselected */
static int tabpage_tab_callback(int event, struct widget *from, s32 param, int owner, const u8 *data) {
  tabpage_show_hide(from->callback_owner, widget_get(from, PG_WP_ON));
  post_event(PG_WE_ACTIVATE,from->callback_owner,0,0,NULL);
}

g_error tabpage_install(struct widget *self) {
  g_error e;
  struct widget *tab;

  WIDGET_INSTALL_PARENT(PG_WIDGET_BOX);
  WIDGET_ALLOC_DATA(tabpagedata);

  /* Set custom defaults */
  e = widget_base_set(self, PG_WP_THOBJ, PGTH_O_TAB_PAGE);
  errorcheck;
  e = widget_base_set(self, PG_WP_SIZE, 0);
  errorcheck;

  /* Go ahead and create the tab. We will attach the tab
   * later when we're attached ourselves, but create it early so the
   * properties can be set.
   */
  e = widget_create(&tab, &DATA->htab, PG_WIDGET_BUTTON,
		    self->dt, self->container, self->owner);
  errorcheck;
  e = widget_set(tab, PG_WP_EXTDEVENTS, PG_EXEV_TOGGLE | PG_EXEV_EXCLUSIVE);
  errorcheck;
  e = widget_set(tab, PG_WP_THOBJ_BUTTON, PGTH_O_TAB);
  errorcheck;
  e = widget_set(tab, PG_WP_THOBJ_BUTTON_HILIGHT, PGTH_O_TAB_HILIGHT);
  errorcheck;
  e = widget_set(tab, PG_WP_THOBJ_BUTTON_ON, PGTH_O_TAB_ON);
  errorcheck;
  e = widget_set(tab, PG_WP_THOBJ_BUTTON_ON_NOHILIGHT, PGTH_O_TAB_ON_NOHILIGHT);
  errorcheck;

  tab->callback = &tabpage_tab_callback;
  tab->callback_owner = self;

  return success;
}

void tabpage_remove(struct widget *self) {
  handle_free(self->owner, DATA->htab);
  g_free(DATA);
  WIDGET_REMOVE_PARENT;
}

/* Return true if this property should be handled
 * by the tab, not by the page.
 */
static int is_tab_property(int property) {
  switch (property) {
  case PG_WP_TEXT:
  case PG_WP_IMAGE:
  case PG_WP_BITMASK:
  case PG_WP_IMAGESIDE:
  case PG_WP_FONT:
  case PG_WP_DISABLED:
  case PG_WP_HOTKEY_FLAGS:
  case PG_WP_HOTKEY_CONSUME:
  case PG_WP_HOTKEY_MODIFIERS:
  case PG_WP_HOTKEY:
  case PG_WP_ALIGN:
  case PG_WP_COLOR:
  case PG_WP_LGOP:
  case PG_WP_ON:
    return 1;
  }
  return 0;
}

/* Return true if this property should be handled
 * by the tab bar
 */
static int is_tab_bar_property(int property) {
  switch (property) {
  case PG_WP_SIDE:
  case PG_WP_SIZE:
  case PG_WP_SIZEMODE:
    return 1;
  }
  return 0;
}

/* This is called whenever the widget is attached, after the attaching
 * process is complete. We use this as a hook for managing the tab and tab_bar.
 */
g_error tabpage_post_attach(struct widget *self, struct widget *parent, int rship) {
  struct widget *tab, *tab_bar, *parent_tab;
  g_error e;
  handle existing_bar = 0;

  /* Dereference handles */
  e = rdhandle((void**)&tab, PG_TYPE_WIDGET, self->owner, DATA->htab);
  errorcheck;
  e = rdhandle((void**)&tab_bar, PG_TYPE_WIDGET, self->owner, DATA->htab_bar);
  errorcheck;

  /* Detach our tab. It will be reattached later if necessary */
  e = widget_derive(&tab, &DATA->htab, tab->type, NULL, 0, 0, self->owner);
  errorcheck;

  /* If we already have a tab bar but it's empty, delete it */
  if (DATA->htab_bar && !widget_traverse(tab_bar, PG_TRAVERSE_CHILDREN, 0)) {
    handle_free(self->owner, DATA->htab_bar);
    DATA->htab_bar = 0;
  }

  /* Are we being attached rather than detached? */
  if (parent) {

    /* If we're attaching before or after another tab page, share its tab bar */
    if (parent->type==PG_WIDGET_TABPAGE &&
	(rship==PG_DERIVE_BEFORE || rship==PG_DERIVE_AFTER)) {
      struct widget *self = parent;
      existing_bar = DATA->htab_bar;
    }
    DATA->htab_bar = existing_bar;

    /* Otherwise, create a new tab bar */
    if (!DATA->htab_bar) {
      tab_bar = NULL;
      e = widget_derive(&tab_bar, &DATA->htab_bar, PG_WIDGET_TOOLBAR,
			self, self->h, PG_DERIVE_BEFORE, self->owner);
      errorcheck;
      e = widget_set(tab_bar, PG_WP_THOBJ, PGTH_O_TAB_BAR);
      errorcheck;
      tab_bar->auto_orientation = PG_AUTO_SIDE;
    }

    /* If we're attaching on an existing bar, attach the tab in the same
     * relative order as the tab pages themselves.
     */
    parent_tab = NULL;
    rdhandle((void**)&parent_tab, PG_TYPE_WIDGET, self->owner, widget_get(parent, PG_WP_TAB));
    if (existing_bar && parent_tab) {
      e = widget_derive(&tab, &DATA->htab, tab->type,
			parent_tab, parent_tab->h, rship, self->owner);
      errorcheck;
    }
    /* Otherwise just put it in our tab bar directly */
    else {
      e = widget_derive(&tab, &DATA->htab, tab->type,
			tab_bar, tab_bar->h, PG_DERIVE_INSIDE, self->owner);
      errorcheck;
    }      
		
    /* If we were here first, make ourselves active */
    if (!existing_bar) {
      e = widget_set(self, PG_WP_ON, 1);
      errorcheck;
    }
  }

  return success;
}

g_error tabpage_set(struct widget *self,int property, glob data) {
  struct widget *w;
  g_error e;

  /* Redirect applicable properties to the tab */ 
  if (is_tab_property(property))
    if (!iserror(rdhandle((void**)&w, PG_TYPE_WIDGET, self->owner, DATA->htab)) && w)
      return widget_set(w, property, data);

  /* Redirect applicable properties to the tab bar */ 
  if (is_tab_bar_property(property))
    if (!iserror(rdhandle((void**)&w, PG_TYPE_WIDGET, self->owner, DATA->htab_bar)) && w)
      return widget_set(w, property, data);

  return WIDGET_PARENT->set(self,property,data);
}

glob tabpage_get(struct widget *self,int property) {
  struct widget *w;

  /* Redirect applicable properties to the tab */ 
  if (is_tab_property(property))
    if (!iserror(rdhandle((void**)&w, PG_TYPE_WIDGET, self->owner, DATA->htab)) && w)
      return widget_get(w, property);

  /* Redirect applicable properties to the tab bar */ 
  if (is_tab_bar_property(property))
    if (!iserror(rdhandle((void**)&w, PG_TYPE_WIDGET, self->owner, DATA->htab_bar)) && w)
      return widget_get(w, property);

  switch (property) {

  case PG_WP_TAB:
    return DATA->htab;

  case PG_WP_TAB_BAR:
    return DATA->htab_bar;

  }
  return WIDGET_PARENT->get(self,property);
}

/* The End */




