/* $Id: signals.c,v 1.1 2002/01/18 00:27:11 micahjd Exp $
 *
 * signal.c - Handle some fatal and not-so-fatal signals gracefully
 *            The SIGSEGV handling et cetera was inspired by SDL's
 *            "parachute" in SDL_fatal.c
 * 
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/common.h>
#include <pgserver/pgmain.h>  /* main loop flags */
#include <pgserver/video.h>   /* drivermessage() */

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Signals we need to handle... */
static int pgserver_signals[] = {
  SIGSEGV,
  SIGBUS,
  SIGFPE,
  SIGQUIT,
  SIGPIPE,
  SIGUSR1,
  SIGUSR2,
  SIGCHLD,
  SIGTERM,
  0
};

/* Handler for all signals!
 */
void signals_handler(int sig) {
  static volatile u8 lock = 0;

  switch (sig) {

  case SIGCHLD:
    /* Wait for child so we don't have zombies */
    waitpid(-1, NULL, WNOHANG);
    
    /* Need to start the session manager? */
    if (sessionmgr_secondary) {
      sessionmgr_start = 1;
      sessionmgr_secondary = 0;
    }
    break;

  case SIGUSR1:
  case SIGUSR2:
    /* These signals may be used for VT switching,
     * debugging, or other custom doodads...
     */
    drivermessage(PGDM_SIGNAL, sig, NULL);
    break;

  case SIGPIPE:
    /* Not fatal... but give a warning */
#ifdef HAS_GURU
    guru("SIGPIPE received (ignoring it)");
#endif
    break;

  case SIGTERM:
    /* We should exit gracefully */
    mainloop_proceed = 0;
    break;

  default:
    /* Argh, it's a fatal signal... terminate as quickly
     * as we can while trying not to leave the console
     * in a confusing or unusable state.
     */

    /* Prevent infinite recursion */
    if (lock++) break;

    /* First put up a guru message if we can, in case we're
     * unsucessful in closing the video device
     */
#ifdef HAS_GURU
    guru("pgserver received a fatal signal!\n"
	 "Attempting to shut down...\n"
	 "(If you can see this message, it probably means\n"
	 "that shutdown has failed. Sorry.)");
#endif

    /* Try to shutdown the video driver if it's on */
    if (vid && !in_init)
      VID(close)();

    /* Print an appropriate error message for the most popular signals */
    switch (sig) {
    case SIGSEGV:  prerror(mkerror(PG_ERRT_INTERNAL,16)); break;
    case SIGFPE:   prerror(mkerror(PG_ERRT_INTERNAL,19)); break;
    }

    /* A generic fatal signal message */
    prerror(mkerror(PG_ERRT_INTERNAL,106));

    /* Ieeeeeee! */
    exit(-sig);
  }
}


/* Someone set up us the signal!
 */
void signals_install(void) {
  int *sig;

  for (sig=pgserver_signals;*sig;sig++)
    if (signal(*sig,&signals_handler)==SIG_ERR) {
      prerror(mkerror(PG_ERRT_INTERNAL,54));
      exit(1);
    }
}

/* The End */
