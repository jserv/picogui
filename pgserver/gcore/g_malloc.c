/* $Id: g_malloc.c,v 1.19 2002/02/03 16:07:58 lonetech Exp $
 *
 * g_malloc.c - malloc wrapper providing error handling
 *
 * During debugging, tacks on a size_t to the beginning of all
 * allocated memory, and keeps track of memory usage
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

#include <stdlib.h>

long memref = 0;

#ifdef DEBUG_ANY
long memamt = 0;       /* Bytes of memory total */
#endif

#ifdef DEBUG_KEYS
/* Memory allocation statistics, for debugging and profiling */
long num_grops = 0;    /* Number of gropnodes */
long num_divs = 0;     /* Number of divnodes */
long num_widgets = 0;  /* Number of widgets */
long num_handles = 0;  /* Number of handles */
#endif

g_error g_malloc(void **p,size_t s) {
  if (!p) return mkerror(PG_ERRT_INTERNAL,25);

#ifdef DEBUG_ANY
  *p = malloc(s+sizeof(size_t));
#else
  *p = malloc(s);
#endif
  if (!(*p)) return mkerror(PG_ERRT_MEMORY,25);
  memref++;
#ifdef DEBUG_ANY
  *(((size_t *)(*p))++) = s;
  memamt += s;
#endif

#ifdef DEBUG_MEMORY
  printf("+%d #%ld (%ld) %p\n",s,memref,memamt,*((char**)p));
#endif

  return success;
}

void g_free(const void *p) {
#ifdef DEBUG_ANY
  size_t s;
#ifdef DEBUG_MEMORY
  const char *adr = p;
#endif
#endif
  if (!p) return;
#ifdef DEBUG_ANY
  ((size_t*)p)--;
#endif
  memref--;
#ifdef DEBUG_ANY
  memamt -= (s = *((size_t*)p));
#ifdef DEBUG_MEMORY
  printf("-%d #%ld (%ld) %p\n",s,memref,memamt,adr);
#endif
#endif

  free((void*)p);
}

g_error g_realloc(void **p,size_t s) {
#ifdef DEBUG_ANY
  size_t from;
#endif
  void *oldp;

  if (!p) return mkerror(PG_ERRT_BADPARAM,25);

#ifdef DEBUG_ANY
  ((size_t*)(*p))--;    /* Get the _real_ pointer so realloc will like us */
  oldp=p;
  memamt -= (from = *((size_t*)(*p)));  /* Store original size */
  *p = realloc(*p,s+sizeof(size_t));
#else
  oldp=p;
  *p = realloc(*p,s);
#endif
  if (!(*p))
   {
    memref--;
    free(oldp);
    return mkerror(PG_ERRT_MEMORY,25);
   }

#ifdef DEBUG_ANY
  *(((size_t *)(*p))++) = s;
  memamt += s;
#endif

#ifdef DEBUG_MEMORY
  printf("* [%d -> %d] #%ld (%ld) %p\n",from,s,memref,memamt,*((char**)p));
#endif

  return success;

}

/* The End */

