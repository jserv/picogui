/* $Id: main.c,v 1.1 2001/01/05 09:13:28 micahjd Exp $
 *
 * main.c - PicoGUI Terminal (the 'p' is silent :)
 *          This handles the PicoGUI init and events
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
#include "pterm.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>

/* file descriptor of the pseudoterminal device */
int ptyfd;

/* Widgets */
pghandle wTerminal;

/****************************** Terminal event handlers ***/

/* These functions pass data back and forth between the
 * terminal widget and the pseudoterminal */

/* Recieves data from the pgBindData association */
int termInput(pghandle from,long size,char *data) {
   /* Write the input character to the subprocess */
   write(ptyfd,data,size);
}

/* FIXME: so far the only way this could work is by
   polling via the idle handler. Need a way to ask the
   client lib to watch an fd for ya.
*/
void termOutput(void) {
   int chars;
   static char buf[4096];
   while ((chars = read(ptyfd,buf,4096)) > 0) {
      pgWriteData(wTerminal,pgFromMemory(&buf,chars));
      pgSubUpdate(wTerminal);
   }

   /* Read error - we're done! */
   if (chars<0 && errno != EAGAIN)
     exit(0);
}

/****************************** Main Program ***/

int main(int argc, char *argv[],char *envp[]) {
  int childpid;

  /*** PicoGUI Initialization */
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Terminal",0);
  
  wTerminal = pgNewWidget(PG_WIDGET_TERMINAL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  pgBindData(PGDEFAULT,&termInput);
  
  /*** Start up subprocess */
  
  if ( (childpid = ptyfork(&ptyfd)) < 0 ) {
    /* I would hope this is a rare error, so don't spend much effort on it */
    pgMessageDialog(argv[0],"Error acquiring pseudoterminal",0);
    exit(1);
  }

  if (!childpid) {
    /* This is the child process */

    char *args[] = { "-sh", NULL };
    execve("/bin/sh",args,envp);
    perror("execve");
  }

  /*** These two are for the polling hack (ick. See the above FIXME) */
  {
    int argh = 1;
    ioctl(ptyfd,FIONBIO,&argh);
    pgSetIdle(50,&termOutput);
  }

  /*** Event loop */
  pgEventLoop();

  /*** Cleanup */

  /* If the child is still alive, fix that */
  kill(childpid,SIGTERM);

  return 0;
}

/* The End */
