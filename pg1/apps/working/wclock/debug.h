/*
   Copyright (C) 2002 by Pascal Bauermeister

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with PocketBee; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Pascal Bauermeister
   Contributors:

   $Id$
*/

#ifndef __PGWCLOCK_DEBUG_H__
#define __PGWCLOCK_DEBUG_H__

#include <stdlib.h>
#include <stdio.h>

/* ------------------------------------------------------------------------- */

#define LOCAL_INFO  0
#define LOCAL_DEBUG 0
#define LOCAL_TRACE 0

/* ------------------------------------------------------------------------- */

#if LOCAL_INFO || LOCAL_DEBUG || LOCAL_TRACE
extern int ____n____;
static inline int ____i____(int x) {
  ____n____ += x;
  return ____n____;
}

static inline int ____indent____(void) {
  int i = ____i____(0) *2;
  for(; i; --i) fprintf(stderr, " ");
  return ____i____(0);
}

# define OUT(x...) { ____indent____(); fprintf(stderr,__FILE__": " x); }
#endif

#if LOCAL_INFO
# define INFO(x...) OUT(x)
#else
# define INFO(x...)
#endif

#if LOCAL_DEBUG
# define DPRINTF(x...) OUT(x)
# define WARNF(x...)   OUT(x)
#else
# define DPRINTF(x...)
# define WARNF(x...)   OUT(x)
# undef LOCAL_TRACE
# define LOCAL_TRACE 0
#endif

#if LOCAL_TRACE
# define TRACEF(x...)  OUT(x)

# define ENTER(x) \
           const char* ____fname____ = x;\
           int ____a = ____indent____(); \
           int ____b = fprintf(stderr, "<%s>\n", ____fname____); \
           int ____c = ____i____(1)

# define LEAVE \
           { \
             ____i____(-1); \
             ____indent____() ; \
             fprintf(stderr, "</%s>\n", ____fname____); \
           }

#else
# define TRACEF(x...)
# define ENTER(x) char ____dummy____
# define LEAVE
#endif

#ifdef DECLARE_DEBUG_VARS
# if LOCAL_INFO || LOCAL_DEBUG || LOCAL_TRACE
int ____n____ = 0;
# endif
#endif

/* ------------------------------------------------------------------------- */

#endif
