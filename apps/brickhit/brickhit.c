/* $Id: brickhit.c,v 1.1 2001/01/24 00:45:05 micahjd Exp $
 *
 * brickhit.c - PicoGUI remake of a game I made when I was really little. The
 *              original BrickHit was written in True Basic using 16-color
 *              graphics. I wrote the video libraries and started work on a
 *              sequel, BrickHit II, written in C and using Mode 0x13.
 *              I never finished that.
 *              This remake isn't really complicated but should be a
 *              reasonable facsimilie of BrickHit.
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

/* Widgets */
pghandle wToolbar,wCanvas;

/****************************** Event Handlers ***/

int evtDrawPlayfield(short event, pghandle from, long param) {
}

int evtMouseDown(short event, pghandle from, long param) {
}

/****************************** Main Program ***/

int main(int argc, char **argv) {
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"BrickHit",0);

   /*** Top-level widgets: toolbar and canvas */
   wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
   wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
   pgBind(PGDEFAULT,PG_WE_BUILD,&evtDrawPlayfield);
   pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&evtMouseDown);

   /*** Event loop */
   pgEventLoop();
   return 0;
}

/* The End */
