/* $Id: p_file.c,v 1.3 2002/01/07 09:05:51 micahjd Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <alloca.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>

#include "url.h"
#include "protocol.h"

void p_file_connect(struct url *u) {
  char *buf;
  int len, fd;
  struct stat st;

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

  /* Just in case it's a network device or a slow disk and fopen() blocks */
  url_setstatus(u,URL_STATUS_CONNECTING);

  if (stat(buf,&st)) {
    browserwin_errormsg(u->browser,strerror(errno));
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }    

  u->size = st.st_size;

  fd = open(buf,O_RDONLY);
  if (fd<=0) {
    browserwin_errormsg(u->browser,strerror(errno));
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }    

  /* Allocate buffer to read the file into */
  u->data = malloc(u->size);
  if (!u->data) {
    browserwin_errormsg(u->browser,"Not enough memory to allocate buffer for file");
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }

  /* Ready to read from file */
  u->proto_extra = (void*) fd;
  url_setstatus(u,URL_STATUS_READ);
  url_activate(u);
}

void p_file_stop(struct url *u) {
  int fd = (int) u->proto_extra;
  url_deactivate(u);
  if (fd)
    close(fd);
  if (u->status == URL_STATUS_READ)
    url_setstatus(u,URL_STATUS_STOPPED);
}

void p_file_fd_init(struct url *u, int *n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds) {
  int fd = (int) u->proto_extra;
  
  if (u->status == URL_STATUS_READ) {
    if (fd+1 > *n)
      *n = fd+1;
    FD_SET(fd, readfds);
  }
}

/* Normally we should be able to get the whole file in one read, but just in case...
 */
void p_file_fd_activate(struct url *u, fd_set *readfds, fd_set *writefds, fd_set *exceptfds) {
  int fd = (int) u->proto_extra;
  size_t s;
  size_t chunk;

  if (FD_ISSET(fd,readfds)) {
    s = read(fd,u->data + u->size_received, u->size - u->size_received);
    if (!s) {
      browserwin_errormsg(u->browser,"Error reading from file");
      url_setstatus(u,URL_STATUS_ERROR);
      url_deactivate(u);
    }
    if (s > 0)
      u->size_received += s;
    if (u->size_received == u->size) {
      url_setstatus(u, URL_STATUS_DONE);
      url_deactivate(u);
    }
    url_progress(u);
  }
}

struct protocol p_file = {
  name: "file",
  connect: p_file_connect,
  stop: p_file_stop,
  fd_init: p_file_fd_init,
  fd_activate: p_file_fd_activate,
}; 

/* The End */









