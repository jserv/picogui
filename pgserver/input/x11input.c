/* $Id: x11input.c,v 1.4 2001/11/20 01:42:08 micahjd Exp $
 *
 * x11input.h - input driver for X11 events
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* Hooks for interfacing with the x11 video driver */
extern Display *xdisplay;    /* X display from the x11.c driver */
int x11_fd;                  /* X display's file descriptor */
void x11_expose(int x, int y, int w, int h);

g_error x11input_init(null) {
  /* Get a file descriptor for the X11 display */
  if (!xdisplay)
    return mkerror(PG_ERRT_BADPARAM,36);   /* No matching video driver */
  x11_fd = ConnectionNumber(xdisplay);
  return sucess;
}

void x11input_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if ((*n)<(x11_fd+1))
    *n = x11_fd+1;
  FD_SET(x11_fd,readfds);
}

/* Recieve and process X events */
int x11input_fd_activate(int fd) {
  int i;
  XEvent ev;
  if(fd != x11_fd) return 0;

  for (i=XPending(xdisplay);i;i--) {
    XNextEvent(xdisplay, &ev);
    switch (ev.type) {

    case Expose:
      x11_expose(ev.xexpose.x,
		 ev.xexpose.y,
		 ev.xexpose.width,
		 ev.xexpose.height);
      break;

    case MotionNotify:
      dispatch_pointing(TRIGGER_MOVE,ev.xmotion.x, ev.xmotion.y, 
			ev.xmotion.state >> 8);
      drivermessage(PGDM_CURSORVISIBLE,1,NULL);
      break;

    case ButtonPress:
      printf("Press %d %d\n",ev.xbutton.state >> 8,ev.xbutton.button);
      dispatch_pointing(TRIGGER_DOWN,ev.xbutton.x, ev.xbutton.y,
			(ev.xbutton.state >> 8) | (1 << (ev.xbutton.button-1)));
      drivermessage(PGDM_CURSORVISIBLE,1,NULL);
      break;

    case ButtonRelease:
      printf("Release %d %d\n",ev.xbutton.state >> 8,ev.xbutton.button);
      dispatch_pointing(TRIGGER_UP,ev.xbutton.x, ev.xbutton.y,
			(ev.xbutton.state >> 8) & (~(1 << (ev.xbutton.button-1))));
      drivermessage(PGDM_CURSORVISIBLE,1,NULL);
      break;


    }
  }
  return 1;
}

g_error x11input_regfunc(struct inlib *i) {
  i->init = &x11input_init;
  i->fd_init = &x11input_fd_init;
  i->fd_activate = &x11input_fd_activate;
  return sucess;
}

/* The End */
