/* $Id: ncursesinput.c,v 1.12 2001/02/14 05:13:18 micahjd Exp $
 *
 * ncursesinput.h - input driver for ncurses
 * 
 * Note that even though ncurses can provide a wrapper around gpm,
 * most distros get this all munged up, so we connect to gpm directly.
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

#include <pgserver/common.h>

#ifdef DRIVER_NCURSESINPUT

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/appmgr.h>

#include <curses.h>
#include <gpm.h>

Gpm_Event ncurses_last_event;

/* the stupid gpm server scales down the input for text mode,
 * so we either have to deal with darn slow input or choppy
 * scaling cruft. */
#define SCALEHACK 2

/******************************************** Implementations */

/* a shortcut to make the mapping junk smaller */
void ncursesinput_sendkey(int key) {
   dispatch_key(TRIGGER_KEYDOWN,key,0);
}

int ncursesinput_fd_activate(int fd) {
   int ch,mods;
   Gpm_Event evt;
   static int savedbtn = 0;
   
   /* Keyboard activity? */
   if (fd==3) {
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
	 
       default:     /* Normal key */
	 
	 /* Check for ctrl */
	 if (ch < ' ')
	   dispatch_key(TRIGGER_KEYDOWN,ch+PGKEY_a-1,PGMOD_CTRL);
	 else
	   dispatch_key(TRIGGER_CHAR,ch,0);
      }
   }
   
   /* Mouse activity? */
   else if (fd==gpm_fd)
      if (Gpm_GetEvent(&evt) > 0) {
	 int trigger;

	 /* Generate our own coordinates and fit it within the
	  * video driver's screen resolution */
	 if (vid->xres>200) {    /* For stupid scale hack */
	    evt.x = cursor->x + (evt.dx << SCALEHACK);
	    evt.y = cursor->y + (evt.dy << SCALEHACK);
	    gpm_mx = vid->xres;
	    gpm_my = vid->yres;
	    Gpm_FitEvent(&evt);
	 }

	 /* Maybe movement outside of window or on another VT */
	 if ((evt.type & (GPM_MOVE|GPM_DRAG)) && 
	     (evt.x==ncurses_last_event.x) &&
	     (evt.y==ncurses_last_event.y))
	   return 1;
	 
	 ncurses_last_event = evt;
	 
	 switch (evt.type & (GPM_MOVE | GPM_DRAG | GPM_UP | GPM_DOWN)) {
	    
	  case GPM_MOVE:
	  case GPM_DRAG:
	    trigger = TRIGGER_MOVE;
	    savedbtn = evt.buttons;
	    break;
	    
	  case GPM_UP:
	    trigger = TRIGGER_UP;
	    evt.buttons = savedbtn &= ~evt.buttons;
	    break;
	    
	  case GPM_DOWN:
	    trigger = TRIGGER_DOWN;
	    savedbtn = evt.buttons;
	    break;
	    
	  default:
	    return 1;
	 }

	 dispatch_pointing(trigger,evt.x,evt.y,
			   ((evt.buttons>>2)&1) ||
			   ((evt.buttons<<2)&4) ||
			   (evt.buttons&2));
      }
      
   /* Pass on the event if necessary */
   else
     return 0;
   return 1;
}

void ncursesinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
   if ((*n)<(gpm_fd+1))
     *n = gpm_fd+1;
   FD_SET(3,readfds);           /* stdin */
   if (gpm_fd>0)                /* mouse */
     FD_SET(gpm_fd,readfds);
}

g_error ncursesinput_init(void) {
   Gpm_Connect my_gpm;

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

   /* Connect to GPM */
   my_gpm.eventMask = GPM_MOVE | GPM_DRAG | GPM_UP | GPM_DOWN;
   my_gpm.defaultMask = 0;                 /* Pass nothing */
   my_gpm.minMod = 0;                      /* Any modifier keys */
   my_gpm.maxMod = ~0;
   if (Gpm_Open(&my_gpm,0) == -1)
     return mkerror(PG_ERRT_IO,74);
   gpm_zerobased = 1;
   
   return sucess;
}

void ncursesinput_close(void) {
   while (Gpm_Close());
   clear(); refresh(); endwin();
}

/******************************************** Driver registration */

g_error ncursesinput_regfunc(struct inlib *i) {
   i->init = &ncursesinput_init;
   i->close = &ncursesinput_close;
   i->fd_activate = &ncursesinput_fd_activate;
   i->fd_init = &ncursesinput_fd_init;
   return sucess;
}

#endif /* DRIVER_NCURSESINPUT */
/* The End */
