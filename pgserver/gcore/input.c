/* $Id: input.c,v 1.8 2001/02/14 05:13:18 micahjd Exp $
 *
 * input.c - Abstract input driver interface
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

#include <pgserver/common.h>

#include <pgserver/input.h>

/******************************************** Vars */

/* Corresponding to the 'extern' definitions in input.h */
struct inlib *inlib_list = NULL;
struct inlib *inlib_main;

/* Loads an input driver, and puts a pointer 
   to it in ppinlib. */
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
  return sucess;
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

/* The End */










