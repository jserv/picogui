/* $Id: gpm.c,v 1.2 2002/01/06 09:22:58 micahjd Exp $
 *
 * gpm.c - input driver for gpm
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

#ifdef DRIVER_GPM

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/appmgr.h>

#include <gpm.h>

Gpm_Event gpm_last_event;

/* the stupid gpm server scales down the input for text mode,
 * so we either have to deal with darn slow input or choppy
 * scaling cruft. */
#define SCALEHACK 2

/******************************************** Implementations */

int gpm_fd_activate(int fd) {
   int ch,mods;
   Gpm_Event evt;
   static int savedbtn = 0;
   
   /* Mouse activity? */
   if (fd==gpm_fd)
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
	     (evt.x==gpm_last_event.x) &&
	     (evt.y==gpm_last_event.y))
	   return 1;
	 
	 gpm_last_event = evt;
	 
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
   
   return success;
}

void gpm_close(void) {
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
