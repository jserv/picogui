/* $Id$
 *
 * g_error.h - Defines a format for errors
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
#define ERRT_PASS     0xF100 /* Returned from a widget's set/get to
			      * let the generic handler handle it */

#define mkerror(type,number) ((type)|(number))
#define iserror(e)           (((e) & 0xFF00)!=PG_ERRT_NONE)
#define errtype(e)           ((e) & 0xFF00)    /* Matches the ERRT_* */

#define success               PG_ERRT_NONE

/* PicoGUI exception handling, if an error was generated in
 * a called function, return it
 */
#ifdef CONFIG_ERROR_TRACE
#include <stdio.h>	/* printf */
extern const char *errtrace_fmt;
#define errorcheck           if (iserror(e)) {  \
                               fprintf(stderr, errtrace_fmt,__FUNCTION__,__FILE__,__LINE__); \
                               prerror(e); return e; \
                             }
#else
#define errorcheck           if (iserror(e)) return e;
#endif

/* Look up a text error message, with internationalization */
const char *errortext(g_error e);

/* Print the error message for 'e' and return 'e' */
g_error prerror(g_error e);

/* Load a table of internationalized error messages from disk */
g_error errorload(const char *filename);

/* "Guru" error screen only available in debugging mode */
#if defined(DEBUG_KEYS) | defined(DEBUG_WIDGET) | defined(DEBUG_EVENT) | \
    defined(DEBUG_VIDEO) | defined(DEBUG_THEME) | defined(DEBUG_NET) | \
    defined(DEBUG_MEMORY) | defined(DEBUG_INIT)
#define DEBUG_ANY
#define HAS_GURU
void guru(const char *fmt, ...);
#else
/* Allow guru to be used, just don't compile it to anything */
#ifndef _MSC_VER
#define guru(fmt, args...)
#else
inline void guru(...) {}
#endif /* WIN32 */
#endif

#endif /* __H_GERROR */

/* The End */










