/* $Id$
 * 
 * main.c - Main source file for pgremote, a networked PicoGUI input driver
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
#include <vga.h>

int main(int argc, char **argv) {
   int oldbutton=0,newbutton;
   pgInit(argc,argv);  
   vga_init();
   vga_setmousesupport(1);
   vga_setmode(G320x200x256);
   
   while (1) {
      newbutton = mouse_getbutton();
      if (oldbutton!=newbutton) {
	 if (newbutton>oldbutton)
	   pgSendPointerInput(PG_TRIGGER_DOWN,
			      mouse_getx(),mouse_gety(),1);
	 else
	   pgSendPointerInput(PG_TRIGGER_UP,
			      mouse_getx(),mouse_gety(),0);
	 oldbutton = newbutton;
      }
      else
	pgSendPointerInput(PG_TRIGGER_MOVE,
			   mouse_getx(),mouse_gety(),oldbutton);
      pgFlushRequests();
      mouse_waitforupdate();
   }
   
   return 0;
}
   
/* The End */
