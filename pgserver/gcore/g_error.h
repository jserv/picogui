/*
 * g_error.h - Defines a format for errors
 * $Revision: 1.1 $ 
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
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

extern g_error sucess;

g_error inline mkerror(unsigned char type, char *msg);
g_error prerror(g_error e);

#endif /* __H_GERROR */
/* The End */


