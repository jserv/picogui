/* $Id: url.h,v 1.1 2002/01/06 12:32:41 micahjd Exp $
 *
 * url.h - framework for parsing and retrieving URLs
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

#ifndef __H_URL
#define __H_URL


/********************************* Structure */

struct url {
  char *url;
  
  /* Parsed from the name (zero/NULL for none) */
  char *protocol;
  char *user;
  char *password;
  char *server;
  char *path;      /* Everything after the server name before the anchor */
  char *filename;  /* Just the file */
  char *anchor;
  int port;
};

/********************************* Methods */

/* Create an object representing this URL.
 * The URL is parsed into its components and returned.
 * Every component of the url structure is dynamically allocated,
 * so the parameter only needs to be valid for the duration of
 * this function call.
 */
struct url * url_new(const char *url);

/* Free all memory associated with the URL.
 * If a transfer is in progress, abort the transfer.
 */
void url_delete(struct url *u);

/* Start downloading data from the URL, call the supplied function
 * when the transfer status changes. It is possible for the
 * callback to call url_delete().
 */
void url_download(struct url *u, void (*callback)(struct url *u));

#endif /* __H_BROWSERWIN */

/* The End */









