/* $Id: main.c,v 1.4 2001/01/13 09:47:37 micahjd Exp $
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

#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

#include <picogui.h>
#include "pterm.h"

#define BUFFERSIZE    1024  /* Size of output buffer */
#define UPDATE_PERIOD 50    /* Minimum milliseconds between updates */
int ptyfd;                  /* file descriptor of the pty master */
pghandle wTerminal;         /* Widgets */

/****************************** Terminal event handlers ***/

/* These functions pass data back and forth between the
 * terminal widget and the pseudoterminal */

/* Recieves data from the pgBindData association */
int termInput(pghandle from,long size,char *data) {
   /* Write the input character to the subprocess */
   write(ptyfd,data,size);
   return 0;
}

/* Terminal was resized, pass on the news */
int termResize(short event, pghandle from, long param) {
  struct winsize size;
  memset(&size,0,sizeof(size));
  size.ws_row = param & 0xFFFF;   /* Width and height are packed in param: */
  size.ws_col = param >> 16;      /* param = (width << 16) | height;       */
  ioctl(ptyfd,TIOCSWINSZ,(char *) &size);
}

/* A wrapper around PicoGUI's select() call so we can
 * check for activity on our pty too */
int mySelect(int n, fd_set *readfds, fd_set *writefds,
	     fd_set *exceptfds, struct timeval *timeout) {
  int result;
  static unsigned long previous_update;
  unsigned long this_update;
  struct timeval now;

  /* Set up our fd */
  if ((ptyfd + 1) > n)
    n = ptyfd + 1;
  FD_SET(ptyfd,readfds);

  /* Selectify things */
  result = select(n,readfds,writefds,exceptfds,timeout);

  /* Is it for us? */
  if (result>0 && FD_ISSET(ptyfd,readfds)) {
   int chars;
   static char buf[BUFFERSIZE];

   if ((chars = read(ptyfd,buf,BUFFERSIZE)) <= 0)
     exit(0);

   /* Send it to the terminal */
   pgWriteData(wTerminal,pgFromMemory(buf,chars));

   /* Get the time */
   gettimeofday(&now,NULL);
   this_update = now.tv_sec*1000 + now.tv_usec/1000;

   /* Only update if sufficient time has passed.
    * This reduces wasted updates so scrolling junk is drawn
    * all at once, not line by line */
   if ((this_update - previous_update) >= UPDATE_PERIOD) {
     pgSubUpdate(wTerminal);
     previous_update = this_update;  /* Save time :) */
   }

  }
  return result;
}

/****************************** Main Program ***/

int main(int argc, char **argv) {
  int childpid;

  /*** PicoGUI Initialization */
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Terminal",0);         /* Register app */
  
  wTerminal = pgNewWidget(PG_WIDGET_TERMINAL,0,0);   /* Make a terminal */
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  pgBindData(PGDEFAULT,&termInput);                  /* Input handler */
  pgCustomizeSelect(&mySelect);                      /* select() wrapper for output */
  pgBind(PGDEFAULT,PG_WE_RESIZE,&termResize);        /* Resize handler */
  pgFocus(PGDEFAULT);
  
  /*** Start up subprocess */

  /* Fork! */  
  if ( (childpid = ptyfork(&ptyfd)) < 0 ) {
    pgMessageDialogFmt(argv[0],0,"Error acquiring pseudoterminal:\n%s",
		       strerror(errno));
    exit(1);
  }
  if (!childpid) {
    /* This is the child process */
    execlp("/bin/sh","-sh",NULL);
    perror("Starting subprocess");
    pause();        /* Give a chance to read the error */
    _exit(127);
  }

  /*** Event loop */
  pgEventLoop();

  /*** Cleanup */
  kill(childpid,SIGTERM);  /* If the child is still alive, fix that */
  return 0;
}

/* The End */
