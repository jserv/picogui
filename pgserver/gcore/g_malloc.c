/* $Id: g_malloc.c,v 1.2 2000/04/24 02:38:36 micahjd Exp $
 *
 * g_malloc.c - malloc wrapper providing error handling
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

