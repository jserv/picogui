/* $Id$
 *
 * appmgr.h - Generic interface to application manager modules
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

#ifndef __H_APPMGR
#define __H_APPMGR

#include <picogui/constants.h>
#include <pgserver/handle.h>
#include <pgserver/widget.h>
#include <pgserver/divtree.h>


/**************************************** Public interface */

/* Parameters defining an application */
struct app_info {
  /* Client-provided data  */
  handle name;      /* Name of the app, as a string */
  int type;         /* PG_APP_* constant */  
  struct sizepair default_size, min_size, max_size;
  int side,sidemask;

  int owner;        /* This should be managed by the request system. */
  handle rootw;     /* Root widget handle */

  void *private;    /* Data owned by the app manager module */

  struct app_info *next;
};

/* most-recently-used ordered linked list of all loaded apps */
extern struct app_info *applist;

/* Init & Free */
g_error appmgr_init(void);
void appmgr_free(void);

/* Register a new application.
 * Any information provided by the client (Usually name and type)
 * should be filled in this structure before calling, the final values
 * will be stored by the app manager and returned in the structure.
 */
g_error appmgr_register(struct app_info *i);

/* Unregister one application. Delete its app_info record, and remove
 * its root widget if it still exists.
 */
void appmgr_unregister(struct app_info **i);

/* Unregisters all applications owned by a given connection */
void appmgr_unregowner(int owner);

/* Return a pointer to a divnode specifying the non-toolbar area that
 * applications and popup boxes may normally inhabit. 
 * This returns NULL if this is not applicable.
 */
struct divnode *appmgr_nontoolbar_area(void);

/* Given a widget, finds the app it belongs to */
struct app_info **appmgr_findapp(struct widget *w);

/* Focus the app by moving it to the front of the app list */
void appmgr_focus(struct app_info **app);

/* All widget instantiations are passed through this function, optionally
 * changing which widget is created. This includes parents of subclassed widgets.
 */
int appmgr_widget_map(int w);


/**************************************** App manager module interface */

struct appmgr {

  /* The name the user can refer to this module as */
  const char *name;

  /* Optional: Appmgr-specific initialization and shutdown */
  g_error (*init)(void);
  void (*shutdown)(void);

  /* Required: 
   * The provided app_info structure has already been initialized
   * with the data provided by our client. If this call succeeds, it will
   * be added to the linked list of apps.
   */
  g_error (*reg)(struct app_info *i);  

  /* Optional:
   * The provided app_info has already been delinked from the list,
   * and after this call it will be freed. Perform any appmgr-specific
   * cleanups on it. If i->rootw is nonzero after this call, it will be
   * automatically deleted.
   */
  void (*unreg)(struct app_info *i);

  /* Optional:
   * Return the nontoolbar area if this appmgr has such a concept
   */
  struct divnode *(*nontoolbar_area)(void);

  /* Optional:
   * All widget instantiations are passed through this function, optionally
   * changing which widget is created. This includes parents of subclassed widgets.
   */
  int (*widget_map)(int w);

};

extern struct appmgr *appmgr_modules[];

extern struct appmgr appmgr_null;
extern struct appmgr appmgr_panel;
extern struct appmgr appmgr_managed_rootless;


#endif /* __H_APPMGR */
/* The End */
