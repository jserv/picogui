/* $Id$
 *
 * main.c - Initialization and event loop for Atomic Navigator
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

#include <picogui.h>
#include "browserwin.h"
#include "url.h"
#include "protocol.h"

/* The PicoGUI client lib doesn't care about writefds or exceptfds, but we do */
fd_set static_wfds, static_efds;

int selectHandler(int n, fd_set *readfds, fd_set *writefds,
		  fd_set *exceptfds, struct timeval *timeout);
void selectBH(int result, fd_set *readfds);

int main(int argc, char **argv) {
  pghandle win;
  int run_eventloop = 0;
  pgInit(argc,argv);

  /* Find the browser window */
  win = pgFindWidget(BROWSER_NAME);

  /* Need to create a new one? */
  if (!win) {
    win = browserwin_new()->wApp;
    run_eventloop = 1;
  }

  /* Send a URL if one was specified on the command line */
  if (argv[1])
    browserwin_command(win, "URL", argv[1]);
  
  if (run_eventloop) {
    pgCustomizeSelect((pgselecthandler) selectHandler, (pgselectbh) selectBH);
    pgEventLoop();
  }
  else
    pgFlushRequests();
}

/* Call fd_init on all activated URLs */
int selectHandler(int n, fd_set *readfds, fd_set *writefds,
		  fd_set *exceptfds, struct timeval *timeout) {
  struct url *u;

  FD_ZERO(&static_efds);
  FD_ZERO(&static_wfds);
    
  for (u=active_urls;u;u=u->next)
    u->handler->fd_init(u,&n,readfds,&static_wfds,&static_efds);

  return select(n,readfds,&static_wfds,&static_efds,timeout);
}

/* Call fd_activate on all active URLs */
void selectBH(int result, fd_set *readfds) {
  struct url *u;
  for (u=active_urls;u;u=u->next)
    u->handler->fd_activate(u,readfds,&static_wfds,&static_efds);
}

/* The End */









