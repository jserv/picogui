/* $Id: global.c,v 1.2 2000/04/24 02:38:36 micahjd Exp $
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

/* App manager doesn't need to be freed, everything is cleaned
   up by the handles */
g_error appmgr_init(struct dtstack *m_dts) {
  struct bitmap *bgbits;
  struct widget *w;
  g_error e;

  /* Allocate default font */
  e = findfont(&defaultfont,-1,NULL,0,FSTYLE_DEFAULT);
  if (e.type != ERRT_NONE) return e;

  /* Load the default background */
  e = hwrbit_pnm(&bgbits,bg_bits,bg_len);
  if (e.type != ERRT_NONE) return e;
  e = mkhandle(&background,TYPE_BITMAP,-1,bgbits);
  if (e.type != ERRT_NONE) return e;

  /* Make the background widget */
  e = widget_create(&bgwidget,WIDGET_BITMAP,m_dts,m_dts->top,
		    &m_dts->top->head->next);
  if (e.type != ERRT_NONE) return e;
  e = widget_set(bgwidget,WP_BITMAP,(glob)background);
  if (e.type != ERRT_NONE) return e;
  e = widget_set(bgwidget,WP_ALIGN,A_ALL);
  if (e.type != ERRT_NONE) return e;
  e = mkhandle(&hbgwidget,TYPE_WIDGET,-1,bgwidget);   
  if (e.type != ERRT_NONE) return e;

  return sucess;
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

/* The End */

