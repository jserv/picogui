/* $Id$
 *
 * g_malloc.c - malloc wrapper providing error handling
 *
 * During debugging, tacks on a size_t to the beginning of all
 * allocated memory, and keeps track of memory usage
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

#include <stdlib.h>
#include <string.h>

int memref = 0;

#ifdef DEBUG_ANY
long memamt = 0;       /* Bytes of memory total */
#endif

#ifdef DEBUG_KEYS
/* Memory allocation statistics, for debugging and profiling */
int num_grops = 0;    /* Number of gropnodes */
int num_divs = 0;     /* Number of divnodes */
int num_widgets = 0;  /* Number of widgets */
int num_handles = 0;  /* Number of handles */
#endif

struct memtrack_struct *memtrack=NULL;

g_error memoryleak_trace(void) {
  int i;

  if (memref==0)
    return success;
  
#ifdef DEBUG_MEMORY
  for(i=0;i<memref;i++)
    fprintf(stderr, "!%d #%d %p %s\n", memtrack[i].size, i,
	    memtrack[i].mem, memtrack[i].where);
#endif
  
  return mkerror(PG_ERRT_MEMORY,56);  /* Memory leak */
}


#ifdef DEBUG_MEMORY
g_error g_dmalloc(void **p,size_t s,const char *where) {
#else
g_error g_malloc(void **p,size_t s) {
#endif
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
  memtrack=realloc(memtrack, sizeof(*memtrack)*memref);
  if(memref && memtrack==NULL)
   {
    guru("Aieee! Memory tracker ran out of memory!");
    exit(234);
   }
  memtrack[memref-1].mem=*p;
  memtrack[memref-1].size=s;
  memtrack[memref-1].where=where;
  fprintf(stderr, "+%d #%d (%ld) %p %s\n",s,memref,memamt,*p,where);
#endif

  return success;
}

#ifndef DEBUG_MEMORY
void g_free(const void *p) {
#else
void g_dfree(const void *p, const char *where) {
  int i;
  const char *adr = p;
#endif
#ifdef DEBUG_ANY
  size_t s;
#endif
  if (!p) return;
#ifdef DEBUG_ANY
  ((size_t*)p)--;
#endif
  memref--;
#ifdef DEBUG_ANY
  memamt -= (s = *((size_t*)p));
#ifdef DEBUG_MEMORY
  for(i=0; i<memref+1; i++)
   {
    if(memtrack[i].mem==adr)
     {
      memmove(memtrack+i, memtrack+memref, sizeof(*memtrack));
      break;
     }
   }
  /* Argh!! Electric Fence assumes it's a bug if we allocate
   * zero bytes, so to keep efence from aborting us, don't resize
   * this if it would be sizing to zero bytes.
   */
#ifdef CONFIG_EFENCE
  if (memref) {
#endif
    memtrack=realloc(memtrack, sizeof(*memtrack)*memref);
    if(memref && memtrack==NULL)
      {
	guru("Aieee! Memory tracker ran out of memory!");
	exit(234);
      }
#ifdef CONFIG_EFENCE
  }
#endif
  fprintf(stderr, "-%d #%d (%ld) %p %s\n",s,memref,memamt,adr,where);
#endif
#endif
  free((void*)p);
}

#ifdef DEBUG_MEMORY
g_error g_drealloc(void **p,size_t s,const char *where) {
  int pos;
#else
g_error g_realloc(void **p,size_t s) {
#define where "Can't happen in g_realloc"
#endif
#ifdef DEBUG_ANY
  size_t from;
#endif
  void *oldp;

  if (!p) return mkerror(PG_ERRT_BADPARAM,25);

  if(s)
   {
    if (!*p)
      return g_dmalloc(p,s,where);
   }
  else
   {
    oldp=*p;
    *p=NULL;
    g_dfree(oldp,where);
    return success;
   }
#ifdef DEBUG_MEMORY
  for(pos=0;pos<memref;pos++)
    if(memtrack[pos].mem==*p)
      break;
  if(pos==memref)
   {
    guru("Attempt to g_realloc memory acquired elsewhere!\n"
	"! [%d] %p %s", s, p, where);
    return mkerror(PG_ERRT_MEMORY,25);
   }
#endif
#ifdef DEBUG_ANY
  ((size_t*)(*p))--;    /* Get the _real_ pointer so realloc will like us */
  oldp=*p;
  memamt -= (from = *((size_t*)(*p)));  /* Store original size */
  *p = realloc(*p,s+sizeof(size_t));
#else
  oldp=*p;
  *p = realloc(*p,s);
#endif
  if (!(*p))
   {
    if(oldp)	/* FIXME */
     {
      memref--;
      free(oldp);
     }
    return mkerror(PG_ERRT_MEMORY,25);
   }

#ifdef DEBUG_ANY
  *(((size_t *)(*p))++) = s;
  memamt += s;
#endif

#ifdef DEBUG_MEMORY
  memtrack[pos].mem=*p;
  memtrack[pos].size=s;
  memtrack[pos].where=where;
  fprintf(stderr, "* [%d -> %d] #%d (%ld) %p %s\n",from,s,memref,memamt,*p,where);
#endif

  return success;

}

/* The End */

