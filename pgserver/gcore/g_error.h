/* $Id: g_error.h,v 1.3 2000/06/11 17:59:18 micahjd Exp $
 *
 * g_error.h - Defines a format for errors
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

#ifndef __H_GERROR
#define __H_GERROR

#include <stdio.h>

typedef struct {
  unsigned char type;   /* Basic type of error */
  const char *msg;   /* Static error message detailing the situation */
} g_error;

/* Error types */
#define ERRT_NONE     0
#define ERRT_MEMORY   1
#define ERRT_IO       2
#define ERRT_NETWORK  3
#define ERRT_BADPARAM 4
#define ERRT_HANDLE   5
#define ERRT_INTERNAL 6
#define ERRT_BUSY     7

extern g_error sucess;

g_error inline mkerror(unsigned char type, char *msg);
g_error prerror(g_error e);

#endif /* __H_GERROR */
/* The End */


