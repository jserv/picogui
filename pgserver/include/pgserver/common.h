/* $Id: common.h,v 1.5 2001/04/29 17:28:39 micahjd Exp $
 *
 * pgserver/common.h - things every file in pgserver should need,
 *                     including memory management, error handling,
 *                     and configuration info.
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

#define PGSERVER

/* Define some good data types */
typedef unsigned char    u8;
typedef signed char      s8;
typedef unsigned short   u16;
typedef signed short     s16;
typedef unsigned long    u32;
typedef signed long      s32;
#ifndef bool
typedef unsigned char    bool;
#endif

#include <pgserver/autoconf.h>
#include <pgserver/g_error.h>
#include <pgserver/g_malloc.h>

/* The End */
