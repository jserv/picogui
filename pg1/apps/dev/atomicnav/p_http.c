/* $Id$
 *
 * p_http.c - HTTP client implementation for the Atomic Navigator web browser
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
#include "debug.h"

/* Some tweakables.. */
#define DEFAULT_PAGE_BUFFER    10240  /* This will grow as needed */
#define HEADER_LINE_BUFFER     2048   /* Needs to be big enough to hold a cookie */
#define GROW_PAGE_BUFFER       4096   /* Grow by the read amount plus this */

struct http_data {
  int fd;
  struct hostent *he;
  struct sockaddr_in server_addr;
  FILE *out;
  char *buffer;
  char *headerline;
  unsigned long buffer_size;
};

/********************************* Utilities */

/* Process one HTTP header */
void p_http_header(struct url *u, const char *name, const char *value) {
  DBG("%s = \"%s\"\n",name,value);

  if (!strcmp(name,"Content-Type") && !u->type) 
    u->type = strdup(value);

  else if (!strcmp(name,"Content-Length"))
    u->size = atoi(value);

  else if (!strcmp(name,"Location")) {
    if (strcmp(value,u->name))
      browserwin_command(u->browser->wApp, "URL", value);
  }
}

char *my_fgets( char *s, int size, int fd) {
  /* similar to fgets(), but using a int file descriptor, and not the buffered FILE type */
  int i;

  i = 0;

  /* read the input file */
  /* remark : test size - 1 because we leave a place for the '\0' terminal */ 
  while(( i < size - 1 ) && ( read(fd,s+i, 1) == 1 ) && ( s[i] != '\n' )) {

    i++;

  }

  if(( i == 0 ) && s[i] != '\n' ) {
    /* noting readed */
    return NULL;
  }

  /* put the '\0' terminal */
  s[i + 1] = 0;

  return s;

}

/********************************* Methods */

void p_http_connect(struct url *u) {
  struct http_data *hd;
  char *p;
  struct in_addr saddr;

  /* Create our http_data */
  hd = malloc(sizeof(struct http_data));
  memset(hd,0,sizeof(struct http_data));
  u->extra = hd;

  /* HTTP defaults */
  if (!u->port)
    u->port = 80;
  if (!u->path)
    u->path = strdup("/");

  /* First try it as aaa.bbb.ccc.ddd. */
  saddr.s_addr = inet_addr(u->server);
  if (saddr.s_addr == -1) {

    /* Resolve hostname */
    url_setstatus(u,URL_STATUS_RESOLVING);
    hd->he = gethostbyname(u->server);
    if (!hd->he) {
      browserwin_errormsg(u->browser,"The web server could not be found<br>(DNS error)");
      url_setstatus(u,URL_STATUS_ERROR);
      return;
    }    
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

  if (saddr.s_addr == -1) {
    hd->server_addr.sin_addr = *((struct in_addr *)hd->he->h_addr);
  } else {
    hd->server_addr.sin_addr.s_addr = saddr.s_addr;
  }

  if (connect(hd->fd, (struct sockaddr *)&hd->server_addr, sizeof(struct sockaddr)) == -1) {
    browserwin_errormsg(u->browser,
			"Can't connect to the web server. "
			"The server could be down or not accepting connections.");
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }    
  hd->out = fdopen(hd->fd,"w");

  /* Send HTTP headers */\
  url_setstatus(u,URL_STATUS_WRITE);
  fprintf(hd->out,
	  "GET %s HTTP/1.0\r\n"
	  "User-Agent: %s\r\n"
	  "\r\n",
	  u->path,
	  USER_AGENT);
  fflush(hd->out);

  /* Receive HTTP headers with buffered I/O */
  url_setstatus(u,URL_STATUS_READ);
  hd->headerline = malloc(HEADER_LINE_BUFFER);
  if (!hd->headerline) {
      browserwin_errormsg(u->browser, "The HTTP headers are incomplete");
      url_setstatus(u,URL_STATUS_ERROR);
      return;
  }

  for (;;) {
    /* Get one header line */
    if (!my_fgets(hd->headerline, HEADER_LINE_BUFFER, hd->fd)) {
      browserwin_errormsg(u->browser, "The HTTP headers are incomplete");
      url_setstatus(u,URL_STATUS_ERROR);
      return;
    }

    /* The fgets separates lines with \n, but there might be a stray \r
     * at the end since most web servers send \r\n
     */
    if (p = strchr(hd->headerline,'\r'))
      *p = 0;
    if (p = strchr(hd->headerline,'\n'))
      *p = 0;
    
    /* Separate into name and value */
    p = strchr(hd->headerline,':');
    if (p) {
      *p = 0;
      p++;
      /* There should be a space after the colon, if so chop if off */
      if (*p == ' ')
	p++;
    }
    else 
      p = "";

    if (!*hd->headerline)
      break;

    p_http_header(u,hd->headerline,p);
  };
  free(hd->headerline);
  hd->headerline = NULL;

  /* Allocate our initial buffer. If we didn't get a content-size header,
   * use a hardcoded default size.
   */
  if (u->size)
    hd->buffer_size = u->size;
  else
    hd->buffer_size = DEFAULT_PAGE_BUFFER;
  hd->buffer = malloc(hd->buffer_size);
  if (!hd->buffer) {
    browserwin_errormsg(u->browser, "Unable to allocate memory to hold the web page");
    url_setstatus(u,URL_STATUS_ERROR);
    return;
  }    
  DBG("malloc'ed %d bytes for page buffer\n", hd->buffer_size);

  /* Yay, start reading the page's content asynchronously */
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
    if (hd->buffer)
      free(hd->buffer);
    if (hd->headerline)
      free(hd->headerline);
    
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
  struct http_data *hd = (struct http_data *) u->extra;
  size_t n_available;
  size_t n_read;

  if (FD_ISSET(hd->fd,readfds)) {

    /* Check how much data is actually available */
    ioctl(hd->fd, FIONREAD, &n_available);
    if (n_available <= 0) {
      /* Done */
      
      url_deactivate(u);
      u->data = pgFromMemory(hd->buffer, u->bytes_received);
      url_setstatus(u, URL_STATUS_DONE);
      return;
    }

    /* Will we need more space to read this? */
    if (u->bytes_received + n_available > hd->buffer_size) {
      hd->buffer_size = u->bytes_received + n_available + GROW_PAGE_BUFFER;
      hd->buffer = realloc(hd->buffer, hd->buffer_size);
      if (!hd->buffer) {
	browserwin_errormsg(u->browser,"Can't increase page buffer size");
	url_setstatus(u,URL_STATUS_ERROR);
	url_deactivate(u);
      }
      DBG("resized page buffer to %d\n",hd->buffer_size);
    }

    n_read = read(hd->fd,hd->buffer + u->bytes_received, n_available);
    if (!n_read) {
      browserwin_errormsg(u->browser,"Error reading from web server");
      url_setstatus(u,URL_STATUS_ERROR);
      url_deactivate(u);
    }
    if (n_read > 0)
      u->bytes_received += n_read;

    url_progress(u);
  }
}

struct protocol p_http = {
  name: "http",
  connect: p_http_connect,
  stop: p_http_stop,
  fd_init: p_http_fd_init,
  fd_activate: p_http_fd_activate,
}; 

/* The End */









