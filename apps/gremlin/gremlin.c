/* $Id: gremlin.c,v 1.1 2001/04/10 00:51:19 micahjd Exp $
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

int main(int argc, char **argv) {
   int c;
   unsigned long i,number = 1000000;
   unsigned int gremlin = 0;
   time_t start,now,eta;
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
      if (i)
	eta = number * now / i;
      else
	eta = 0;
      
      /* Use unbuffered output or it looks bad */
      write(1,stats,sprintf(stats,"\r %c -- #%-10lu ---"
			    " Running %02d:%02d:%02d ---"
			    " Remaining %02d:%02d:%02d ",
			    spinner[(i>>6)&3],i,now/3600,(now%3600)/60,
			    now%60,eta/3600,(eta%3600)/60,eta%60));

      /* Move the mouse */
      cx = rand() % mi.xres;
      cy = rand() % mi.yres;
      pgSendPointerInput(PG_TRIGGER_MOVE,cx,cy,0);
      pgUpdate();
      
      /* Clickski! */
      if ((rand()%100) < 80) {
	 pgSendPointerInput(PG_TRIGGER_DOWN,cx,cy,1);
	 pgUpdate();
	 pgSendPointerInput(PG_TRIGGER_UP,cx,cy,0);
	 pgUpdate();
      }
      
      /* Some common keys */
      if ((rand()%100) < 5) {
	 pgSendKeyInput(PG_TRIGGER_KEYDOWN,PGKEY_y,0);
	 pgUpdate();
	 pgSendKeyInput(PG_TRIGGER_KEYUP,PGKEY_y,0);
	 pgSendKeyInput(PG_TRIGGER_CHAR,'y',0);
	 pgUpdate();
      }
      if ((rand()%100) < 2) {
	 pgSendKeyInput(PG_TRIGGER_KEYDOWN,PGKEY_n,0);
	 pgUpdate();
	 pgSendKeyInput(PG_TRIGGER_KEYUP,PGKEY_n,0);
	 pgSendKeyInput(PG_TRIGGER_CHAR,'n',0);
	 pgUpdate();
      }
      if ((rand()%100) < 6) {
	 pgSendKeyInput(PG_TRIGGER_KEYDOWN,PGKEY_RETURN,0);
	 pgUpdate();
	 pgSendKeyInput(PG_TRIGGER_KEYUP,PGKEY_RETURN,0);
	 pgUpdate();
      }

           
      /* Wait */
      if ((rand()%100) < 20)
	 usleep(1000);
   
   }
   
   return 0;
}
   
/* The End */
