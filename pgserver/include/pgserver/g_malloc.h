/* $Id: g_malloc.h,v 1.5 2001/02/17 05:18:40 micahjd Exp $
 *
 * g_malloc.h - malloc wrapper providing error handling
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef __H_GMALLOC
#define __H_GMALLOC

#include <malloc.h>
#include <pgserver/g_error.h>

g_error g_malloc(void **p,size_t s);
void g_free(void *p);
g_error g_realloc(void **p,size_t s);

#ifdef DEBUG_KEYS
/* Memory allocation statistics, for CTRL-ALT-M */
extern long num_grops;    /* Number of gropnodes */
extern long num_divs;     /* Number of divnodes */
extern long num_widgets;  /* Number of widgets */
extern long num_handles;  /* Number of handles */
extern long memref;       /* Total allocations */
#endif

#ifdef DEBUG_ANY
extern long memamt;       /* Bytes of memory total */
#endif

#endif /* __H_GMALLOC */
/* The End */
