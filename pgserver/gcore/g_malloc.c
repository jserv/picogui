
/* $Id: g_malloc.c,v 1.5 2000/08/14 19:35:45 micahjd Exp $
 *
 * g_malloc.c - malloc wrapper providing error handling
 *
 * During debugging, tacks on a size_t to the beginning of all allocated memory,
 * and keeps track of memory usage
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

#include <stdlib.h>
#include <g_malloc.h>
#include <g_error.h>

long memref = 0;

#ifdef DEBUG
long memamt = 0;
#endif

g_error g_malloc(void **p,size_t s) {
  if (!p) return mkerror(ERRT_INTERNAL,24);

#ifdef DEBUG
  *p = malloc(s+sizeof(size_t));
#else
  *p = malloc(s);
#endif
  if (!(*p)) return mkerror(ERRT_MEMORY,25);
  memref++;
#ifdef DEBUG
  *(((size_t *)(*p))++) = s;
  memamt += s;

  printf("+%d #%d (%d) 0x%08X\n",s,memref,memamt,*((char**)p));
#endif

  return sucess;
}

void g_free(void *p) {
#ifdef DEBUG
  size_t s;
  char *adr = p;
#endif
  if (!p) return;
#ifdef DEBUG
  ((size_t*)p)--;
#endif
  memref--;
#ifdef DEBUG
  memamt -= (s = *((size_t*)p));
  printf("-%d #%d (%d) 0x%08X\n",s,memref,memamt,adr);
#endif

  free(p);
}

g_error g_realloc(void **p,size_t s) {
#ifdef DEBUG
  size_t from;
#endif

  if (!p) return mkerror(ERRT_BADPARAM,24);

#ifdef DEBUG
  ((size_t*)(*p))--;    /* Get the _real_ pointer so realloc will like us */
  memamt -= (from = *((size_t*)(*p)));  /* Store original size */
  *p = realloc(*p,s+sizeof(size_t));
#else
  *p = realloc(*p,s);
#endif
  if (!(*p)) return mkerror(ERRT_MEMORY,25);

#ifdef DEBUG
  *(((size_t *)(*p))++) = s;
  memamt += s;

  printf("* [%d -> %d] #%d (%d) 0x%08X\n",from,s,memref,memamt,*((char**)p));
#endif

  return sucess;

}

/* The End */

