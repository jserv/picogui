/* $Id: managedwindow.c,v 1.6 2002/11/04 14:05:13 micahjd Exp $
 *
 * managedwindow.c - A root widget representing a window managed by a host GUI
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

#include <pgserver/common.h>
#include <pgserver/widget.h>

struct managedwindowdata {
  struct divtree *my_dt;
  handle text;            /* Stored handle to the text, so we can get PG_WP_TEXT later */

  unsigned int already_sized : 1;
};
#define DATA WIDGET_DATA(0,managedwindowdata)


g_error managedwindow_install(struct widget *self) {
  g_error e;

  WIDGET_ALLOC_DATA(0,managedwindowdata)

  /* New divtree. This will cause the new window to be created in the host GUI */
  e = dts_push();
  errorcheck;
  DATA->my_dt = dts->top;
  
  /* Take up the entire size of the divtree root */
  e = newdiv(&self->in,self);
  errorcheck;
  self->in->build = &build_bgfill_only;
  self->in->state = PGTH_O_MANAGEDWINDOW;

  self->out = &self->in->next;
  self->sub = &self->in->div;
  
  /* Attach ourselves as a root widget in the new divtree */
  e = widget_attach(self,DATA->my_dt,&DATA->my_dt->head->next,0);  
  errorcheck;
  self->isroot = 1;

  return success;
}

void managedwindow_remove(struct widget *self) {
  r_divnode_free(self->in);
  dts_pop(DATA->my_dt);
  g_free(DATA);
}

g_error managedwindow_set(struct widget *self,int property, glob data) {
  struct pgstring *str;
  s16 x,y;

  /* All of these pass through to the video driver's implementation */
  switch (property) {
    
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
    VID(window_get_position)(self->dt->display, &x, &y);
    VID(window_set_position)(self->dt->display, data, y);
    break;

  case PG_WP_ABSOLUTEY:
    VID(window_get_position)(self->dt->display, &x, &y);
    VID(window_set_position)(self->dt->display, x, data);
    break;

  case PG_WP_WIDTH:
    VID(window_get_size)(self->dt->display, &x, &y);
    VID(window_set_size)(self->dt->display, data, y);
    break;

  case PG_WP_HEIGHT:
    VID(window_get_size)(self->dt->display, &x, &y);
    VID(window_set_size)(self->dt->display, x, data);
    break;

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob managedwindow_get(struct widget *self,int property) {
  s16 x,y;

  switch (property) {

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

  }
  return widget_base_get(self,property);
}

void managedwindow_resize(struct widget *self) {
  if (self->in->child.w && self->in->child.h && !DATA->already_sized) {
    s16 w = self->in->child.w;
    s16 h = self->in->child.h;

    /* Make sure the window size is reasonable */
    if (w < 8)
      w = 10;
    if (h < 8)
      h = 10;
    if (w > vid->lxres)
      w = vid->lxres;
    if (h > vid->lyres)
      h = vid->lyres;

    VID(window_set_size)(DATA->my_dt->display,w,h);
    DATA->already_sized = 1;
  }
}

/* The End */


