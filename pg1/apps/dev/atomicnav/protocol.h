/* $Id$
 *
 * protocol.h - Definition of the protocol handler object
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef __H_PROTOCOL
#define __H_PROTOCOL

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "url.h"

struct protocol {
  /* The name of this protocol (in the URL) */
  const char *name;

  /* Call this when the URL is in the URL_STATUS_IDLE
   * state to begin connecting.
   */
  void (*connect)(struct url *u);

  /* This stops all data transfer and frees all memory
   * associated with this URL connection.
   */
  void (*stop)(struct url *u);

  /* The protocol should initialize it's file descriptors to watch here */
  void (*fd_init)(struct url *u, int *n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);

  /* And this should check whether a descriptor belonging to this protocol
   * was activated, and take apropriate action.
   */
  void (*fd_activate)(struct url *u, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);
};

/* This list is defined in protocol.c */
extern struct protocol *supported_protocols[];

#endif /* __H_PROTOCOL */

/* The End */









