/* $Id$
 *
 * g_malloc.h - malloc wrapper providing error handling
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

#ifndef __H_GMALLOC
#define __H_GMALLOC

/* FIXME: Check for Mac OS X using autoconf */
#if (defined(__APPLE__) && defined(__MACH__)) // Mac OS X and Darwin 
#include <sys/types.h>
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

#include <pgserver/g_error.h>

g_error memoryleak_trace(void);

#ifdef DEBUG_MEMORY
g_error g_dmalloc(void **p,size_t s,const char *where);
void g_dfree(const void *p,const char *where);
g_error g_drealloc(void **p,size_t s,const char *where);
#define g_malloc(p,s) g_dmalloc(p,s,__FUNCTION__ " in " __FILE__)
#define g_free(p) g_dfree(p,__FUNCTION__ " in " __FILE__)
#define g_realloc(p,s) g_drealloc(p,s,__FUNCTION__ " in " __FILE__)
#else
g_error g_malloc(void **p,size_t s);
void g_free(const void *p);
g_error g_realloc(void **p,size_t s);
#define g_dmalloc(p,s,where) g_malloc(p,s)
#define g_dfree(p,where) g_free(p)
#define g_drealloc(p,s,where) g_realloc(p,s)
#endif

#ifdef DEBUG_KEYS
/* Memory allocation statistics, for CTRL-ALT-M */
extern int num_grops;    /* Number of gropnodes */
extern int num_divs;     /* Number of divnodes */
extern int num_widgets;  /* Number of widgets */
extern int num_handles;  /* Number of handles */
extern int memref;       /* Total allocations */
extern s32 grop_zombie_count;  /* borrowed from grop.c */
#endif

#ifdef DEBUG_ANY
extern long memamt;       /* Bytes of memory total */
#endif

#ifdef DEBUG_MEMORY
extern struct memtrack_struct {
  void *mem;
  size_t size;
  const char *where;
} *memtrack;
#endif

#endif /* __H_GMALLOC */
/* The End */
