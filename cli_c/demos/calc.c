/* $Id: calc.c,v 1.1 2000/11/18 07:45:17 micahjd Exp $
 *
 * calc.c - Calculator application for PicoGUI
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
 * Contributors: Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 * 
 */

#include <picogui.h>

static char *buttongrid[] = {
   NULL,"1","2","3",
   NULL,"4","5","6",
   NULL,"7","8","9",
   NULL,NULL
};

pghandle wDisplay;

int btnHandler(short event,pghandle from,long param) {
   pgReplaceTextFmt(wDisplay,"%s%s",
		    pgGetString(pgGetWidget(wDisplay,PG_WP_TEXT)),
		    pgGetPayload(from));
}

int main(int argc, char **argv) {
   int i;
   char *s;
   pghandle wRow = 0,wButton = 0;
   
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Calculator",0);
   
   wDisplay = pgNewWidget(PG_WIDGET_LABEL,0,0);
   pgSetWidget(PGDEFAULT,
	       PG_WP_TRANSPARENT,0,
	       PG_WP_TEXT,pgNewString("0"),
	       0);
   
   /* Buttons */

   for (i=0;;i++) {
      s = buttongrid[i];
      if (s) {   /* Add a button to the row */
	 wButton = pgNewWidget(PG_WIDGET_BUTTON,
			       wButton ? PG_DERIVE_AFTER : PG_DERIVE_INSIDE,
			       wButton ? wButton : wRow);
	 pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString(s),0);
	 pgSetPayload(PGDEFAULT,s);
      }
      else {     /* Add a row */
	 if (!buttongrid[i+1])  /* Two consecutive NULLs, exit */
	   break;
         wRow = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_AFTER,
			    wRow ? wRow : wDisplay);
	 pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_BOTTOM,0);
	 wButton = 0;
      }
   }

   /* Use one handler for all the buttons */
   pgBind(PGBIND_ANY,PG_WE_ACTIVATE,&btnHandler);
   
   pgEventLoop();
   return 0;
}


/* The End */
