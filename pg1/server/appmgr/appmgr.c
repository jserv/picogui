/* $Id$
 *
 * appmgr.c - Manage the application list, and use one of the installed
 *            app manager modules to determine app policy.
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
#include <pgserver/appmgr.h>
#include <pgserver/configfile.h>

/* most-recently-used ordered linked list of all loaded apps */
struct app_info *applist;

struct appmgr *current_appmgr;

/**************************************** App manager modules */

/* This should be sorted in reverse order of preference-
 * items earlier in this list are probed first.
 */
struct appmgr *appmgr_modules[] = {
#ifdef CONFIG_APPMGR_MANAGED_ROOTLESS
  &appmgr_managed_rootless,
#endif
#ifdef CONFIG_APPMGR_PANEL
  &appmgr_panel,
#endif
#ifdef CONFIG_APPMGR_NULL
  &appmgr_null,
#endif
  NULL
};

  
/**************************************** Public functions */

/* Use the specified appmgr, or try them in sequence until one loads */
g_error appmgr_init(void) {
  struct appmgr **p;
  const char *name = get_param_str("pgserver","appmgr",NULL);
  g_error e;

  if (name) {
    /* Load one particular app manager */
    for (p=appmgr_modules;*p;p++) {
      if (strcmp(name,(*p)->name))
	continue;
      current_appmgr = *p;
      if (current_appmgr->init) {
	e = current_appmgr->init();
	errorcheck;
      }
      break;
    }
    if (!*p)
      return mkerror(PG_ERRT_BADPARAM,143);   /* Unknown app manager */
  }
  else {
    /* Try them all in order */
    for (p=appmgr_modules;*p;p++) {
      current_appmgr = *p;
      if (current_appmgr->init) {
	e = current_appmgr->init();
	if (!iserror(e))
	  break;
      }
    }
    if (!*p)
      return mkerror(PG_ERRT_BADPARAM,141);   /* All installed app managers failed */
  }

  return success;
}

void appmgr_free(void) {
  /* Unregister all remaining apps */
  while (applist)
    appmgr_unregister(&applist);

  if (current_appmgr->shutdown)
    current_appmgr->shutdown();
}

/* Register a new application.
 * Any information provided by the client (Usually name and type)
 * should be filled in this structure before calling, the final values
 * will be stored by the app manager and returned in the structure.
 */
g_error appmgr_register(struct app_info *i) {
  g_error e;
  struct app_info *new_node;
  struct widget *w;

  e = current_appmgr->reg(i);
  errorcheck;

  /* We're successfully registered, add the info to applist */
  e = g_malloc((void**)&new_node,sizeof(struct app_info));
  errorcheck;
  memcpy(new_node,i,sizeof(struct app_info));
  new_node->next = applist;
  applist = new_node;

  /* Set the name of the root widget to the given name, so we can find
   * the application with pgFindWidget later easily.
   */
  e = rdhandle((void**) &w,PG_TYPE_WIDGET,i->owner,i->rootw);
  errorcheck;
  e = widget_set(w,PG_WP_NAME,i->name);
  errorcheck;
  
  return success;
}

/* Unregister one application. Delete its app_info record, and remove
 * its root widget if it still exists.
 */
void appmgr_unregister(struct app_info **i) {
  struct app_info *inf = *i;

  /* Delete from the list */
  *i = (*i)->next;

  if (current_appmgr->unreg)
    current_appmgr->unreg(inf);

  /* Delete root widget */
  handle_free(inf->owner,inf->rootw);
  
  g_free(inf);
}

/* Unregisters applications owned by a given connection */
void appmgr_unregowner(int owner) {
  struct app_info **i = &applist;
  while (*i) {
    if ((*i)->owner == owner)
      appmgr_unregister(i);
    else
      i = &(*i)->next;
  }
}

/* Return a pointer to a divnode specifying the non-toolbar area that
 * applications and popup boxes may normally inhabit. 
 * This returns NULL if this is not applicable.
 */
struct divnode *appmgr_nontoolbar_area(void) {
  if (current_appmgr->nontoolbar_area)
    return current_appmgr->nontoolbar_area();
  return NULL;
}

/* Given a widget, finds the app it belongs to */
struct app_info **appmgr_findapp(struct widget *w) {
  struct app_info **p = &applist;
  struct widget *rootw;

  /* Find the root widget */
  while (w) {
    if (w->isroot)
      break;
    w = widget_traverse(w, PG_TRAVERSE_CONTAINER, 1);
  }
  if (!w) return NULL;

  /* Now find the app_info entry */
  while (*p) {
    if (!iserror(rdhandle((void**) &rootw, PG_TYPE_WIDGET, 
			  (*p)->owner, (*p)->rootw)) && rootw==w)
      return p;
    p = &((*p)->next);
  }
  return NULL;
}

/* Focus the app by moving it to the front of the app list */
void appmgr_focus(struct app_info **app) {
  struct app_info *ap;
  if (!app) return;
  ap = *app;
  *app = ap->next;
  ap->next = applist;
  applist = ap;
}

/* All widget instantiations are passed through this function, optionally
 * changing which widget is created. This includes parents of subclassed widgets.
 */
int appmgr_widget_map(int w) {
  if (current_appmgr->widget_map)
    return current_appmgr->widget_map(w);
  return w;
}

/* The End */



