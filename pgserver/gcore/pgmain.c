/* $Id: pgmain.c,v 1.1 2000/09/03 18:28:07 micahjd Exp $
 *
 * pgmain.c - Processes command line, initializes and shuts down
 *            subsystems, and invokes the net subsystem for the
 *            main loop.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <pgnet.h>
#include <pgmain.h>
#include <video.h>
#include <divtree.h>
#include <handle.h>
#include <g_error.h>
#include <appmgr.h>
#include <input.h>
#include <timer.h>

#if defined(__WIN32__) || defined(WIN32)
#define WINDOWS
#include <process.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#endif

volatile int proceed = 1;
volatile int in_shutdown = 0;
int use_sessionmgmt = 0;
extern long memref;
struct dtstack *dts;

#ifndef WINDOWS
pid_t my_pid;
void sigterm_handler(int x);
#endif

/********** And it all starts here... **********/
int main(int argc, char **argv) {
  int argi=1;
  const char *arg;

#ifndef WINDOWS
  my_pid = getpid();
#endif

  /*************************************** Command-line processing */

  while (argi<argc && argv[argi][0]=='-') {
    arg = argv[argi++] + 1;

    do {
      switch (*arg) {
	/* No actual command line options yet... */
	
      case '-':  /* --, forces end of switches */
	if (!arg[1])  /* Falls through to default if it's a long arg */
	  goto switches_done;
	
      default:   /* Catches -, -h, --help, etc... */
	puts("\nPicoGUI server (http://pgui.sourceforge.net)\n"
	     "$Id: pgmain.c,v 1.1 2000/09/03 18:28:07 micahjd Exp $\n\n"
	     "pgserver [-h] [--] [session manager prog]\n\n"
	     "\t-h: Displays this usage screen\n"
	     "\nIf a session manager program is specified, it will be run when PicoGUI\n"
	     "starts, and PicoGUI will shut down when closes.\n");
	exit(1);
      }
    } while (*arg);
  };
 switches_done:

  /*************************************** Initialization */

  /* HACK ALERT */
  if (iserror(prerror(load_vidlib(
				  sdl_regfunc,500,500,0,0 	
		      )))) exit(1);  

  /* Subsystem initialization and error check */
  if (iserror(prerror(dts_new()))) exit(1);
  if (iserror(prerror(net_init()))) exit(1);
  if (iserror(prerror(appmgr_init()))) exit(1);
  if (iserror(prerror(timer_init()))) exit(1);

#ifndef WINDOWS
  /* Signal handler (it's usually good to have a way to exit!) */
  if (signal(SIGTERM,&sigterm_handler)==SIG_ERR) {
    prerror(mkerror(ERRT_INTERNAL,54));
    exit(1);
  }
#endif

  /* initial update */
  update();

  /* Now that the socket is listening, run the session manager */

  if (argi<argc && argv[argi]) {
    use_sessionmgmt = 1;

#ifdef WINDOWS
    if (_spawnvp(_P_NOWAIT,argv[argi],argv+argi)<=0) {
      prerror(mkerror(ERRT_BADPARAM,55));
      exit(1);
    }
#else
    if (!fork()) {
      execvp(argv[argi],argv+argi);
      prerror(mkerror(ERRT_BADPARAM,55));
      kill(my_pid,SIGTERM);
      exit(1);
    }
#endif

  }

  /*************************************** Main loop */

  while (proceed)
    net_iteration();

  /*************************************** cleanup time */
  in_shutdown = 1;
  timer_release();
  cleanup_inlib();
  handle_cleanup(-1,-1);
  dts_free();
  net_release();
  appmgr_free();
  if (vid)
    (*vid->close)();
  if (memref!=0) prerror(mkerror(ERRT_MEMORY,56));
  exit(0);
}

#ifndef WINDOWS
void sigterm_handler(int x) {
  proceed = 0;
}
#endif

void request_quit(void) {
#ifdef WINDOWS
  proceed = 0;
#else
  kill(my_pid,SIGTERM);
#endif
}

/* The End */









