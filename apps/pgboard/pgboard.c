/* $Id: pgboard.c,v 1.5 2001/05/06 00:16:40 micahjd Exp $
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
pghandle wCanvas, wApp;

int evtMouse(struct pgEvent *evt) {
   struct key_entry *k;
   short n;
   
   /* Figure out what (if anything) was clicked */
   for (k=mpat.keys,n=mpat.num_keys;n;n--,k++) {
      if (evt->e.pntr.x < k->x) continue;
      if (evt->e.pntr.x > (k->x+k->w-1)) continue;
      if (evt->e.pntr.y < k->y) continue;
      if (evt->e.pntr.y > (k->y+k->h-1)) continue;
   
      /* If we got this far, it was clicked */
      if (evt->type == PG_WE_PNTR_DOWN) {
	 if (k->key)
	   pgSendKeyInput(PG_TRIGGER_CHAR,k->key,k->mods);
	 pgSendKeyInput(PG_TRIGGER_KEYDOWN,k->pgkey,k->mods);
      }
      else {
	 pgSendKeyInput(PG_TRIGGER_KEYUP,k->pgkey,k->mods);
      }
	 
      /* Flash the clicked key with an XOR'ed rectangle */
      pgWriteCmd(evt->from,PGCANVAS_GROP,2,PG_GROP_SETLGOP,PG_LGOP_XOR);
      pgWriteCmd(evt->from,PGCANVAS_GROPFLAGS,1,PG_GROPF_TRANSIENT);
      pgWriteCmd(evt->from,PGCANVAS_GROP,6,
		 PG_GROP_RECT,k->x,k->y,k->w,k->h,0xFFFFFF);
      pgWriteCmd(evt->from,PGCANVAS_GROPFLAGS,1,
		 PG_GROPF_TRANSIENT | PG_GROPF_COLORED);
      pgWriteCmd(evt->from,PGCANVAS_INCREMENTAL,0);
      pgSubUpdate(evt->from);
   }
   return 0;
}

int main(int argc,char **argv) {
   /* Make a 'toolbar' app */
   pgInit(argc,argv);
   wApp = pgRegisterApp(PG_APP_TOOLBAR,"Keyboard",0);
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

   /* Resize app widget */
   pgSetWidget(wApp,
	       PG_WP_SIDE,mpat.app_side,
	       PG_WP_SIZE,mpat.app_size,
	       PG_WP_SIZEMODE,mpat.app_sizemode,
	       0);
   
   if (kb_loadpattern(fpat,&mpat,0,wCanvas)) {
      pgMessageDialog(*argv,"Error loading keyboard pattern",0);
      return 1;
   }
   
   /* Set up an event handler */
   pgBind(wCanvas,PG_WE_PNTR_DOWN,&evtMouse,NULL);
   pgBind(wCanvas,PG_WE_PNTR_UP,&evtMouse,NULL);
   
   pgEventLoop();
   return 0;
}
