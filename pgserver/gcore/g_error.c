/*
 * g_error.h - Defines a format for errors
 * $Revision: 1.1 $ 
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <g_error.h>

g_error sucess = {ERRT_NONE,NULL};

g_error mkerror(unsigned char type, char *msg) {
  g_error e = {type,msg};
  return e;
}

g_error prerror(g_error e) {
  if (e.type == ERRT_NONE) return sucess;
  if (!e.msg) e.msg = "?";
  printf("*** ERROR (");
  switch (e.type) {
  case ERRT_MEMORY: printf("MEMORY"); break;
  case ERRT_IO: printf("IO"); break;
  case ERRT_NETWORK: printf("NETWORK"); break;
  case ERRT_BADPARAM: printf("BADPARAM"); break;
  case ERRT_HANDLE: printf("HANDLE"); break;
  case ERRT_INTERNAL: printf("INTERNAL"); break;
  default: printf("?");
  }
  printf(") : %s\n",e.msg);
  return e;
}

/* The End */

