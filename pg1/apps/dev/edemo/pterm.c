/* $Id$
 *
 * pterm.c - based on "pterm", PicoGUI Terminal
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * Modified by: Daniele Pizzoni - Ascensit s.r.l. - Italy
 * tsho@ascensit.com - auouo@tin.it
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
 */

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grp.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

#include <picogui.h>
#include "pterm.h"

#define BUFFERSIZE    1024  /* Size of output buffer */
int ptyfd;                  /* file descriptor of the pty master */
pghandle wTerminal,wPanel;  /* Widgets */
char *title = "Terminal";
int terminalHasFont = 0;
int bSwapDeleteBackspace = 0;   /* if you need to swap delete and backspace */

int childpid = 0;
/****************************** UI functions ***/

/* Handler for the menu
 */
/* int btnFont(struct pgEvent *evt) { */
/*   pghandle fnt; */

/*   fnt = pgFontPicker("Terminal Font"); */

/*   if (fnt) { */
/*     if (terminalHasFont) */
/*       pgDelete(pgGetWidget(wTerminal,PG_WP_FONT)); */
/*     pgSetWidget(wTerminal,PG_WP_FONT,fnt,0); */
/*     terminalHasFont = 1; */
/*   }   */
/* } */

/****************************** Terminal event handlers ***/

/* These functions pass data back and forth between the
 * terminal widget and the pseudoterminal */

/* Recieves data from the pgBind association */
int termInput(struct pgEvent *evt) {

    /* switch the delete and backspace if needed */
    if (bSwapDeleteBackspace && evt->e.data.size) {
	switch (*(evt->e.data.pointer)) {
	case PGKEY_BACKSPACE:
	    *(evt->e.data.pointer) = PGKEY_DELETE;
	    break;
	case PGKEY_DELETE:
	    *(evt->e.data.pointer) = PGKEY_BACKSPACE;
	    break;
	}
    }

   /* Write the input character to the subprocess */
   write(ptyfd,evt->e.data.pointer,evt->e.data.size);
   return 0;
}

/* Terminal was resized, pass on the news */
int termResize(struct pgEvent *evt) {
  struct winsize size;

  /* If we're rolled up just stay calm... */
  if (evt->e.size.w <= 0 || evt->e.size.h <= 0)
     return 0;
   
  memset(&size,0,sizeof(size));
  size.ws_row = evt->e.size.h;
  size.ws_col = evt->e.size.w;
  ioctl(ptyfd,TIOCSWINSZ,(char *) &size);

  /* Update the title bar */
/*   pgReplaceTextFmt(wPanel,"%s (%dx%d)",title,evt->e.size.w,evt->e.size.h); */

  return 0;
}

/* A wrapper around PicoGUI's select() call so we can
 * check for activity on our pty too */
int mySelect(int n, fd_set *readfds, fd_set *writefds,
	     fd_set *exceptfds, struct timeval *timeout) {

  /* Set up our fd */
  if ((ptyfd + 1) > n)
    n = ptyfd + 1;
  FD_SET(ptyfd,readfds);

  /* Selectify things */
  return select(n,readfds,writefds,exceptfds,timeout);
}

/* Bottom-half for the select, allowed to make PicoGUI calls */
void mySelectBH(int result, fd_set *readfds) {
  int chars;
  static char buf[BUFFERSIZE];
  
  /* Is it for us? */
  if (result<=0 || !FD_ISSET(ptyfd,readfds)) 
    return;

  if ((chars = read(ptyfd,buf,BUFFERSIZE)) <= 0)
    return;
       
  /* Send it to the terminal */
  pgWriteData(wTerminal,pgFromMemory(buf,chars));
  pgSubUpdate(wTerminal);
}

/****************************** Main Program ***/

void sigChild(int x) {
  exit(0);
}

void printHelp(void) {
  puts("PicoGUI terminal (pgui.sourceforge.net)\n\n"
       "usage: pterm [options] [subprocess [subprocess args]]\n"
       "If no subprocess is given, \"/bin/sh\" is run as a login shell\n"
       "\n"
       "  -f size      Try to use a font of the given size (in pixels)\n"
       "  -t title     Set the window title\n"
       "  -s           swap delete and backspace\n"
       "  -p           Pause after subprocess exits");
 
  exit(1);
}


/* int pterm(int argc, char **argv) { */
int pterm(pghandle widget) {
  unsigned char fontsize;
  unsigned char noexit;

  fontsize = 0;
  title = "edemo terminal";
  bSwapDeleteBackspace = 0;
  noexit = 1;
   
  wTerminal = widget;

  if (fontsize) {
     pgSetWidget(wTerminal,PG_WP_FONT,
		 pgNewFont(0,fontsize,PG_FSTYLE_FIXED),0);
     terminalHasFont = 1;
  }
  pgBind(wTerminal,PG_WE_DATA,&termInput,NULL);      /* Input handler */
  pgCustomizeSelect(&mySelect, (void*)mySelectBH);          /* select() wrapper for output */
  pgBind(wTerminal, PG_WE_RESIZE, &termResize,NULL);   /* Resize handler */
/*   pgFocus(wTerminal); */
  
  /* Scroll bar */
//  pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_BEFORE,wTerminal);
//  pgSetWidget(PGDEFAULT,
//	      PG_WP_BIND,wTerminal,
//	      0);

  /*** Start up subprocess */

  /* So we know when the subprocess exits... */
  if (!noexit)
     signal(SIGCHLD,&sigChild);

  /* Fork! */
  {
    char * cmd [] = {
      "/bin/sh", "-rsh", NULL 
    };

/*     if ((childpid = ptyfork (& ptyfd, argv [0] ? argv : cmd)) < 0 ) { */
/*       pgMessageDialogFmt (argv [0] ? argv[0] : "Shell", */
/* 			  0, "Error acquiring pseudoterminal:\n%s", */
/* 			  strerror (errno)); */
/*       exit(1); */
/*     } */
/*   } */

    if ((childpid = ptyfork (& ptyfd,  cmd)) < 0 ) {
      pgMessageDialogFmt ("Shell",
			  0, "Error acquiring pseudoterminal:\n%s",
			  strerror (errno));
      exit(1);
    }
  }

  /*** Event loop */
/*    pgEventLoop(); */
  return 0;
   
  /*** Cleanup */
  kill(childpid,SIGTERM);  /* If the child is still alive, fix that */
  return 0;
}

void pterm_delete(void)
{
    if (childpid) {
	printf("killing child\n");
	kill(childpid,SIGTERM);  /* If the child is still alive, fix that */
    }
}

int ptyfork(int * ptyfd, char ** cmd) {

  char fname[11];
  char *chr1,*chr2;
  int master,slave;
  int pid;

#ifdef HAVE_FORKPTY

  if ( (pid = forkpty (& master, NULL, NULL, NULL)) < 0 ) {

#else /* HAVE_FORKPTY */

  strcpy(fname,"/dev/ptyAB");

  /* Find an available pair */
  for (chr1 = "abcdefpqrstuvwxyzPQRST";*chr1;chr1++) {
    fname[8] = *chr1;      /* Replace the A */

    for (chr2 = "0123456789abcdef";*chr2;chr2++) {
      fname[9] = *chr2;    /* Replace the B */
      
      /* Try to open each */
      if ((master = open(fname,O_RDWR)) < 0) {
	if (errno == ENOENT) {
	  /* maybe there is no such group of ttys, check for the next one */
	  break;
	}
	else
	  continue;
      }

      /* We have the master open. Change the 'pty' to a 'tty' */
      fname[5] = 't';

      /* See if we can open the tty. (The permissions might be fubar) */
      if ((slave = open(fname,O_RDWR)) < 0) {
	fname[5] = 'p';
	close(master);
	continue;         /* Argh. Try again */
      }
      else
	close(slave);     /* Good, but save this for later */

      /* Fork! */
      if ( (pid = fork()) < 0 ) {

#endif /* HAVE_FORKPTY */

#ifdef DEBUG
	perror("fork");
#endif
	return -1;
      }
      
      if (pid) {
	if (ptyfd) {
	  * ptyfd = master;
	}

	return pid;
      }

      /* Child */

#ifndef HAVE_FORKPTY	

      /* Shed our old controlling terminal and get a new session */
      if (setsid() < 0) {
#ifdef DEBUG
	perror("setsid");
#endif
	return -1;
      }
      
      /* Try to make the device our own, but this won't work unless the
       * terminal is setuid root */
      {
	struct group *grptr;
	int gid;
	
	if ((grptr = getgrnam("tty")) != NULL)
	  gid = grptr->gr_gid;
	else
	  gid = -1;  /* no tty group */
	
	chown(fname,getuid(),gid);
	chmod(fname,S_IRUSR | S_IWUSR | S_IWGRP);  /* Good permissions for a tty */
      }
      
      /* Open the slave device */
      if ( (slave = open(fname,O_RDWR)) < 0) {
	close(master);
#ifdef DEBUG
	perror("opening slave pty");
#endif
	return -1;
      }
      
      /* Child doesn't need the master pty anymore */
      close(master);
      
      /* Acquire a controlling terminal */
      ioctl(slave,TIOCSCTTY,NULL);
      
      /* Make the slave pty our stdin/out/err */
      if (dup2(slave, STDIN_FILENO) != STDIN_FILENO) {
#ifdef DEBUG
	perror("dup2 stdin");
#endif
	return -1;
      }
      if (dup2(slave, STDOUT_FILENO) != STDOUT_FILENO) {
#ifdef DEBUG
	perror("dup2 stdout");
#endif
	return -1;
      }
      if (dup2(slave, STDERR_FILENO) != STDERR_FILENO) {
#ifdef DEBUG
	perror("dup2 stderr");
#endif
	return -1;
      }

      if (slave > STDERR_FILENO) close(slave);

#endif /* HAVE_FORKPTY */
      
      execvp (cmd [0], cmd);
      
      perror ("execvp");
      exit (127);

#ifndef HAVE_FORKPTY

    }
  }

#ifdef DEBUG
  perror("finding pseudoterminal");
#endif
  return -1;

#endif /* HAVE_FORKPTY */

}

/* The End */

  
