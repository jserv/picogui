/* $Id: pgboard.c,v 1.1 2001/05/02 02:25:43 micahjd Exp $
  *
  * pgboard.c - Onscreen keyboard for PicoGUI on handheld devices
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

#include <picogui.h>

int main(int argc,char **argv) {
   pgcontext gc;
   
   /* Initialize drawing, set mapping */
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Keyboard",0);
   pgNewWidget(PG_WIDGET_CANVAS,0,0);
   gc = pgNewCanvasContext(PGDEFAULT,PGFX_PERSISTENT);
   pgWriteCmd(PGDEFAULT,PGCANVAS_GROP,6,PG_GROP_SETMAPPING,
	      0,0,30,4,PG_MAP_SCALE);
   
   pgSlab(gc,0,1,30);
   pgSlab(gc,0,2,30);
   pgSlab(gc,0,3,30);
   pgBar(gc,1,0,4);
   
   pgEventLoop();
}
