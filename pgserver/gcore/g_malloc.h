/* $Id: g_malloc.h,v 1.2 2000/04/24 02:38:36 micahjd Exp $
 *
 * g_malloc.h - malloc wrapper providing error handling
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

#ifndef __H_GMALLOC
#define __H_GMALLOC

#include <malloc.h>
#include <g_error.h>

g_error g_malloc(void **p,size_t s);
void g_free(void *p);

#endif /* __H_GMALLOC */
/* The End */
