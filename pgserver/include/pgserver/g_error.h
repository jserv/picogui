/* $Id: g_error.h,v 1.8 2001/03/03 01:44:26 micahjd Exp $
 *
 * g_error.h - Defines a format for errors
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

#ifndef __H_GERROR
#define __H_GERROR

#include <picogui/constants.h>

/* Error type or'ed with error number */
typedef unsigned int g_error;

/* Used internally, so not in picogui/constants.h */
#define ERRT_NOREPLY  0xF000 /* This special error type sends
				no reply packet- assumes that
				no reply is needed or that one
				will be sent seperately */
#define ERRT_PASS     0xF100 /* Returned from a widget's set/get to
			      * let the generic handler handle it */

#define mkerror(type,number) ((type)|(number))
#define iserror(e)           (((e) & 0xFF00)!=PG_ERRT_NONE)
#define errtype(e)           ((e) & 0xFF00)    /* Matches the ERRT_* */

#define sucess               PG_ERRT_NONE

/* It's so common, let's make it a macro */
#define errorcheck           if (iserror(e)) return e;

const char *errortext(g_error e);
g_error prerror(g_error e);

/* "Guru" error screen only available in debugging mode */
#if defined(DEBUG_KEYS) | defined(DEBUG_WIDGET) | defined(DEBUG_EVENT) | \
    defined(DEBUG_VIDEO) | defined(DEBUG_THEME) | defined(DEBUG_NET) | \
    defined(DEBUG_MEMORY) | defined(DEBUG_INIT)
#define DEBUG_ANY
void guru(const char *fmt, ...);
#endif

#endif /* __H_GERROR */
/* The End */










