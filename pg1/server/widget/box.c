/* $Id$
 *
 * box.c - Generic container for holding a group of widgets. It's sizing and
 *         appearance are defined by the theme.
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

struct boxdata {
  int margin_override;
};
#define WIDGET_SUBCLASS 0
#define DATA WIDGET_DATA(boxdata)

void box_resize(struct widget *self) {
   int m;
   
   if (!DATA->margin_override) {

     /* Transparent boxen have the same margin as a button
      * (necessary for grids to look good) */
     if (self->in->div->build)
       m = theme_lookup(self->in->div->state,PGTH_P_MARGIN);
     else
       m = theme_lookup(PGTH_O_BUTTON,PGTH_P_SPACING) >> 1;
     
     self->in->div->split = m;
   }
}

g_error box_install(struct widget *self) {
  g_error e;

  WIDGET_ALLOC_DATA(boxdata);

  e = newdiv(&self->in,self);
  errorcheck;
  self->in->flags |= PG_S_TOP;
  e = newdiv(&self->in->div,self);
  errorcheck;
  self->in->div->flags |= DIVNODE_SPLIT_BORDER;
  self->in->div->flags &= ~DIVNODE_SIZE_AUTOSPLIT;
  self->in->div->build = &build_bgfill_only;
  self->in->div->state = PGTH_O_BOX;

  DATA->margin_override = 0;
  
  self->out = &self->in->next;
  self->sub = &self->in->div->div;

  return success;
}

void box_remove(struct widget *self) {
  r_divnode_free(self->in);
  g_free(DATA);
}

g_error box_set(struct widget *self,int property, glob data) {
  switch (property) {
    
  case PG_WP_TRANSPARENT:
    self->in->div->build = data ? NULL : (&build_bgfill_only);
    break;	
    
  case PG_WP_MARGIN:
    self->in->div->split = data;
    DATA->margin_override = 1;       /* Prevent automatic setting of margins */
    break;
    
  case PG_WP_HILIGHTED:
    if ( data ) {
      
      //
      //
      // Hilight this widget
      //
      widget_set(self, PG_WP_THOBJ, PGTH_O_BOX_HILIGHT);
    }
    else {
      
      // 
      // Un hilight this widget
      //
      widget_set(self, PG_WP_THOBJ, PGTH_O_BOX);
    }
    
    
    /* Let the default handler pass it on */
    return mkerror(ERRT_PASS,0);

  default:
    return mkerror(ERRT_PASS,0);
  }
  return success;
}

glob box_get(struct widget *self,int property) {
  switch (property) {

  case PG_WP_MARGIN:
    return self->in->div->split;

  }
  return widget_base_get(self,property);
}

/* The End */




