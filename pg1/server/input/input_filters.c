/* $Id$
 *
 * input_filters.c - Abstract input filter interface
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
#include <pgserver/input.h>
#include <pgserver/appmgr.h>

#ifdef DEBUG_EVENT
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

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

  while (1) {
    /* Find the destination */
    to = from ? from->next : infilter_list;

#ifdef DEBUG_INFILTER_CHAIN
    if (!from) {
      DBG("********************************* BEGIN FILTER CHAIN ***\n");
    }
    DBG("Sending trigger type 0x%08X from infilter %p to infilter %p:\n",trigger,from,to);
    DBG("\ttrigparam union is at %p, contains:\n",param);
    DBG("\tmouse:\n");
    DBG("\t\tx = %d\n",param->mouse.x);
    DBG("\t\ty = %d\n",param->mouse.y);
    DBG("\t\tbtn = 0x%08X\n",param->mouse.btn);
    DBG("\t\tchbtn = 0x%08X\n",param->mouse.chbtn);
    DBG("\t\tpressure = %d\n",param->mouse.pressure);
    DBG("\t\tcursor = %p\n",param->mouse.cursor);
    DBG("\tkbd:\n");
    DBG("\t\tkey = %d\n",param->kbd.key);
    DBG("\t\tmods = 0x%04X\n",param->kbd.mods);
    DBG("\t\tflags = 0x%04X\n",param->kbd.flags);
    DBG("\t\tconsume = %d\n",param->kbd.consume);
    DBG("\tstream:\n");
    DBG("\t\tsize = %d\n",param->stream.size);
    DBG("\t\tdata = %p\n",param->stream.data);
#endif

    /* Has the event reached the end of the filter chain? */
    if (!to)
      return;

    /* Accept triggers */
    if (to->accept_trigs & trigger)
      to->handler(to,trigger,param);
    
    /* Pass triggers */
    if (to->absorb_trigs & trigger)
      return;
    from = to;
  }
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

void infilter_send_key(u32 trigger, int key, int mods) {
  union trigparam p;
  memset(&p,0,sizeof(p));
  p.kbd.key = key;
  p.kbd.mods = mods;
  infilter_send(NULL,trigger,&p);
}

void infilter_send_pointing(u32 trigger, int x, int y, 
			    int btn, struct cursor *cursor) {
  union trigparam p;
  memset(&p,0,sizeof(p));
  p.mouse.x = x;
  p.mouse.y = y;
  p.mouse.btn = btn;
  p.mouse.cursor = cursor;
  infilter_send(NULL,trigger,&p);
}

void infilter_send_touchscreen(int x, int y, int pressure, int btn) {
  union trigparam p;
  memset(&p,0,sizeof(p));
  p.mouse.x = x;
  p.mouse.y = y;
  p.mouse.pressure = pressure;
  p.mouse.btn = btn;
  infilter_send(NULL,PG_TRIGGER_TOUCHSCREEN,&p);
}

/******************************************** Registration */
  
/* The built-in input filters' template structures
 * (executed in the order shown below)
 */

/* Handle PNTR_STATUS events and stuff before the touchscreen processing */
extern struct infilter infilter_pntr_normalize;

/* Touchscreen calibration and filtering */
extern struct infilter infilter_touchscreen;

/* Transform coordinates and otherwise munge the data 
 * so it's ready for widget consumption */
extern struct infilter infilter_key_preprocess, infilter_pntr_preprocess;

/* Handling magic CTRL-ALT-* keys */
extern struct infilter infilter_magic;

/* Optional input filter for managing the "alpha" key on some numeric keypads */
extern struct infilter infilter_key_alpha;

/* Send the input where it needs to go */
extern struct infilter infilter_key_dispatch, infilter_pntr_dispatch;

/* Process global navigation keys used for hotspots */
extern struct infilter infilter_hotspot;

/* Instantiate all of the build-in input filters. */
g_error infilter_init(void) {
  g_error e;
  int i;
  struct infilter *filters[] = {
#ifdef CONFIG_TOUCHSCREEN
    &infilter_touchscreen,
#endif
    &infilter_pntr_normalize,
    &infilter_key_preprocess,
    &infilter_pntr_preprocess,
#ifdef CONFIG_KEY_ALPHA
    &infilter_key_alpha,
#endif
    &infilter_magic,
    &infilter_key_dispatch,
    &infilter_pntr_dispatch,
    &infilter_hotspot,
  };
  int filter_res[] = {
#ifdef CONFIG_TOUCHSCREEN
    PGRES_INFILTER_TOUCHSCREEN,
#endif
    PGRES_INFILTER_PNTR_NORMALIZE,
    PGRES_INFILTER_KEY_PREPROCESS,
    PGRES_INFILTER_PNTR_PREPROCESS,
    PGRES_INFILTER_MAGIC,
#ifdef CONFIG_KEY_ALPHA
    PGRES_INFILTER_KEY_ALPHA,
#endif
    PGRES_INFILTER_KEY_DISPATCH,
    PGRES_INFILTER_PNTR_DISPATCH,
    PGRES_INFILTER_HOTSPOT,
  };

  for (i=(sizeof(filters)/sizeof(filters[0]))-1;i>=0;i--) {
    e = infilter_insert(&infilter_list, &res[filter_res[i]],-1, filters[i]);
    errorcheck;
  }

  /* Load them into any active cursors */
  e = cursor_retheme();
  errorcheck;

  return success;
}

/* The End */






