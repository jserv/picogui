/* $Id: pterm.c,v 1.1 2000/12/29 21:28:17 micahjd Exp $
 *
 * pterm.c - PicoGUI Terminal (the 'p' is silent :)
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

#include <unistd.h>
#include <sys/ioctl.h>

#define BUFSIZE 1024

/* Widgets */
pghandle wTerminal;

/* Pipes for the subprocess's output and its input */
int outpipe,inpipe;

int termInput(short event,pghandle from,long param) {
   char c = param;

   /* Write the input character to the subprocess */
   write(inpipe,&c,1);
   
   /* Also echo to the terminal */
   pgWriteTo(wTerminal,pgFromMemory(&c,1));
   pgSubUpdate(wTerminal);
}

void termOutput(void) {
   int chars;
   static char buf[BUFSIZE];
   
   while ((chars = read(outpipe,buf,BUFSIZE)) > 0) {
      pgWriteTo(wTerminal,pgFromMemory(&buf,chars));
      pgSubUpdate(wTerminal);
   }
}

int main(int argc, char *argv[],char *envp[])
{
   int pair_in[2],pair_out[2];
   int argh=1;
   
   /*** PicoGUI Initialization */
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Terminal",0);
      
   wTerminal = pgNewWidget(PG_WIDGET_TERMINAL,0,0);
   pgSetWidget(PGDEFAULT,
	       PG_WP_SIDE,PG_S_ALL,
	       0);
   pgBind(PGDEFAULT,PG_WE_ACTIVATE,&termInput);
   
   /*** Start up subprocess */
   
   pipe(pair_in);
   pipe(pair_out);
   inpipe = pair_in[1];
   outpipe = pair_out[0];
   ioctl(outpipe,FIONBIO,&argh);
   if (!fork()) {
      char *args[] = { "/bin/asmsh", NULL };
      
      dup2(pair_out[1],1);
      dup2(pair_out[1],2);
      dup2(pair_in[0],0);
      execve(args[0],args,envp);
   }

   /*** Event loop */
   pgSetIdle(20,&termOutput);
   pgEventLoop();
   return 0;
}

/* The End */
