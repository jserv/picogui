/* $Id: gpmremote.c,v 1.1 2001/02/14 01:42:51 micahjd Exp $
 * 
 * gpmremote.c - A mouse-only remote input driver using gpm
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

#include <picogui.h>
#include <gpm.h>

/* Nonzero to multiply mouse motion by 2^x */
#define SCALEHACK 1

int main(int argc, char **argv) {
   Gpm_Connect my_gpm;
   Gpm_Event evt;
   int trigger;
   int cx=0,cy=0;  /* Cursor */
   int savedbtn=0;
   
   /* Don't need an app, but a connection would be nice... */
   pgInit(argc,argv); 
  
   /* Connect to GPM */
   my_gpm.eventMask = GPM_MOVE | GPM_DRAG | GPM_UP | GPM_DOWN;
   my_gpm.defaultMask = 0;                 /* Pass nothing */
   my_gpm.minMod = 0;                      /* Any modifier keys */
   my_gpm.maxMod = ~0;
   if (Gpm_Open(&my_gpm,0) == -1) {
      printf("Error connecting to gpm\n");
      exit(-1);
   }
   gpm_zerobased = 1;

   /* Wait in a loop for gpm events. Normally we would need to wait for picogui
    * events, but we're not expecting any so don't bother 
    *
    * This code is mostly just taken from PicoGUI's ncursesinput driver
    */
   
   while (1) {
      if (Gpm_GetEvent(&evt) <= 0)
	  continue;
	  
      /* Maybe movement outside of window or on another VT */
      if ((evt.type & (GPM_MOVE|GPM_DRAG)) && 
	  (!evt.dx) &&
	  (!evt.dy))
	continue;
      
      /* Generate our own coordinates and fit it within the
       * video driver's screen resolution 
       *
       * FIXME: get resolution from server. It's currently
       *        hardcoded at 320x200
       */
      
      evt.x = cx + (evt.dx << SCALEHACK);
      evt.y = cy + (evt.dy << SCALEHACK);
      gpm_mx = 320;
      gpm_my = 200;
      Gpm_FitEvent(&evt);
      cx = evt.x;
      cy = evt.y;
      
      switch (evt.type & (GPM_MOVE | GPM_DRAG | GPM_UP | GPM_DOWN)) {
	 
       case GPM_MOVE:
       case GPM_DRAG:
	 trigger = PG_TRIGGER_MOVE;
	 savedbtn = evt.buttons;
	 break;
	 
       case GPM_UP:
	 trigger = PG_TRIGGER_UP;
	 evt.buttons = savedbtn &= ~evt.buttons;
	 break;
	 
       case GPM_DOWN:
	 trigger = PG_TRIGGER_DOWN;
	 savedbtn = evt.buttons;
	 break;
	 
       default:
	 return 1;
      }

      pgSendPointerInput(trigger,evt.x,evt.y,
			 ((evt.buttons>>2)&1) ||
			 ((evt.buttons<<2)&4) ||
			 (evt.buttons&2));
      pgFlushRequests();
   }

   return 0;
}
   
/* The End */
