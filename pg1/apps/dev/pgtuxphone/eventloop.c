/* $Id$
 *
 * eventloop.c - Event loop for graphical tuxphone
 *
 * Copyright (C) 2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 */

#include <picogui.h>
#include <sys/types.h>
#include <unistd.h>

#include "phonecall.h"
#include "tuxclient.h"

/* Custom select handler to process tuxphone events */
int mySelect(int n, fd_set *readfds, fd_set *writefds,
	     fd_set *exceptfds, struct timeval *timeout) {

  if ((phone_fd + 1) > n)
    n = phone_fd + 1;
  FD_SET(phone_fd,readfds);

  return select(n,readfds,writefds,exceptfds,timeout);
}

/* Bottom-half for the select, allowed to make PicoGUI calls */
void mySelectBH(int result, fd_set *readfds) {
  /* Is it for us? */
  if (result<=0 || !FD_ISSET(phone_fd,readfds)) 
    return;

  phone_get_event(phone_fd);
}

int main(int argc, char **argv) {
  /* Connect to PicoGUI and tuxphone */
  pgInit(argc,argv);

  phone_fd = phone_open();
  if (phone_fd <= 0) {
    pgMessageDialog("TuxPhone Error","Unable to connect to tuxphone daemon",0);
    return 1;
  }

  /* Build the GUI */
  init_call_info();
  init_keypad();

  /* Set up to recieve phone events */
  phone_register_events(phone_fd, ALL_EVENT_MASK);
  pgCustomizeSelect(&mySelect,&mySelectBH);

  pgEventLoop();
}

/* The End */
