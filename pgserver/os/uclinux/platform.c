/* $Id: platform.c,v 1.5 2001/02/17 05:18:41 micahjd Exp $
 *
 * platforms.c - Contains platform-dependant stuff
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micah@homesoftware.com>
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
 *   Philippe Ney <philippe.ney@smartdata.ch> :
 *   initial version
 *
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch>
 * 
 */

#include <pgserver/common.h>
#include <malloc.h>

/* VSNPRINTF *********************************************************************************/

#if 0
#include "vnsprintf.c"
#else

/* We do a quick'n'dirty hack her instead of including the 'real' vsnprintf code
 * (that would make us need _ctype and _ctmp
*/

#include <stdarg.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>

static char _buf[1024];

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
  vsprintf(_buf, fmt, args);
  strncpy(buf, _buf, size);
  buf[size-1] = '\0';
}

#endif

/* CLEANUP ***********************************************************************************/

/* needed for compatibility with uclinux compiler (m68k-pic-coff-gcc) */
void _cleanup() {}

/* REALLOC ***********************************************************************************/

/* code for realloc, not provided in the libc.a of uclinux */ 
void* realloc(void* ptr, size_t size)
{
  void* new;
  size_t* old;

  if (ptr == NULL)
    return malloc(size);

  old =  (size_t*) ptr;

  if (old[-1] > size)
    return ptr;  /* old size is bigger the new size */


  new = malloc(size);
  if (new)
    memcpy(new, ptr, old[-1]);

  free(ptr);

  return new;
}

/*********************************************************************************************/
