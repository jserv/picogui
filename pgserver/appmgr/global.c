/* $Id: global.c,v 1.22 2000/10/29 01:45:32 micahjd Exp $
 *
 * global.c - Handle allocation and management of objects common to
 * all apps: the clipboard, background widget, default font, and containers.
 * Uses functions in one of the app manager directories.
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
#include <pgserver/divtree.h>
#include <pgserver/font.h>
#include <pgserver/g_malloc.h>
#include <pgserver/appmgr.h>

#include "defaultcursor.inc"

struct app_info *applist;
handle defaultfont;
struct widget *bgwidget;
handle hbgwidget;
struct sprite *pointer;

g_error appmgr_init(void) {
  hwrbitmap bgbits;
  struct widget *w;
  g_error e;

  applist = NULL;  /* No apps yet! */

  /* Allocate default font */
  e = findfont(&defaultfont,-1,NULL,0,PG_FSTYLE_DEFAULT);
  errorcheck;

  /* Make the background widget */
  e = widget_create(&bgwidget,PG_WIDGET_BACKGROUND,dts->root,
		    &dts->root->head->next,0,-1);
  errorcheck;
  e = mkhandle(&hbgwidget,PG_TYPE_WIDGET,-1,bgwidget);   
  errorcheck;

  /* Create the pointer sprite */
  e = new_sprite(&pointer,19,22);
  errorcheck;
  e = (*vid->bitmap_loadpnm)(&pointer->bitmap,cursor_bits,cursor_len);
  errorcheck;
  e = (*vid->bitmap_loadpnm)(&pointer->mask,cursor_mask_bits,cursor_mask_len);
  errorcheck;

  return sucess;
}

/* Most of it is cleaned up by the handles.
   Here we free the linked list */
void appmgr_free(void) {
  struct app_info *n,*condemn;
  n = applist;
  while (n) {
    condemn = n;
    n = n->next;
    g_free(condemn);
  }
  
  /* Free the mouse pointer */
  free_sprite(pointer);
  pointer = NULL;
}

/* Unregisters applications owned by a given connection */
void appmgr_unregowner(int owner) {
  struct app_info *n,*condemn;
  condemn = NULL;
  if (!applist) return;
  if (applist->owner==owner) {
    condemn = applist;
    applist = applist->next;
  }
  else {
    n = applist;
    while (n->next) {
      if (n->next->owner==owner) {
	condemn = n->next;
	n->next = n->next->next;
	break;
      }
      else
	n = n->next;
    }  
  }
  g_free(condemn);
}

g_error appmgr_register(struct app_info *i) {
  struct app_info *dest;
  struct widget *w;
  g_error e;

  /* Allocate root widget, do any setup specific to the app type */
  switch (i->type) {

  case PG_APP_TOOLBAR:
    /* Create a simple toolbar as a root widget */
    e = widget_create(&w,PG_WIDGET_TOOLBAR,dts->root,&dts->root->head->next,0,i->owner);
    errorcheck;
    e = mkhandle(&i->rootw,PG_TYPE_WIDGET,i->owner,w);
    errorcheck;    

    /* Size specs are ignored for the toolbar.
       They won't be moved by the appmgr, so sidemask has no effect.
       Set the side here, though.
    */
    e = widget_set(w,PG_WP_SIDE,i->side);
    errorcheck;
    
    break;

  case PG_APP_NORMAL:
    /* Use a panel */
    e = widget_create(&w,PG_WIDGET_PANEL,dts->root,&dts->root->head->next,0,i->owner);
    errorcheck;
    e = mkhandle(&i->rootw,PG_TYPE_WIDGET,i->owner,w);
    errorcheck;    

    /* Set all the properties */
    e = widget_set(w,PG_WP_SIDE,i->side);
    errorcheck;
    e = widget_set(w,PG_WP_SIZE,(i->side & (PG_S_LEFT|PG_S_RIGHT)) ? i->w : i->h);
    errorcheck;
    
    break;

  default:
    return mkerror(PG_ERRT_BADPARAM,30);
  }

  w->isroot = 1;

  /* Copy to a new structure */
  e = g_malloc((void **) &dest,sizeof(struct app_info));
  errorcheck;
  memcpy(dest,i,sizeof(struct app_info));

  /* Insert it */
  dest->next = applist;
  applist = dest;

  return sucess;
}

/* The End */


