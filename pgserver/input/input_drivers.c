/* $Id: input_drivers.c,v 1.1 2002/05/21 19:45:33 micahjd Exp $
 *
 * input_drivers.c - Abstract input driver interface
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

#include <string.h>

#include <pgserver/common.h>

#include <pgserver/input.h>

/******************************************** Vars */

/* Corresponding to the 'extern' definitions in input.h */
struct inlib *inlib_list = NULL;
struct inlib *inlib_main;

/* If this is 1, no user input is taken. */
int disable_input;

/******************************************** Inlib management */

/* Loads an input driver, and puts a pointer 
   to it in 'inl' */
g_error load_inlib(g_error (*regfunc)(struct inlib *i),
		   struct inlib **inl) {
  struct inlib *newnode,*p;
  g_error e;

  if (!regfunc)
    return mkerror(PG_ERRT_BADPARAM,75);

  /* Avoid duplicates */
  p = inlib_list;
  while (p) {
    if (p->regfunc == regfunc)
      return mkerror(PG_ERRT_BADPARAM,76);
    p = p->next;
  }

  /* Allocate... */
  e = g_malloc((void**)&newnode,sizeof(struct inlib));
  errorcheck;
  memset(newnode,0,sizeof(struct inlib));

  /* Register it */
  e = (*regfunc)(newnode);
  if (iserror(e)) {
    g_free(newnode);
    return e;
  }
  newnode->regfunc = regfunc;

  /* Insert */
  newnode->next = inlib_list;
  inlib_list = newnode;

  /* Init */
  if (newnode->init)
    e = (*newnode->init)();
  if (iserror(e)) {
    g_free(newnode);
    return e;
  }

  /* Return stuff */
  if (inl)
    *inl = newnode;
  return success;
}

/* Unload a specific driver */
void unload_inlib(struct inlib *inl) {
  if (!inl) return;

  if (inlib_list==inl)
    inlib_list = inlib_list->next;
  else if (inlib_list) {
    struct inlib *n=inlib_list;
    while (n->next)
      if (n->next==inl) {
	n->next = n->next->next;
	break;
      }
      else
	n = n->next;
  }

  if (inl->close)
    (*inl->close)();
  g_free(inl);
}

/* Unload all drivers */
void cleanup_inlib(void) {
  struct inlib *n,*condemn;
  
  n = inlib_list;
  while (n) {
    condemn = n;
    n = n->next;
    if (condemn->close)
      (*condemn->close)();
    g_free(condemn);
  }
  inlib_list = NULL;
  inlib_main = NULL;
}

g_error (*find_inputdriver(const char *name))(struct inlib *i) {
  struct inputinfo *p = inputdrivers;
  while (p->name) {
    if (!strcmp(name,p->name))
      return p->regfunc;
    p++;
  }
  return NULL;
}

/* Check whether any drivers have a backlog of events.
 * This calls the driver's ispending function */
int events_pending(void) {
   struct inlib *p = inlib_list;
   while (p) {
      if (p && p->ispending)
	if ((*p->ispending)())
	  return 1;
      p = p->next;
   }
   return 0;
}

/* The End */









