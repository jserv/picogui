/* $Id: g_error.h,v 1.1 2000/09/03 19:27:59 micahjd Exp $
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

/* Error type or'ed with error number */
typedef unsigned int g_error;

/* Error types */
#define ERRT_NONE     0x0000
#define ERRT_MEMORY   0x0100
#define ERRT_IO       0x0200
#define ERRT_NETWORK  0x0300
#define ERRT_BADPARAM 0x0400
#define ERRT_HANDLE   0x0500
#define ERRT_INTERNAL 0x0600
#define ERRT_BUSY     0x0700

#define ERRT_NOREPLY  0xF000 /* This special error type sends
				no reply packet- assumes that
				no reply is needed or that one
				will be sent seperately */

#define mkerror(type,number) ((type)|(number))
#define iserror(e)           (((e) & 0xFF00)!=ERRT_NONE)
#define errtype(e)           ((e) & 0xFF00)    /* Matches the ERRT_* */
#define neterrtype(e)        (errtype(e)>>8)   /* To send to the client */

#define sucess               ERRT_NONE

/* It's so common, let's make it a macro */
#define errorcheck           if (iserror(e)) return e;

const char *errortext(g_error e);
g_error prerror(g_error e);

#endif /* __H_GERROR */
/* The End */










