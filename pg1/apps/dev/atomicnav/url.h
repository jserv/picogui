/* $Id$
 *
 * url.h - framework for parsing and retrieving URLs
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

#ifndef __H_URL
#define __H_URL

#include <picogui.h>

/********************************* Structure */

struct protocol;
struct browserwin;

struct url {
  char *name;

  /* Browser window this URL is associated with.
   * Useful for error reporting via browserwin_errormsg()
   * and for use by the callback.
   */
  struct browserwin *browser;

  /* Callbacks set by the URL owner */
  void (*status_change)(struct url *u);
  void (*progress_change)(struct url *u);
  
  /* Parsed from the name (zero/NULL for none) */
  char *protocol;
  char *user;
  char *password;
  char *server;
  char *path;      /* Everything after the server name before the anchor */
  char *filename;  /* Just the file */
  char *anchor;
  int port;

  /* This is a pointer to the current protocol handler for this URL */
  struct protocol *handler;

  /* Connection status for this URL (a URL_STATUS_* constant) */
  int status;

  /* Progress. -1 for unknown, or a number between 0 and 100 */
  int progress;

  /* By the time status is URL_STATUS_DONE these fields should
   * be filled in, reflecting the completed page. Usually 'size' will
   * also be available during loading so that progress can be estimated.
   */
  char *type;
  unsigned long size;
  struct pgmemdata data;

  /* Amount of data read so far */
  unsigned long bytes_received;

  /* If this URL is currently loading it will be in a linked list
   * of URLs to check in our pgCustomizeSelect handler
   */
  struct url *next;

  /* Extra data may be needed by the protocol handler */
  void *extra;
};

/********************************* Constants */

#define URL_STATUS_IDLE       0
#define URL_STATUS_RESOLVING  1
#define URL_STATUS_CONNECTING 2
#define URL_STATUS_WRITE      3
#define URL_STATUS_READ       4
#define URL_STATUS_DONE       5   /* Url done loading into the buffer */
#define URL_STATUS_ERROR      6   /* Protocol handler already called browserwin_errormsg() */
#define URL_STATUS_STOPPED    7
#define URL_STATUS_CACHED     8   /* Using cached copy */

/********************************* Methods */

/* Create an object representing this URL.
 * The URL is parsed into its components and returned.
 * Every component of the url structure is dynamically allocated,
 * so the parameter only needs to be valid for the duration of
 * this function call.
 */
struct url * url_new(struct browserwin *browser, const char *name);

/* Free all memory associated with the URL.
 * If a transfer is in progress, abort the transfer.
 */
void url_delete(struct url *u);

/* This is called by the protocol handler to change the
 * URL's status. It calls the url->callback.
 */
void url_setstatus(struct url *u, int status);

/* Add/remove a URL to the list of active URLs */
void url_activate(struct url *u);
void url_deactivate(struct url *u);

/* Update progress indicators associated with the URL */
void url_progress(struct url *u);

extern struct url *active_urls;

#endif /* __H_BROWSERWIN */

/* The End */









