/* $Id$
 *
 * pgserver/common.h - things every file in pgserver should need,
 *                     including memory management, error handling,
 *                     and configuration info.
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
 *   Pascal Bauermeister  -- Copyright (C) 2002 SMARTDATA (SA)
 * 
 */

#ifndef _H_PG_TYPES
#define _H_PG_TYPES

/******* Fixed-sized types */


/*
 * Fixed type definitions for MacOS X, Darwin, and IRIX
 * FIXME: this is a kludge
 */
#if ((defined(__APPLE__) && defined(__MACH__)) || defined(_MIPS_ISA) || defined(_WIN32))
typedef unsigned char __u8;
typedef signed char __s8;
typedef unsigned short __u16;
typedef signed short __s16;
typedef unsigned long __u32;
typedef signed long __s32;

/* Use asm/types.h on linux */
#else
#include <asm/types.h>
#endif

#ifndef u8
# define u8 __u8
#endif

#ifndef s8
# define s8 __s8
#endif

#ifndef u16
# define u16 __u16
#endif

#ifndef s16
# define s16 __s16
#endif

#ifndef u32
# define u32 __u32
#endif

#ifndef s32
# define s32 __s32
#endif

#if !defined(bool) && !defined(NO_BOOL) && !defined(__cplusplus)
typedef int bool;
#endif


#endif /* _H_PG_TYPES */

/*
   Local Variables:
   c-file-style: "smartdata"
   End:
*/
