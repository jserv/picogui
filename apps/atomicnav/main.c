/* $Id: main.c,v 1.2 2002/01/07 06:28:08 micahjd Exp $
 *
 * main.c - Initialization and event loop for Atomic Navigator
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
  pgInit(argc,argv);

  browserwin_new();

  pgCustomizeSelect(selectHandler, selectBH);
  pgEventLoop();
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









