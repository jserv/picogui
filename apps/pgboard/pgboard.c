/* $Id: pgboard.c,v 1.2 2001/05/02 04:37:57 micahjd Exp $
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

#include <stdio.h>
#include <picogui.h>
#include "kbfile.h"

int main(int argc,char **argv) {
   FILE *f;
   struct mem_pattern pat;
   pghandle c;
   
   /* Initialize drawing, set mapping */
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Keyboard",0);
   c = pgNewWidget(PG_WIDGET_CANVAS,0,0);

   /* Load a pattern */
   memset(&pat,0,sizeof(pat));
   f = fopen("examples/us_qwerty.kb","r");
   if (!f) {
      pgMessageDialog(*argv,"Error loading keyboard file",0);
      return 1;
   }
   if (kb_validate(f,&pat)) {
      pgMessageDialog(*argv,"Invalid keyboard file",0);
      return 1;
   }
   if (kb_loadpattern(f,&pat,0,c)) {
      pgMessageDialog(*argv,"Error loading keyboard pattern",0);
      return 1;
   }
   
   pgEventLoop();
}
