/* $Id$
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
int ptyfd;                  /* file descriptor of the pty master */
pghandle wTerminal,wPanel;  /* Widgets */
char *title = "Terminal";
int sizeW, sizeH;
int terminalHasFont = 0;
int bSwapDeleteBackspace = 0;   /* if you need to swap delete and backspace */

/****************************** UI functions ***/

/* Handler for the menu
 */
int btnFont(struct pgEvent *evt) {
  pghandle fnt;

  fnt = pgFontPicker("Terminal Font");

  if (fnt) {
    if (terminalHasFont)
      pgDelete(pgGetWidget(wTerminal,PG_WP_FONT));
    pgSetWidget(wTerminal,PG_WP_FONT,fnt,0);
    terminalHasFont = 1;
  }  
}

/* Update the titlebar */
void updateTitle(void) {
  pgReplaceTextFmt(wPanel,"%s (%dx%d)",title,sizeW,sizeH);
}  

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

  sizeW = evt->e.size.w;
  sizeH = evt->e.size.h;
  updateTitle();
  
  return 0;
}

/* The terminal's title was changed */
int termTitleChange(struct pgEvent *evt) {
  static char titleBuf[256];
  strncpy(titleBuf, evt->e.data.pointer, sizeof(titleBuf)-1);
  titleBuf[sizeof(titleBuf)-1] = 0;
  title = titleBuf;
  updateTitle();
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

int main(int argc, char **argv) {
  int childpid;
  unsigned char fontsize = 0;
  unsigned char noexit = 0;
  pghandle panelbar;

  pgInit(argc,argv);

  /* Process arguments */


  if (*argv)
     argv++;                 /* Skip program name */
  while (*argv && **argv=='-') {            /* Process switch(es) */
     switch ((*argv)[1]) {  /* Switch :-) */
	
      case 'f':             /* Font size */
	argc--;
	argv++;
	fontsize = atoi(*argv);
	break;
	
      case 't':             /* Title */
	argc--;
	argv++;
	title = *argv;
	break;
	
      case 's':             /* swap delete and backspace */
	bSwapDeleteBackspace = 1;
	break;
	
      case 'p':             /* Pause */
	noexit = 1;
	break;
	
      default:
	printHelp();
     }
     argv++;
  }	
   
  /*** PicoGUI Initialization */
  wPanel = pgRegisterApp(PG_APP_NORMAL,title,0);              /* Register app */

  pgSetWidget(wPanel,PG_WP_MARGIN,0,0);   
   
  /* Use our supernifty panelbar functionality to add a font button,
     if the panelbar was not disabled at compilation time */
  panelbar = pgGetWidget(wPanel,PG_WP_PANELBAR_LABEL);

  if (panelbar) {
    pgNewWidget(PG_WIDGET_BUTTON, PG_DERIVE_BEFORE, panelbar);
    pgSetWidget(PGDEFAULT,
		PG_WP_SIDE, PG_S_LEFT,
		PG_WP_TEXT, pgNewString("f"),
		0);
    pgBind(PGDEFAULT, PG_WE_ACTIVATE, btnFont, NULL);
  }

  /* Make a terminal 
   */
  wTerminal = pgNewWidget(PG_WIDGET_TERMINAL,PG_DERIVE_INSIDE,wPanel);
  if (fontsize) {
     pgSetWidget(PGDEFAULT,PG_WP_FONT,
		 pgNewFont(NULL,fontsize,PG_FSTYLE_FIXED),0);
     terminalHasFont = 1;
  }
  pgBind(PGDEFAULT,PG_WE_DATA,&termInput,NULL);      /* Input handler */
  pgCustomizeSelect(&mySelect,&mySelectBH);          /* select() wrapper for output */
  pgBind(PGDEFAULT,PG_WE_RESIZE,&termResize,NULL);   /* Resize handler */
  pgBind(PGDEFAULT,PG_WE_TITLECHANGE,&termTitleChange,NULL);  /* Title handler */
  pgFocus(PGDEFAULT);
  
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
      "/bin/sh", "-sh", NULL 
    };

    /* We support xterm's extended escapes, so pretend to be xterm */
    putenv("TERM=xterm");

    if ((childpid = ptyfork (& ptyfd, argv [0] ? argv : cmd)) < 0 ) {
      pgMessageDialogFmt (argv [0] ? argv[0] : "Shell",
			  0, "Error acquiring pseudoterminal:\n%s",
			  strerror (errno));
      exit(1);
    }
  }

  /*** Event loop */
  pgEventLoop();
   
  /*** Cleanup */
  kill(childpid,SIGTERM);  /* If the child is still alive, fix that */
  return 0;
}

/* The End */

