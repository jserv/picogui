/* $Id$
 *
 * p_file.c - Local disk access for the Atomic Navigator web browser
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <picogui.h>

#include "url.h"
#include "protocol.h"

void p_file_connect(struct url *u) {
  char *buf;
  int len;
  struct stat st;
  FILE *f;

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

  buf = malloc(len);
  if( buf == NULL ) {
    /* problem allocating buff */
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }

  strcpy(buf,"/");
  if (u->server)
    strcat(buf,u->server);
  strcat(buf,"/");
  if (u->path)
    strcat(buf,u->path);

  /* Just in case it's a network device or a slow disk and open() blocks */
  url_setstatus(u,URL_STATUS_CONNECTING);

  /* Stat it so we can get the file's size and type */
  if (stat(buf,&st)) {
    browserwin_errormsg(u->browser,strerror(errno));
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }    
  u->size = st.st_size;

  /* Normal file */
  f = fopen(buf,"r");

  /* release the memory used by buf */
  free( buf );
  
  if (!f) {
    browserwin_errormsg(u->browser,strerror(errno));
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }    
  u->data = pgFromStream(f, u->size);
  fclose(f);

  u->bytes_received = u->size;
  url_setstatus(u,URL_STATUS_DONE);

}

struct protocol p_file = {
  name: "file",
  connect: p_file_connect,
  stop: NULL,
  fd_init: NULL,
  fd_activate: NULL,
}; 

/* The End */









