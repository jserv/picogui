/* $Id: url.c,v 1.2 2002/01/06 15:34:23 micahjd Exp $
 *
 * url.c - framework for parsing and retrieving URLs
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

#include <malloc.h>
#include <string.h>
#include "url.h"
#include "protocol.h"

/********************************* Methods */

/* Create an object representing this URL.
 * The URL is parsed into its components and returned.
 * Every component of the url structure is dynamically allocated,
 * so the parameter only needs to be valid for the duration of
 * this function call.
 */
struct url * url_new(struct browserwin *browser, const char *url) {
  struct url *u;
  const char *p;
  int n;
  struct protocol **h;

  /* Allocate the URL itself */
  u = malloc(sizeof(struct url));
  if (!u)
    return NULL;
  memset(u,0,sizeof(struct url));

  u->progress = -1;
  u->browser = browser;

  /* Make a local copy of the entire URL */
  u->url = strdup(url);
  if (!u->url) {
    free(u);
    return NULL;
  }

  printf("1 u->url = %s, url = %s, u->protocol = %s\n",u->url,url,u->protocol);

  /* Is there a protocol specified? It should be at the beginning
   * of the URL, preceeding a "://"
   */
  p = strstr(url, "://");
  if (p) {
    if (p != url) {
      u->protocol = malloc(p-url+1);
      if (u->protocol) {
	strncpy(u->protocol,url,p-url);
	u->protocol[p-url] = 0;
      }
    }
    url = p+3;
  }
  
  printf("2 u->url = %s, url = %s, u->protocol = %s\n",u->url,url,u->protocol);

  /* Next will come the server */
  n = strcspn(url,":/#");
  if (n) {
    u->server = malloc(n+1);
    if (u->server) {
      strncpy(u->server,url,n);
      u->server[n] = 0;
    }
    url += n;
  }

  /* The port */
  if (*url == ':') {
    url++;
    u->port = atoi(url);
    p = strchr(url,'/');
    if (!p)
      return u;
    url = p;
  }

  /* Everything after the port will be considered the path
   * (as long as it's before the anchor)
   */
  n = strcspn(url,"#");
  if (n) {
    u->path = malloc(n+1);
    if (u->path) {
      strncpy(u->path,url,n);
      u->path[n] = 0;
    }
    url += n;
  }

  /* Only the anchor left */
  if (*url == '#' && url[1])
    u->anchor = strdup(url+1);

  /* No protocol?
   * See if we can guess...
   * Use http normally, but if there's no server use file
   */
  if (!u->protocol) {
    if (u->server)
      u->protocol = strdup("http");
    else
      u->protocol = strdup("file");
  }

  /* Figure out which protocol handler to use */
  h = supported_protocols;
  while (*h && strcasecmp(u->protocol,(*h)->name))
    h++;
  if (!*h) {
    /* Didn't find a protocol */
    browserwin_errormsg(browser,"Unsupported protocol.");
    url_delete(u);
    return NULL;
  }
  u->handler = *h;

  return u;
}

/* Free all memory associated with the URL.
 * If a transfer is in progress, abort the transfer.
 */
void url_delete(struct url *u) {
  if (u->handler)
    u->handler->stop(u);
  if (u->url)       free(u->url);
  if (u->protocol)  free(u->protocol);
  if (u->user);     free(u->user);
  if (u->password); free(u->password);
  if (u->server);   free(u->server);
  if (u->path);     free(u->path);
  if (u->filename); free(u->filename);
  if (u->anchor);   free(u->anchor);
  if (u->type);     free(u->type);
  free(u);
}

/* This is called by the protocol handler to change the
 * URL's status. It calls the url->callback.
 */
void url_setstatus(struct url *u, int status) {
  u->status = status;
  if (u->callback)
    u->callback(u);
}

/* The End */









