/* $Id: pgboard.c,v 1.3 2001/05/02 05:44:06 micahjd Exp $
 *
 * pgboard.c - Onscreen keyboard for PicoGUI on handheld devices. Loads
 *             a keyboard definition file containing one or more 'patterns'
 *             with key layouts.
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

FILE *fpat;
struct mem_pattern mpat;
pghandle wCanvas;

int evtMouseDown(struct pgEvent *evt) {
   struct key_entry *k;
   short n;
   
   /* Figure out what (if anything) was clicked */
   for (k=mpat.keys,n=mpat.num_keys;n;n--,k++) {
      if (evt->e.pntr.x < k->x) continue;
      if (evt->e.pntr.x > (k->x+k->w-1)) continue;
      if (evt->e.pntr.y < k->y) continue;
      if (evt->e.pntr.y > (k->y+k->h-1)) continue;
   
      /* If we got this far, it was clicked */
      pgSendKeyInput(PG_TRIGGER_CHAR,k->key,k->mods);
      pgSendKeyInput(PG_TRIGGER_KEYDOWN,k->pgkey,k->mods);
   }
   return 0;
}

int main(int argc,char **argv) {
   /* Initialize drawing, set mapping */
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Keyboard",0);
   wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);

   /* Load a pattern */
   memset(&mpat,0,sizeof(mpat));
   fpat = fopen("examples/us_qwerty.kb","r");
   if (!fpat) {
      pgMessageDialog(*argv,"Error loading keyboard file",0);
      return 1;
   }
   if (kb_validate(fpat,&mpat)) {
      pgMessageDialog(*argv,"Invalid keyboard file",0);
      return 1;
   }
   if (kb_loadpattern(fpat,&mpat,0,wCanvas)) {
      pgMessageDialog(*argv,"Error loading keyboard pattern",0);
      return 1;
   }
   
   /* Set up an event handler */
   pgBind(wCanvas,PG_WE_PNTR_DOWN,&evtMouseDown,NULL);
   
   pgEventLoop();
   return 0;
}
