/* $Id$
 *
 * dialogbox.c - The dialogbox is a type of popup widget that is always
 *               automatically sized, and has a title
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

struct dialogboxdata {
  struct divnode **title_location;   /* Insertion point for adding the title */
  struct widget *title;
  handle htitle;
};
#define WIDGET_SUBCLASS 1
#define DATA WIDGET_DATA(dialogboxdata)

g_error dialogbox_install(struct widget *self) {
  g_error e;
  struct divnode *interior;

  WIDGET_INSTALL_PARENT(PG_WIDGET_POPUP);
  WIDGET_ALLOC_DATA(dialogboxdata);

  DATA->title_location = self->sub;

  /* Make a divnode after the title to act as an insertion point for child widgets */
  e = newdiv(self->sub, self);
  errorcheck;
  interior = *self->sub;
  interior->flags |= PG_S_ALL;
  self->sub = &interior->div;

  return success;
}

void dialogbox_remove(struct widget *self) {
  if (DATA->title)
    handle_free(self->owner, DATA->htitle);
  g_free(DATA);

  /* FIXME: This is a kludge provied by sanit that fixes a memory leak */
  /* NOTE: This has been disabled, since it will sometimes create a cyclic divtree
   *       and therefore infinite recursion.
   *
   if (((self->in) && (self->in->div)) && !(self->in->div->div) && (self->sub))
   self->in->div->div = self->sub;
   */

  WIDGET_REMOVE_PARENT;
}

void dialogbox_resize(struct widget *self) {
  WIDGET_PARENT->resize(self);
}

g_error dialogbox_set(struct widget *self,int property, glob data) {
  g_error e;

  switch (property) {
  
  case PG_WP_TEXT:
    /* If possible, get the popup widget to set the title */
    if (iserror(WIDGET_PARENT->set(self,property,data))) {
      /* Otherwise, we'll use our own title widget */
      
      /* Need to create the title? */
      if (!DATA->title) {
	e = widget_create(&DATA->title, &DATA->htitle, PG_WIDGET_LABEL,
			  self->dt, self->container, self->owner);
	errorcheck;
	e = widget_attach(DATA->title, self->dt, DATA->title_location, self->h);
	errorcheck;
	e = widget_set(DATA->title, PG_WP_SIDE, PG_S_TOP);
	errorcheck;
	e = widget_set(DATA->title, PG_WP_TRANSPARENT, 0);
	errorcheck;
	e = widget_set(DATA->title, PG_WP_THOBJ, PGTH_O_LABEL_DLGTITLE);
	errorcheck;
      }			
      
      /* Set the title */
      e = widget_set(DATA->title, PG_WP_TEXT, data);
    }
    break;

  default:
    return WIDGET_PARENT->set(self,property,data);
  }
  return success;
}

glob dialogbox_get(struct widget *self,int property) {
  return WIDGET_PARENT->get(self,property);
}

void dialogbox_trigger(struct widget *self,s32 type,union trigparam *param) {
  WIDGET_PARENT->trigger(self,type,param);
}

/* The End */




