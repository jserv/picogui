/* $Id$
 *
 * url.c - framework for parsing and retrieving URLs
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

#include <malloc.h>
#include <string.h>
#include "url.h"
#include "protocol.h"
#include "debug.h"

struct url *active_urls;

/********************************* Methods */

/* Create an object representing this URL.
 * The URL is parsed into its components and returned.
 * Every component of the url structure is dynamically allocated,
 * so the parameter only needs to be valid for the duration of
 * this function call.
 */
struct url * url_new(struct browserwin *browser, const char *name) {
  struct url *u;
  const char *p;
  int n;
  struct protocol **h;

  /* Allocate the URL itself */
  u = (struct url *) malloc(sizeof(struct url));
  if (!u)
    return NULL;
  memset(u,0,sizeof(struct url));

  DBG("creating URL 0x%08X from \"%s\"\n",u,name);

  u->progress = -1;
  u->browser = browser;

  /* Make a local copy of the URL name */
  u->name = strdup(name);
  if (!u->name) {
    free(u);
    return NULL;
  }

  /* Is there a protocol specified? It should be at the beginning
   * of the URL, preceeding a "://"
   */
  p = strstr(name, "://");
  if (p) {
    if (p != name) {
      u->protocol = malloc(p-name+1);
      if (u->protocol) {
	strncpy(u->protocol,name,p-name);
	u->protocol[p-name] = 0;
      }
    }
    name = p+3;
  }
  
  /* Next will come the server */
  n = strcspn(name,":/#");
  if (n) {
    u->server = malloc(n+1);
    if (u->server) {
      strncpy(u->server,name,n);
      u->server[n] = 0;
    }
    name += n;
  }

  /* The port */
  if (*name == ':') {
    name++;
    u->port = atoi(name);
    p = strchr(name,'/');
    if (!p) {
      /* No more string left...
       * Just for convenience point the following checks at an empty string
       */
      name = "";
    }
    else 
      name = p;
  }

  /* Everything after the port will be considered the path
   * (as long as it's before the anchor)
   */
  n = strcspn(name,"#");
  if (n) {
    u->path = malloc(n+1);
    if (u->path) {
      strncpy(u->path,name,n);
      u->path[n] = 0;
    }
    name += n;
  }

  /* Only the anchor left */
  if (*name == '#' && name[1])
    u->anchor = strdup(name+1);

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
    url_setstatus(u,URL_STATUS_ERROR);
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
  DBG("deleting URL 0x%08X\n",u);

  if (u->handler)
    u->handler->stop(u);
  url_deactivate(u);
  if (u->name)      free(u->name);
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
  if (u->status_change)
    u->status_change(u);
  url_progress(u);
}

/* Add/remove a URL to the list of active URLs */
void url_activate(struct url *u) {
  url_deactivate(u);
  u->next = active_urls;
  active_urls = u;
}
void url_deactivate(struct url *u) {
  struct url *p;
  
  if (active_urls == u)
    active_urls = active_urls->next;
  for (p=active_urls;p;p=p->next)
    if (p->next == u)
      p->next = u->next;
}

/* Update progress indicators associated with the URL */
void url_progress(struct url *u) {
  if (u->status == URL_STATUS_DONE)  
    u->progress = 100;
  else if (u->size > 0)
    u->progress = u->bytes_received * 100 / u->size;
  else 
    u->progress = -1;

  DBG("u->progress = %d\n", u->progress);
  
  if (u->progress_change)
    u->progress_change(u);
}

/* The End */









