/*
 * g_malloc.c - malloc wrapper providing error handling
 * $Revision: 1.1 $
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <stdlib.h>
#include <g_malloc.h>
#include <g_error.h>

long memref = 0;

#ifdef DEBUG
long memamt = 0;
#endif

g_error g_malloc(void **p,size_t s) {
  if (!p) return mkerror(ERRT_BADPARAM,"p==NULL in g_malloc");

#ifdef DEBUG
  *p = malloc(s+sizeof(size_t));
#else
  *p = malloc(s);
#endif
  if (!(*p)) return mkerror(ERRT_MEMORY,"Allocation error");
  memref++;
#ifdef DEBUG
  *(((size_t *)(*p))++) = s;
  memamt += s;

  printf("+%d #%d (%d)\n",s,memref,memamt);
#endif

  return sucess;
}

void g_free(void *p) {
#ifdef DEBUG
  size_t s;
#endif
  if (!p) return;
#ifdef DEBUG
  ((size_t*)p)--;
#endif
  memref--;
#ifdef DEBUG
  memamt -= (s = *((size_t*)p));
  printf("-%d #%d (%d)\n",s,memref,memamt);
#endif

  free(p);
}

/* The End */

