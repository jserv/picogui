/* $Id$
 *
 * gpm.c - input driver for gpm
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
 * 
 * 
 */

#include <pgserver/common.h>

#ifdef DRIVER_GPM

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/appmgr.h>

#include <gpm.h>

Gpm_Event gpm_last_event;
struct cursor *gpm_cursor = NULL;

/******************************************** Implementations */

int gpm_fd_activate(int fd) {
   Gpm_Event evt;
   
   /* Mouse activity? */
   if (fd==gpm_fd && Gpm_GetEvent(&evt) > 0) {
	 gpm_last_event = evt;
	 infilter_send_pointing(PG_TRIGGER_PNTR_RELATIVE,evt.dx,evt.dy,
				((evt.buttons>>2)&1) ||
				((evt.buttons<<2)&4) ||
				(evt.buttons&2),gpm_cursor);
	 return 1;
      }
      
   /* Pass on the event if necessary */
   else
     return 0;
}

void gpm_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
   if ((*n)<(gpm_fd+1))
     *n = gpm_fd+1;
   if (gpm_fd>0)                /* mouse */
     FD_SET(gpm_fd,readfds);
}

g_error gpm_init(void) {
   Gpm_Connect my_gpm;

   /* Connect to GPM */
   my_gpm.eventMask = GPM_MOVE | GPM_DRAG | GPM_UP | GPM_DOWN;
   my_gpm.defaultMask = 0;                 /* Pass nothing */
   my_gpm.minMod = 0;                      /* Any modifier keys */
   my_gpm.maxMod = ~0;
   if (Gpm_Open(&my_gpm,0) == -1)
     return mkerror(PG_ERRT_IO,74);
   gpm_zerobased = 1;

   return cursor_new(&gpm_cursor, NULL, -1);
}

void gpm_close(void) {
  pointer_free(-1,gpm_cursor);
  while (Gpm_Close());
}

/******************************************** Driver registration */

g_error gpm_regfunc(struct inlib *i) {
   i->init = &gpm_init;
   i->close = &gpm_close;
   i->fd_activate = &gpm_fd_activate;
   i->fd_init = &gpm_fd_init;
   return success;
}

#endif /* DRIVER_GPM */
/* The End */
