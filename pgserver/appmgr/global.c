/* $Id: global.c,v 1.7 2000/06/03 17:50:42 micahjd Exp $
 *
 * global.c - Handle allocation and management of objects common to
 * all apps: the clipboard, background widget, default font, and containers.
 * Uses functions in one of the app manager directories.
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

struct app_info *applist;
handle defaultfont;
handle background;
struct widget *bgwidget;
handle hbgwidget;

/* A little pattern for a default background (in PNM format) */
unsigned char bg_bits[] = {
  0x50, 0x36, 0x0A, 0x38, 0x20, 0x38, 0x0A, 0x32, 0x35, 0x35, 
  0x0A, 0xA0, 0xA0, 0xFF, 0xA0, 0xA0, 0xFF, 0xA0, 0xA0, 0xFF, 
  0xA0, 0xA0, 0xFF, 0xA0, 0xA0, 0xFF, 0xA0, 0xA0, 0xFF, 0xA0, 
  0xA0, 0xFF, 0x76, 0x76, 0xD7, 0xA0, 0xA0, 0xFF, 0x76, 0x76, 
  0xD7, 0x76, 0x76, 0xD7, 0x76, 0x76, 0xD7, 0x76, 0x76, 0xD7, 
  0x76, 0x76, 0xD7, 0x62, 0x62, 0xA5, 0x76, 0x76, 0xD7, 0xA0, 
  0xA0, 0xFF, 0x76, 0x76, 0xD7, 0x62, 0x62, 0xA5, 0x62, 0x62, 
  0xA5, 0x62, 0x62, 0xA5, 0x76, 0x76, 0xD7, 0x62, 0x62, 0xA5, 
  0x76, 0x76, 0xD7, 0xA0, 0xA0, 0xFF, 0x76, 0x76, 0xD7, 0x62, 
  0x62, 0xA5, 0x76, 0x76, 0xD7, 0xA0, 0xA0, 0xFF, 0x76, 0x76, 
  0xD7, 0x62, 0x62, 0xA5, 0x76, 0x76, 0xD7, 0xA0, 0xA0, 0xFF, 
  0x76, 0x76, 0xD7, 0x62, 0x62, 0xA5, 0xA0, 0xA0, 0xFF, 0xA0, 
  0xA0, 0xFF, 0x76, 0x76, 0xD7, 0x62, 0x62, 0xA5, 0x76, 0x76, 
  0xD7, 0xA0, 0xA0, 0xFF, 0x76, 0x76, 0xD7, 0x76, 0x76, 0xD7, 
  0x76, 0x76, 0xD7, 0x76, 0x76, 0xD7, 0x76, 0x76, 0xD7, 0x62, 
  0x62, 0xA5, 0x76, 0x76, 0xD7, 0xA0, 0xA0, 0xFF, 0x62, 0x62, 
  0xA5, 0x62, 0x62, 0xA5, 0x62, 0x62, 0xA5, 0x62, 0x62, 0xA5, 
  0x62, 0x62, 0xA5, 0x62, 0x62, 0xA5, 0x76, 0x76, 0xD7, 0x76, 
  0x76, 0xD7, 0x76, 0x76, 0xD7, 0x76, 0x76, 0xD7, 0x76, 0x76, 
  0xD7, 0x76, 0x76, 0xD7, 0x76, 0x76, 0xD7, 0x76, 0x76, 0xD7, 
  0x76, 0x76, 0xD7, 0x0A, 0x0A, 0x0A, 
};
#define bg_len 206

g_error appmgr_init(void) {
  struct bitmap *bgbits;
  struct widget *w;
  g_error e;

  applist = NULL;  /* No apps yet! */

  /* Allocate default font */
  e = findfont(&defaultfont,-1,NULL,0,FSTYLE_DEFAULT);
  if (e.type != ERRT_NONE) return e;

  /* Load the default background */
  e = hwrbit_pnm(&bgbits,bg_bits,bg_len);
  if (e.type != ERRT_NONE) return e;
  e = mkhandle(&background,TYPE_BITMAP,-1,bgbits);
  if (e.type != ERRT_NONE) return e;

  /* Make the background widget */
  e = widget_create(&bgwidget,WIDGET_BITMAP,dts->root,
		    &dts->root->head->next);
  if (e.type != ERRT_NONE) return e;
  e = widget_set(bgwidget,WP_BITMAP,(glob)background);
  if (e.type != ERRT_NONE) return e;
  e = widget_set(bgwidget,WP_ALIGN,A_ALL);
  if (e.type != ERRT_NONE) return e;
  e = widget_set(bgwidget,WP_SIDE,S_ALL);
  if (e.type != ERRT_NONE) return e;
  e = mkhandle(&hbgwidget,TYPE_WIDGET,-1,bgwidget);   
  if (e.type != ERRT_NONE) return e;

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
}

/* Set the background, or NULL to restore it */
g_error appmgr_setbg(int owner,handle bitmap) {
  struct bitmap *bgbits;
  g_error e;

  if (!bitmap) {
    /* Load our default */
    e = hwrbit_pnm(&bgbits,bg_bits,bg_len);
    if (e.type != ERRT_NONE) return e;
    e = mkhandle(&bitmap,TYPE_BITMAP,-1,bgbits);
    if (e.type != ERRT_NONE) return e;
    owner = -1;
  }

  e = handle_bequeath(background,bitmap,owner);
  if (e.type != ERRT_NONE) return e;
  return widget_set(bgwidget,WP_BITMAP,0);
}

g_error appmgr_register(struct app_info *i) {
  struct app_info *dest;
  struct widget *w;
  g_error e;

  /* Allocate root widget, do any setup specific to the app type */
  switch (i->type) {

  case APP_TOOLBAR:
    /* Create a simple toolbar as a root widget */
    e = widget_create(&w,WIDGET_TOOLBAR,dts->root,&dts->root->head->next);
    if (e.type != ERRT_NONE) return e;
    w->isroot = 1;
    e = mkhandle(&i->rootw,TYPE_WIDGET,i->owner,w);
    if (e.type != ERRT_NONE) return e;    

    /* Size specs are ignored for the toolbar.
       They won't be moved by the appmgr, so sidemask has no effect.
       Set the side here, though.
    */
    e = widget_set(w,WP_SIDE,i->side);
    if (e.type != ERRT_NONE) return e;
    
    break;

  case APP_NORMAL:
    /* Use a panel */
    e = widget_create(&w,WIDGET_PANEL,dts->root,&dts->root->head->next);
    if (e.type != ERRT_NONE) return e;
    w->isroot = 1;
    e = mkhandle(&i->rootw,TYPE_WIDGET,i->owner,w);
    if (e.type != ERRT_NONE) return e;    

    /* Set all the properties */
    e = widget_set(w,WP_SIDE,i->side);
    if (e.type != ERRT_NONE) return e;
    
    break;

  default:
    return mkerror(ERRT_BADPARAM,"Unsupported application type");
  }

  /* Copy to a new structure */
  e = g_malloc((void **) &dest,sizeof(struct app_info));
  if (e.type != ERRT_NONE) return e;
  memcpy(dest,i,sizeof(struct app_info));

  /* Insert it */
  dest->next = applist;
  applist = dest;
  
  return sucess;
}

/* The End */

