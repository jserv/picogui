/* $Id: g_error.c,v 1.2 2000/04/24 02:38:36 micahjd Exp $
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

