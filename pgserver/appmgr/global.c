/* $Id: global.c,v 1.32 2001/03/01 02:23:10 micahjd Exp $
 *
 * global.c - Handle allocation and management of objects common to
 * all apps: the clipboard, background widget, default font, and containers.
 * Uses functions in one of the app manager directories.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/divtree.h>
#include <pgserver/font.h>
#include <pgserver/appmgr.h>

/*** Simple arrow cursor in XBM format */

#define cursor_width 8
#define cursor_height 14
unsigned char const cursor_bits[] = {
0x01,0x03,0x05,0x09,0x11,0x21,0x41,0x81,0xC1,0x21,0x2D,0x4B,0x50,0x70
};
unsigned char const cursor_mask_bits[] = {
0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF,0xFF,0x3F,0x3F,0x7B,0x70,0x70
};


struct app_info *applist;
handle defaultfont;
struct widget *bgwidget;
handle hbgwidget;
struct sprite *cursor;
hwrbitmap defaultcursor_bitmap,defaultcursor_bitmask;
handle string_ok,string_cancel,string_yes,string_no;
handle htbboundary;       /* The last toolbar, represents the boundary between
			     toolbars and application panels */
struct widget *wtbboundary;  /* htbboundary, dereferenced. Only used for comparison
				when deleting a toolbar. Do not rely on the
				validity of this pointer, dereferencing it could
				cause a segfault! */

g_error appmgr_init(void) {
  hwrbitmap bgbits;
  struct widget *w;
  g_error e;

  applist = NULL;  /* No apps yet! */

#ifdef DEBUG_INIT
   printf("Init: appmgr: default font\n");
#endif

  /* Allocate default font */
  e = findfont(&defaultfont,-1,NULL,0,PG_FSTYLE_DEFAULT);
  errorcheck;

#ifdef DEBUG_INIT
   printf("Init: appmgr: bg widget\n");
#endif
   
  /* Make the background widget */
  e = widget_create(&bgwidget,PG_WIDGET_BACKGROUND,dts->root,
		    &dts->root->head->next,0,-1);
  errorcheck;
  e = mkhandle(&hbgwidget,PG_TYPE_WIDGET,-1,bgwidget);   
  errorcheck;

#ifdef DEBUG_INIT
   printf("Init: appmgr: cursor sprite bitmaps\n");
#endif

  /* Load the default mouse cursor bitmaps */
  e = (*vid->bitmap_loadxbm)(&defaultcursor_bitmap,cursor_bits,
			     cursor_width,cursor_height,
			     (*vid->color_pgtohwr)(0xFFFFFF),
			     (*vid->color_pgtohwr)(0x000000));
  errorcheck;
  e = (*vid->bitmap_loadxbm)(&defaultcursor_bitmask,cursor_mask_bits,
			     cursor_width,cursor_height,
			     (*vid->color_pgtohwr)(0x000000),
			     (*vid->color_pgtohwr)(0xFFFFFF));
  errorcheck;

#ifdef DEBUG_INIT
   printf("Init: appmgr: cursor sprite\n");
#endif

  /* Allocate the sprite */
  e = new_sprite(&cursor,cursor_width,cursor_height);
  errorcheck;
   
  /* Load bitmaps */
  appmgr_loadcursor(PGTH_O_DEFAULT);
   
#ifdef DEBUG_INIT
   printf("Init: appmgr: strings\n");
#endif
   
  /* Default strings */
  e = mkhandle(&string_ok,PG_TYPE_STRING | HFLAG_NFREE,-1,"Ok");
  errorcheck;
  e = mkhandle(&string_cancel,PG_TYPE_STRING | HFLAG_NFREE,-1,"Cancel");
  errorcheck;
  e = mkhandle(&string_yes,PG_TYPE_STRING | HFLAG_NFREE,-1,"Yes");
  errorcheck;
  e = mkhandle(&string_no,PG_TYPE_STRING | HFLAG_NFREE,-1,"No");
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
  
  /* Free the mouse cursor */
  free_sprite(cursor);
  cursor = NULL;
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
  struct divnode *p;
  g_error e;

  /* Dereference the toolbar boundary */
  if (iserror(rdhandle((void**) &wtbboundary,PG_TYPE_WIDGET,-1,htbboundary)))
    wtbboundary = NULL;  

  /* Allocate root widget, do any setup specific to the app type */
  switch (i->type) {

  case PG_APP_TOOLBAR:
    /* Create a simple toolbar as a root widget. Create it after the previous
     * toolbar, if applicable, and update htbboundary. */
    if (wtbboundary)
      e = widget_derive(&w,PG_WIDGET_TOOLBAR,wtbboundary,htbboundary,PG_DERIVE_AFTER,i->owner);
    else
      e = widget_create(&w,PG_WIDGET_TOOLBAR,dts->root,&dts->root->head->next,0,i->owner);
    errorcheck;
    e = mkhandle(&i->rootw,PG_TYPE_WIDGET,i->owner,w);
    errorcheck;    
    htbboundary = i->rootw;
    wtbboundary = w;

    /* Size specs are ignored for the toolbar.
       They won't be moved by the appmgr, so sidemask has no effect.
       Set the side here, though.
    */
    e = widget_set(w,PG_WP_SIDE,i->side);
    errorcheck;
    
    break;

  case PG_APP_NORMAL:
    /* Put the new app right before the background widget */
    e = widget_derive(&w,PG_WIDGET_PANEL,bgwidget,hbgwidget,PG_DERIVE_BEFORE,i->owner);
    errorcheck;
    e = mkhandle(&i->rootw,PG_TYPE_WIDGET,i->owner,w);
    errorcheck;    

    /* Set all the properties */
    e = widget_set(w,PG_WP_TEXT,i->name);
    errorcheck;
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

/* Load the mouse cursor specified by the given theme object */
void appmgr_loadcursor(int thobj) {
   hwrbitmap bitmap,mask;
   int w,h;
   
   /* Load the cursor bitmaps, using the default if there is a problem */
   
   if (iserror(rdhandle((void**)&bitmap,PG_TYPE_BITMAP,-1,
			theme_lookup(thobj,PGTH_P_CURSORBITMAP))) || !bitmap)
     bitmap = defaultcursor_bitmap;
   if (iserror(rdhandle((void**)&mask,PG_TYPE_BITMAP,-1,
			theme_lookup(thobj,PGTH_P_CURSORBITMASK))) || !mask)
     mask = defaultcursor_bitmask;
   (*vid->bitmap_getsize)(bitmap,&w,&h);
  
   /* Insert the new bitmaps, resize the sprite if necessary */

   (*vid->sprite_hide)(cursor);

   if ( (w!=cursor->w) || (h!=cursor->h) ) {
      cursor->w = w;
      cursor->h = h;
      (*vid->bitmap_free)(cursor->backbuffer);
      (*vid->bitmap_new)(&cursor->backbuffer,w,h);
   }
   
   cursor->bitmap = bitmap;
   cursor->mask = mask;
   
   (*vid->sprite_show)(cursor);
}
	

/* The End */


