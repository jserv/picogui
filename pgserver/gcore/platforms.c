/* $Id: platforms.c,v 1.1 2000/10/12 16:30:36 pney Exp $
 *
 * platforms.c - groups some platforms dependant functions 
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


/* Case platform is uclinux */
#ifdef UCLINUX

#include <malloc.h>

/* needed for compatibility with uclinux compiler (m68k-pic-coff-gcc) */
void _cleanup() {}

/* code for realloc, note provided in the libc.a of uclinux */ 
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

#endif /* UCLINUX */
