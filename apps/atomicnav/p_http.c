/* $Id: p_http.c,v 1.1 2002/01/06 15:34:23 micahjd Exp $
 *
 * p_http.c - HTTP protocol for the Atomic Navigator web browser
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

#include "url.h"
#include "protocol.h"

void p_http_connect(struct url *u) {
}

unsigned long p_http_read(struct url *u, void *buf, unsigned long count) {
}

void p_http_stop(struct url *u) {
}

struct protocol p_http = {
  name: "http",
  connect: p_http_connect,
  read: p_http_read,
  stop: p_http_stop,
}; 

/* The End */









