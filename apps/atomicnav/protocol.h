/* $Id: protocol.h,v 1.1 2002/01/06 15:34:23 micahjd Exp $
 *
 * protocol.h - Definition of the protocol handler object
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

#ifndef __H_PROTOCOL
#define __H_PROTOCOL

#include "url.h"

struct protocol {
  /* The name of this protocol (in the URL) */
  const char *name;

  /* Call this when the URL is in the URL_STATUS_IDLE
   * state to begin connecting.
   */
  void (*connect)(struct url *u);

  /* When the URL is in the READ state, use this function
   * to read a block of data from the URL.
   */
  unsigned long (*read)(struct url *u, void *buf, unsigned long count);
  
  /* This stops all data transfer and frees all memory
   * associated with this URL connection.
   */
  void (*stop)(struct url *u);
};

/* This list is defined in protocol.c */
extern struct protocol *supported_protocols[];

#endif /* __H_PROTOCOL */

/* The End */









