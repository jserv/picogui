/* $Id$
 * 
 * svrtheme.h - functions and data structures for themes, used
 *              only in the server
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

#ifndef __SVRTHEME_H
#define __SVRTHEME_H

#include <pgserver/g_error.h>
#include <pgserver/divtree.h>
#include <pgserver/pgstring.h>

/**** Structures describing a theme in memory */

struct pgmemtheme {
  /* Next theme loaded */
  struct pgmemtheme *next;

  /* Does this theme require a full redraw on load/unload, 
   * or is it simple enough not to need that? */
  bool requires_full_update;

  /* Immediately following this header is an array of theme objects */
  unsigned short num_thobj;  /* Number of objects in the array */

  /* The handle the theme was assigned on loading */
  handle h;
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
  u32            data;       /* Property data, already processed by the loader */
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

/* Reload themes according to the ones set in the config database */
g_error reload_initial_themes(void);

/* Free all memory used by server-loaded themes */
void theme_shutdown(void);

/* Searches for a thobj in a sorted array. Null if none found */
struct pgmemtheme_thobj *find_thobj(struct pgmemtheme *th,unsigned short id);

/* Search for a property in a sorted array. Null if none found */
struct pgmemtheme_prop *find_prop(struct pgmemtheme_thobj *tho,unsigned short id);

/* Look for the given property, starting at 'object'
 * returns the property's 'data' member */
u32 theme_lookup(u16 object, u16 property);

/* Load a theme into memory from a compiled theme heap */
g_error theme_load(handle *h,int owner,const u8 *themefile,
		   u32 themefile_len);

/* Remove a theme from memory */
void theme_remove(struct pgmemtheme *th);

/* Fillstyle checker- verifies that a fillstyle looks valid */
g_error check_fillstyle(const unsigned char *fs, u32 fssize);

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

/* Custom theme objects are assigned unused IDs automatically.
 * Objects with the same name get the same ID 
 */
u16 custom_thobj_id(struct pgstring *name);

/* Reports whether the given theme object ID has not yet been taken
 * in any of the loaded themes.
 */
int thobj_id_available(s16 id);

/* Find a theme's id given its name. Returns nonzero if the object
 * was found sucessfully, and loads 'id' with its id.
 */
int find_named_thobj(const struct pgstring *name, s16 *id);

/* Given a theme object, returns the theme object parent's ID 
 */
s16 thobj_parent(s16 id);

#ifdef CONFIG_ANIMATION
/* Use the specified value as the 'ticks' theme property instead of
 * retrieving it with os_getticks(). This lets pgserver be used to generate
 * non-realtime animation.
 */
void pg_ticks_override(u32 ticks);
#endif

#endif /* __SVRTHEME_H */

/* The End */




