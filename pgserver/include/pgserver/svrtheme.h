/* $Id: svrtheme.h,v 1.4 2001/06/01 01:00:47 micahjd Exp $
 * 
 * svrtheme.h - functions and data structures for themes, used
 *              only in the server
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

#ifndef __SVRTHEME_H
#define __SVRTHEME_H

#include <pgserver/g_error.h>
#include <pgserver/divtree.h>

/**** Structures describing a theme in memory */

struct pgmemtheme {
  /* Next theme loaded */
  struct pgmemtheme *next;
  
  /* Immediately following this header is an array of theme objects */
  unsigned short num_thobj;  /* Number of objects in the array */
};

/* Macro for a theme's thobj list */
#define theme_thobjlist(th)  ((struct pgmemtheme_thobj *)   \
			      (((unsigned char *)(th))+     \
			       sizeof(struct pgmemtheme)))

/* Defines one property that is used in a theme object. This could represent
 * something like "background color = blue" but also something more complex
 * like "The height of this widget should be 1.2 times the height of the
 * default font"
 */
struct pgmemtheme_prop {
  unsigned short id;         /* a PGTH_P_* constant */
  unsigned short loader;     /* Loader constant */
  unsigned long  data;       /* Property data, already processed by the loader */
};

/* The header for a theme object. See the theme constants section
   in picogui/constants.h for more information */
struct pgmemtheme_thobj {
  unsigned short id;                /* A PGTH_O_* constant */
  unsigned short num_prop;          /* Number of properties */
  union {
    struct pgmemtheme_prop *ptr;    /* When loaded into internal heap */
    unsigned long offset;           /* The original file offset */
  } proplist;
};

/**** Functions for manipulating in-memory themes */

/* Searches for a thobj in a sorted array. Null if none found */
struct pgmemtheme_thobj *find_thobj(struct pgmemtheme *th,unsigned short id);

/* Search for a property in a sorted array. Null if none found */
struct pgmemtheme_prop *find_prop(struct pgmemtheme_thobj *tho,unsigned short id);

/* Look for the given property, starting at 'object'
 * returns the property's 'data' member */
unsigned long theme_lookup(unsigned short object,
			   unsigned short property);

/* Load a theme into memory from a compiled theme heap */
g_error theme_load(handle *h,int owner,char *themefile,
		   unsigned long themefile_len);

/* Remove a theme from memory */
void theme_remove(struct pgmemtheme *th);

/* Fillstyle interpreter- generates/refreshes a gropnode list */
g_error exec_fillstyle(struct gropctxt *ctx,unsigned short state,
		       unsigned short property);

/* By calling the 'build' function, rebuilds a divnode's groplist */
void div_rebuild(struct divnode *d);

/* Change a divnode's state, and update the necessary things. 
 * Unless 'force' is nonzero, it will check whether the update is really
 * necessary.
 */
void div_setstate(struct divnode *d,unsigned short state,bool force);

/* Small build function for widgets that only need a background */
void build_bgfill_only(struct gropctxt *c,unsigned short state,struct widget *self);

#endif /* __SVRTHEME_H */

/* The End */




