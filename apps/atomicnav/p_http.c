/* $Id: p_http.c,v 1.3 2002/01/07 19:25:50 micahjd Exp $
 *
 * p_http.c - Local disk access for the Atomic Navigator web browser
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
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <dirent.h>
#include <signal.h>
#include <netdb.h>
#include <stdio.h>

#include "url.h"
#include "protocol.h"
#include "browserwin.h"

struct http_data {
  int fd;
  struct hostent *he;
  struct sockaddr_in server_addr;
  FILE *out;
};

void p_http_connect(struct url *u) {
  struct http_data *hd;

  /* Create our http_data */
  hd = malloc(sizeof(struct http_data));
  memset(hd,0,sizeof(struct http_data));
  u->extra = hd;

  /* HTTP defaults */
  if (!u->port)
    u->port = 80;
  if (!u->path)
    u->path = strdup("/");
  
  /* Resolve hostname */
  url_setstatus(u,URL_STATUS_RESOLVING);
  hd->he = gethostbyname(u->server);
  if (!hd->he) {
    browserwin_errormsg(u->browser,"The web server could not be found<br>(DNS error)");
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }    
    
  /* Create TCP socket */
  hd->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (!hd->fd) {
    browserwin_errormsg(u->browser,
			"Can't create TCP/IP socket. Has TCP/IP"
			" networking been compiled into the kernel?");
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }    

  /* Connect socket */
  url_setstatus(u,URL_STATUS_CONNECTING);
  memset(&hd->server_addr,0,sizeof(hd->server_addr));
  hd->server_addr.sin_family = AF_INET;
  hd->server_addr.sin_port = htons(u->port);
  hd->server_addr.sin_addr = *((struct in_addr *)hd->he->h_addr);
  if (connect(hd->fd, (struct sockaddr *)&hd->server_addr, sizeof(struct sockaddr)) == -1) {
    browserwin_errormsg(u->browser,
			"Can't connect to the web server. "
			"The server could be down or not accepting connections.");
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }    
  hd->out = fdopen(hd->fd,"w");

  /* Send HTTP headers */
  url_setstatus(u,URL_STATUS_WRITE);
  fprintf(hd->out,
	  "GET %s HTTP/1.0\r\n"
	  "User-Agent: %s\r\n"
	  "\r\n",
	  u->path,
	  BROWSER_TITLE);
  fflush(hd->out);

  /* Ready to receive response, leave that to the select loop */
  url_setstatus(u,URL_STATUS_READ);
  url_activate(u);
}

void p_http_stop(struct url *u) {
  struct http_data *hd = (struct http_data *) u->extra;
  
  url_deactivate(u);
  if (hd) {
    if (hd->out)
      fclose(hd->out);
    if (hd->fd)
      close(hd->fd);
    
    free(hd);
    hd = NULL;
  }

  switch (u->status) {
  case URL_STATUS_RESOLVING:
  case URL_STATUS_CONNECTING:
  case URL_STATUS_WRITE:
  case URL_STATUS_READ:
    url_setstatus(u,URL_STATUS_STOPPED);
  }
}

void p_http_fd_init(struct url *u, int *n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds) {
  struct http_data *hd = (struct http_data *) u->extra;

  if (u->status == URL_STATUS_READ) {
    if (hd->fd+1 > *n)
      *n = hd->fd+1;
    FD_SET(hd->fd, readfds);
  }
}

void p_http_fd_activate(struct url *u, fd_set *readfds, fd_set *writefds, fd_set *exceptfds) {
  /*

  size_t s;
  size_t chunk;

  if (FD_ISSET(u->fd,readfds)) {
    s = read(u->fd,u->data + u->size_received, u->size - u->size_received);
    if (!s) {
      browserwin_errormsg(u->browser,"Error reading from http");
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
  */
}

struct protocol p_http = {
  name: "http",
  connect: p_http_connect,
  stop: p_http_stop,
  fd_init: p_http_fd_init,
  fd_activate: p_http_fd_activate,
}; 

/* The End */









