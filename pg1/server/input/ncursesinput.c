/* $Id$
 *
 * ncursesinput.h - input driver for ncurses
 * 
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * Lalo Martins <lalo@laranja.org>  --  mouse support
 * 
 */

#include <curses.h>
/* ncurses defines bool */
#define NO_BOOL

#include <pgserver/common.h>

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/appmgr.h>

/******************************************** Implementations */

static WINDOW *ncursesinput_window;
struct cursor *ncursesinput_cursor = NULL;
#ifdef DRIVER_GPM
extern struct cursor *gpm_cursor;
#endif

/* a shortcut to make the mapping junk smaller */
void ncursesinput_sendkey(int key) {
   infilter_send_key(PG_TRIGGER_KEYDOWN,key,0);
}

void ncursesinput_getmouse() {
  MEVENT event;
  static int btnstate = 0;
  /* on my machine this only works on xterm. Then again my libgpm seems to be
   * a bit on the crazy side...    -- Lalo
   */

  getmouse(&event);

  /* might have moved - let the filters figure it out */
  infilter_send_pointing(PG_TRIGGER_MOVE, event.x, event.y, btnstate, ncursesinput_cursor);

  switch (event.bstate) {
  case BUTTON1_RELEASED:
    btnstate &= ~1;
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON1_PRESSED:
    btnstate |= 1;
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON1_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 1, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON1_DOUBLE_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 1, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 1, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON1_TRIPLE_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 1, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 1, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 1, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;

  case BUTTON2_RELEASED:
    btnstate &= ~2;
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON2_PRESSED:
    btnstate |= 2;
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON2_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 2, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON2_DOUBLE_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 2, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 2, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON2_TRIPLE_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 2, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 2, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 2, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;

  case BUTTON3_RELEASED:
    btnstate &= ~3;
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON3_PRESSED:
    btnstate |= 3;
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON3_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 3, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON3_DOUBLE_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 3, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 3, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON3_TRIPLE_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 3, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 3, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 3, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;

  case BUTTON4_RELEASED:
    btnstate &= ~4;
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON4_PRESSED:
    btnstate |= 4;
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON4_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 4, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON4_DOUBLE_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 4, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 4, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  case BUTTON4_TRIPLE_CLICKED:
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 4, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 4, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_DOWN, event.x, event.y, btnstate | 4, ncursesinput_cursor);
    infilter_send_pointing(PG_TRIGGER_UP, event.x, event.y, btnstate, ncursesinput_cursor);
    break;
  }
}

int ncursesinput_fd_activate(int fd) {
   int ch,mods;
   
   /* Keyboard activity? */
   if (fd==0) {
      switch (ch = wgetch(ncursesinput_window)) {
	 
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
	 
	 infilter_send_key(PG_TRIGGER_KEYDOWN,ch,mods);
	 break;

	 /*** Mouse events */
      case KEY_MOUSE:
	ncursesinput_getmouse();
	break;

	 /**** Keys that must be mapped */
	 
       case KEY_SUSPEND:      infilter_send_key(PG_TRIGGER_KEYDOWN,PGKEY_z,PGMOD_CTRL); break;
       case KEY_BACKSPACE:    infilter_send_key(PG_TRIGGER_CHAR,'\b',0); break;
       case KEY_UP:           ncursesinput_sendkey(PGKEY_UP); break;
       case KEY_DOWN:         ncursesinput_sendkey(PGKEY_DOWN); break;
       case KEY_LEFT:         ncursesinput_sendkey(PGKEY_LEFT); break;
       case KEY_RIGHT:        ncursesinput_sendkey(PGKEY_RIGHT); break;
       case KEY_HOME:         ncursesinput_sendkey(PGKEY_HOME); break;
       case KEY_END:          ncursesinput_sendkey(PGKEY_END); break;
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
	 infilter_send_key(PG_TRIGGER_CHAR, ch, 0);
	 break;
	 
       default:     /* Normal key */
	 
	 /* Check for ctrl */
	 if (ch < ' ')
	   infilter_send_key(PG_TRIGGER_KEYDOWN,ch+PGKEY_a-1,PGMOD_CTRL);
	 else
	   infilter_send_key(PG_TRIGGER_CHAR,ch,0);
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
   ncursesinput_window = initscr(); 
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
   
#ifdef DRIVER_GPM
   if (gpm_cursor != NULL)
     /* don't give the user two cursors */
     return 1;
#endif

   mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

   return cursor_new(&ncursesinput_cursor, NULL, -1);
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
