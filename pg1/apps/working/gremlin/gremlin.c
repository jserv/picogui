/* $Id$
 * 
 * gremlin.c - This is similar to the PalmOS app with the similar name. It
 *             sends random (but repeatable) input events to the server in an
 *             attempt to crash apps or pgserver.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2001 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <stdio.h>    /* printf / puts */
#include <stdlib.h>   /* random numbers */
#include <time.h>     /* For time() to seed RNG and report running time */
#include <unistd.h>   /* getopt */

/* Wrappers used to convert this to input filters */
void sendPointerInput(int trigger,int x,int y,int btn);
void sendKeyInput(int trigger,int key,int mods);

int main(int argc, char **argv) {
   int c;
   unsigned long i,number = 1000000;
   unsigned int gremlin = 0;
   time_t start,now,last=0,eta=0;
   struct pgmodeinfo mi;
   char spinner[] = "/-\\|";
   char stats[80];
   int cx,cy;  /* Cursor position */

   /* Init and get video mode */
   pgInit(argc,argv); 
   mi = *pgGetVideoMode();

   /* Process command line */
   while ( (c = getopt(argc,argv,"g:n:")) != -1 )
     switch (c) {
	
      case 'g':
	gremlin = atoi(optarg);
	break;
	
      case 'n':
	number = atol(optarg);
	break;
	
      default:        /* Help */
	puts("PicoGUI gremlin (pgui.sourceforge.net)\n\n"
	     "usage pgserver [-g gremlin] [-n number]\n\n"
	     "  g gremlin  : Specify a gremlin number to repeat previous results\n"
	     "               Defaults is derived from the current time\n"
	     "  n number   : Specify the number of iteratons\n"
	     "               Default is a large number\n"
	     "\n"
	     "  PicoGUI gremlin is similar to the PalmOS program of similar name.\n"
	     "  It feeds random events to the server, acting as a relentles but\n"
	     "  quite unintelligent beta tester.");
	exit(1);
     }
   
   /* Initialize to a (sometimes) predictable state */
   if (!gremlin)
     gremlin = time(NULL) & 0xFFFFF ;
   srand(gremlin);
   printf("Summoning gremlin #%lu for %lu frobs...\n",gremlin,number);

   /* Start the clock */
   start = time(NULL);

   /* The gremlin loop */
   for (i=0;i<number;i++) {
      /* Get the time and calculate the ETA */
      now = time(NULL) - start;
      if (i && now>last) {  /* Only update ETA every second to prevent round errors
			     * causing wild fluctuation */
	 eta = (number * now / i) - now;
	 last = now;
      }
      
      /* Use unbuffered output or it looks bad */
      write(1,stats,sprintf(stats,"\r %c --- #%-10lu ---"
			    " Running %02d:%02d:%02d ---"
			    " Remaining %02d:%02d:%02d ",
			    spinner[(i>>6)&3],i+1,now/3600,(now%3600)/60,
			    now%60,eta/3600,(eta%3600)/60,eta%60));

      /* Move the mouse */
      cx = rand() % mi.xres;
      cy = rand() % mi.yres;
      sendPointerInput(PG_TRIGGER_MOVE,cx,cy,0);
      pgUpdate();
      
      /* Clickski! */
      if ((rand()%100) < 80) {
	 sendPointerInput(PG_TRIGGER_DOWN,cx,cy,1);
	 pgUpdate();

	 /* Drag */
	 if ((rand()%100) < 5) {
	    cx = rand() % mi.xres;
	    cy = rand() % mi.yres;
	    sendPointerInput(PG_TRIGGER_MOVE,cx,cy,1);
	    pgUpdate();
	 }
	 
	 sendPointerInput(PG_TRIGGER_UP,cx,cy,0);
	 pgUpdate();
      }
      
      /* Some common keys */
      if ((rand()%100) < 5) {
	 sendKeyInput(PG_TRIGGER_KEYDOWN,PGKEY_y,0);
	 pgUpdate();
	 sendKeyInput(PG_TRIGGER_KEYUP,PGKEY_y,0);
	 sendKeyInput(PG_TRIGGER_CHAR,'y',0);
	 pgUpdate();
      }
      if ((rand()%100) < 2) {
	 sendKeyInput(PG_TRIGGER_KEYDOWN,PGKEY_n,0);
	 pgUpdate();
	 sendKeyInput(PG_TRIGGER_KEYUP,PGKEY_n,0);
	 sendKeyInput(PG_TRIGGER_CHAR,'n',0);
	 pgUpdate();
      }
      if ((rand()%100) < 30) {
	 sendKeyInput(PG_TRIGGER_KEYDOWN,PGKEY_TAB,0);
	 pgUpdate();
	 sendKeyInput(PG_TRIGGER_KEYUP,PGKEY_TAB,0);
	 pgUpdate();
      }
      if ((rand()%100) < 5) {
	 sendKeyInput(PG_TRIGGER_KEYDOWN,PGKEY_SPACE,0);
	 pgUpdate();
	 sendKeyInput(PG_TRIGGER_KEYUP,PGKEY_SPACE,0);
	 pgUpdate();
      }

           
      /* Wait */
      if ((rand()%100) < 20)
	 usleep(1000);
   
   }

   printf("\nDone. Remember to check for memory leaks with CTRL-ALT-M!\n");
   
   return 0;
}

void sendPointerInput(int trigger,int x,int y,int btn) {
  union pg_client_trigger trig;
  memset(&trig,0,sizeof(trig));

  trig.content.type        = trigger;
  trig.content.u.mouse.x   = x;
  trig.content.u.mouse.y   = y;
  trig.content.u.mouse.btn = btn;

  pgInFilterSend(&trig);
}

void sendKeyInput(int trigger,int key,int mods) {
  union pg_client_trigger trig;
  memset(&trig,0,sizeof(trig));

  trig.content.type        = trigger;
  trig.content.u.kbd.key   = key;
  trig.content.u.kbd.mods  = mods;

  pgInFilterSend(&trig);
}
   
/* The End */
