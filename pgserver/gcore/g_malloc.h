/*
 * g_malloc.h - malloc wrapper providing error handling
 * $Revision: 1.1 $ 
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#ifndef __H_GMALLOC
#define __H_GMALLOC

#include <malloc.h>
#include <g_error.h>

g_error g_malloc(void **p,size_t s);
void g_free(void *p);

#endif /* __H_GMALLOC */
/* The End */
