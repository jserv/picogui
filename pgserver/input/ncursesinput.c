/* $Id: ncursesinput.c,v 1.20 2002/01/22 12:25:08 micahjd Exp $
 *
 * ncursesinput.h - input driver for ncurses
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

#include <pgserver/common.h>

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/appmgr.h>

#include <curses.h>


/******************************************** Implementations */

/* a shortcut to make the mapping junk smaller */
void ncursesinput_sendkey(int key) {
   dispatch_key(TRIGGER_KEYDOWN,key,0);
}

int ncursesinput_fd_activate(int fd) {
   int ch,mods;
   
   /* Keyboard activity? */
   if (fd==0) {
      switch (ch = getch()) {
	 
       case ERR:    /* Nothing yet */
	 break;

       case '\033':
	 /* Escape- the following key has the alt modifier */
	 ch = getch();
	 mods = PGMOD_ALT;

	 /* CTRL-ALT-/ seems not to work... this hack maps ALT-/ to CTRL-ALT-/ */
	 if (ch==47)
	   mods |= PGMOD_CTRL;
	 
	 /* Check for ctrl too */
	 if (ch < ' ') {
	    ch += 'a'-1;
	    mods |= PGMOD_CTRL;
	 }
	
	 /* Hack to map CTRL-Z */
	 if (ch==KEY_SUSPEND)
	   ch = PGKEY_z;
	 
	 dispatch_key(TRIGGER_KEYDOWN,ch,mods);
	 break;
	 
	 /**** Keys that must be mapped */
	 
       case KEY_SUSPEND:      dispatch_key(TRIGGER_KEYDOWN,PGKEY_z,PGMOD_CTRL); break;
       case KEY_BACKSPACE:    dispatch_key(TRIGGER_CHAR,'\b',0); break;
       case KEY_UP:           ncursesinput_sendkey(PGKEY_UP); break;
       case KEY_DOWN:         ncursesinput_sendkey(PGKEY_DOWN); break;
       case KEY_LEFT:         ncursesinput_sendkey(PGKEY_LEFT); break;
       case KEY_RIGHT:        ncursesinput_sendkey(PGKEY_RIGHT); break;
       case KEY_HOME:         ncursesinput_sendkey(PGKEY_HOME); break;
       case KEY_IC:           ncursesinput_sendkey(PGKEY_INSERT); break;
       case KEY_DC:           ncursesinput_sendkey(PGKEY_DELETE); break;
       case KEY_PPAGE:        ncursesinput_sendkey(PGKEY_PAGEUP); break;
       case KEY_NPAGE:        ncursesinput_sendkey(PGKEY_PAGEDOWN); break;
	 
       case KEY_F(1):         ncursesinput_sendkey(PGKEY_F1); break;
       case KEY_F(2):         ncursesinput_sendkey(PGKEY_F2); break;
       case KEY_F(3):         ncursesinput_sendkey(PGKEY_F3); break;
       case KEY_F(4):         ncursesinput_sendkey(PGKEY_F4); break;
       case KEY_F(5):         ncursesinput_sendkey(PGKEY_F5); break;
       case KEY_F(6):         ncursesinput_sendkey(PGKEY_F6); break;
       case KEY_F(7):         ncursesinput_sendkey(PGKEY_F7); break;
       case KEY_F(8):         ncursesinput_sendkey(PGKEY_F8); break;
       case KEY_F(9):         ncursesinput_sendkey(PGKEY_F9); break;
       case KEY_F(10):        ncursesinput_sendkey(PGKEY_F10); break;
       case KEY_F(11):        ncursesinput_sendkey(PGKEY_F11); break;
       case KEY_F(12):        ncursesinput_sendkey(PGKEY_F12); break;

       case '\t':
       case '\r':	/* control characters NOT to be sent as Ctrl+letter */
	 ncursesinput_sendkey(ch);
	 dispatch_key(TRIGGER_CHAR, ch, 0);
	 break;
	 
       default:     /* Normal key */
	 
	 /* Check for ctrl */
	 if (ch < ' ')
	   dispatch_key(TRIGGER_KEYDOWN,ch+PGKEY_a-1,PGMOD_CTRL);
	 else
	   dispatch_key(TRIGGER_CHAR,ch,0);
      }
   }
   
   /* Pass on the event if necessary */
   else
     return 0;
   return 1;
}

void ncursesinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
   if (*n<1)
     *n = 1;
   FD_SET(0,readfds);           /* stdin */
}

g_error ncursesinput_init(void) {
   /* Initialize ncurses, possibly also for a video driver's benefit... */
   initscr(); 
   start_color();
   raw(); 
   meta(stdscr, TRUE);
   noecho();
   nonl();
   intrflush(stdscr, FALSE);
   keypad(stdscr, TRUE);
   curs_set(0);   
   nodelay(stdscr,1);

   /* Force a screen update here, otherwise ncurses will impolitely clear the
    * screen next time a key is input */
   refresh();
   
   return success;
}

void ncursesinput_close(void) {
   clear(); refresh(); endwin();
}

/******************************************** Driver registration */

g_error ncursesinput_regfunc(struct inlib *i) {
   i->init = &ncursesinput_init;
   i->close = &ncursesinput_close;
   i->fd_activate = &ncursesinput_fd_activate;
   i->fd_init = &ncursesinput_fd_init;
   return success;
}

/* The End */
