/* $Id: ncursesinput.c,v 1.1 2001/01/14 23:03:11 micahjd Exp $
 *
 * ncursesinput.h - input driver for ncurses
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifdef DRIVER_NCURSESINPUT

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>

#include <curses.h>

#define POLL_USEC 100

/******************************************** Implementations */

void ncursesinput_poll(void) {
   int ch;
   switch (ch = getch()) {
      
    case ERR:    /* Nothing yet */
      return;

    default:     /* Normal key */
      dispatch_key(TRIGGER_CHAR,ch,0);
   }
}

g_error ncursesinput_init(void) {
   /* Make getch nonblocking */
   nodelay(stdscr,1);
   return sucess;
}

/* Polling time for the input driver */ 
void ncursesinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
   timeout->tv_sec = 0;
   timeout->tv_usec = POLL_USEC;
}

/******************************************** Driver registration */

g_error ncursesinput_regfunc(struct inlib *i) {
   i->init = &ncursesinput_init;
   i->poll = &ncursesinput_poll;
   i->fd_init = &ncursesinput_fd_init;
   return sucess;
}

#endif /* DRIVER_NCURSESINPUT */
/* The End */
