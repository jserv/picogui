/* $Id: input_filters.c,v 1.1 2002/05/22 09:26:32 micahjd Exp $
 *
 * input_filters.c - Abstract input filter interface
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

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/appmgr.h>

/* Head of the input filter list */
struct infilter *infilter_list;

/******************************************** Infilter management */

/* Pass an event to the next input filter, given a source filter.
 * Events coming directly from drivers should give NULL as the source.
 *
 * Assumes there's at least one filter loaded
 */
void infilter_send(struct infilter *from, u32 trigger, union trigparam *param) {
  struct infilter *to;

  do {
    /* Find the destination */
    to = from ? from->next : infilter_list;
    
    /* Accept triggers */
    if (to->accept_trigs & trigger)
      to->handler(trigger,param);
    
    /* Pass triggers */
    if (to->absorb_trigs & trigger)
      return;
    from = to;

  } while (from);
}

g_error infilter_insert(struct infilter **insertion, handle *h, int owner,
			struct infilter *template) {
  struct infilter *n;
  g_error e;
  
  e = g_malloc((void**) &n, sizeof(struct infilter));
  errorcheck;
  memcpy(n, template, sizeof(struct infilter));
  
  n->next = *insertion;
  *insertion = n;

  return mkhandle(h,PG_TYPE_INFILTER,owner,n);
}

void infilter_delete(struct infilter *node) {
  struct infilter **i;

  /* Remove the node from our filter list if it's there */
  for (i=&infilter_list;*i;i=&((*i)->next))
    if (*i == node) {
      *i = node->next;
      break;
    }
  
  g_free(node);
}

/******************************************** Registration */
  
/* The built-in input filters' template structures
 * (executed in the order shown below)
 */

/* Clean up and standardize input from the drivers */
extern struct infilter infilter_key_normalize, infilter_pntr_normalize;

/* Touchscreen calibration and filtering */
extern struct infilter infilter_touchscreen;

/* Transform coordinates and otherwise munge the data 
 * so it's ready for widget consumption */
extern struct infilter infilter_key_preprocess, infilter_pntr_preprocess;

/* Handling magic CTRL-ALT-* keys */
extern struct infilter infilter_key_magic;

/* Send the input where it needs to go */
extern struct infilter infilter_key_dispatch, infilter_pntr_dispatch;

/* Instantiate all of the build-in input filters. */
g_error infilter_init(void) {
  g_error e;
  int i;
  struct infilter *filters[] = {
    &infilter_key_normalize,
    &infilter_pntr_normalize,
#ifdef CONFIG_TOUCHSCREEN
    &infilter_touchscreen,
#endif
    &infilter_key_preprocess,
    &infilter_pntr_preprocess,
#ifdef DEBUG_KEYS
    &infilter_key_magic,
#endif
    &infilter_key_dispatch,
    &infilter_pntr_dispatch,
  };
  int filter_res[] = {
    PGRES_INFILTER_KEY_NORMALIZE,
    PGRES_INFILTER_PNTR_NORMALIZE,
#ifdef CONFIG_TOUCHSCREEN
    PGRES_INFILTER_TOUCHSCREEN,
#endif
    PGRES_INFILTER_KEY_PREPROCESS,
    PGRES_INFILTER_PNTR_PREPROCESS,
#ifdef DEBUG_KEYS
    PGRES_INFILTER_KEY_MAGIC,
#endif
    PGRES_INFILTER_KEY_DISPATCH,
    PGRES_INFILTER_PNTR_DISPATCH,
  };

  for (i=(sizeof(filters)/sizeof(filters[0]))-1;i>=0;i--) {
    e = infilter_insert(&infilter_list, &res[filter_res[i]],-1, filters[i]);
    errorcheck;
  }

  return success;
}

/* The End */






