/* $Id: p_file.c,v 1.1 2002/01/06 15:34:23 micahjd Exp $
 *
 * p_file.c - Local disk access for the Atomic Navigator web browser
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

#include <stdio.h>
#include <alloca.h>
#include <string.h>
#include "url.h"
#include "protocol.h"

/* "connect" to a file by opening it.
 * It should be nearly instantaneous, so don't bother
 * with intermediate resolving and connecting states
 */
void p_file_connect(struct url *u) {
  FILE *f;
  char *buf;
  int len;

  /* First a little workaround...
   * file:// is the protocol, and there is no server. So technically
   * there should be three forward slashes after file:, but often users
   * will just use two. This gives us a "server" that's really our first
   * directory. Additionally, the files should always be relative to the
   * root directory. Fix all this up with some concatenation...
   */
  len = 5;  /* room for nulls and slashes */
  if (u->path)
    len += strlen(u->path);
  if (u->server)
    len += strlen(u->server);
  buf = alloca(len);
  strcpy(buf,"/");
  if (u->server)
    strcat(buf,u->server);
  strcat(buf,"/");
  if (u->path)
    strcat(buf,u->path);

  printf("buf: %s\n",buf);
  f = fopen(buf,"r");
  if (!f) {
    browserwin_errormsg("Error opening file");
    return;
  }

}

unsigned long p_file_read(struct url *u, void *buf, unsigned long count) {
}

void p_file_stop(struct url *u) {
}

struct protocol p_file = {
  name: "file",
  connect: p_file_connect,
  read: p_file_read,
  stop: p_file_stop,
}; 

/* The End */









