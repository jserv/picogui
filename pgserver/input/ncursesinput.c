/* $Id: ncursesinput.c,v 1.3 2001/01/15 04:18:27 micahjd Exp $
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

#ifdef DRIVER_NCURSESINPUT

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>

#include <curses.h>
#include <gpm.h>

Gpm_Event ncurses_last_event;

/******************************************** Implementations */

int ncursesinput_fd_activate(int fd) {
   int ch;
   Gpm_Event evt;
   static int savedbtn = 0;
   
   /* Keyboard activity? */
   if (fd==3) {
      switch (ch = getch()) {
	 
       case ERR:    /* Nothing yet */
	 break;
	 
       default:     /* Normal key */
	 dispatch_key(TRIGGER_CHAR,ch,0);
      }
   }
   
   /* Mouse activity? */
   else if (fd==gpm_fd)
      if (Gpm_GetEvent(&evt) > 0) {
	 int trigger;

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

   /* Make getch nonblocking */
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
