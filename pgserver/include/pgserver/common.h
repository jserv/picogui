/* $Id: common.h,v 1.10 2002/01/16 15:25:21 bauermeister Exp $
 *
 * pgserver/common.h - things every file in pgserver should need,
 *                     including memory management, error handling,
 *                     and configuration info.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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


/* We'll need this if we don't already have it... */
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#include <pgserver/types.h>
#include <pgserver/autoconf.h>
#include <pgserver/g_error.h>
#include <pgserver/g_malloc.h>
/* The End */
