/* $Id: signals.c,v 1.8 2002/03/28 11:43:38 micahjd Exp $
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
#include <pgserver/input.h>   /* cleanup_inlib() */

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Signals we need to handle... */
static int pgserver_signals[] = {
  /* Not-so-fatal signals */
  SIGPIPE,
  SIGCHLD,
  /* Fatal signals */
#ifndef DEBUG_FATALSIGNALS
  SIGTERM,
  SIGSEGV,
  SIGBUS,
  SIGFPE,
  SIGQUIT,
  SIGINT,
#endif
  /* Extra signals */
  SIGUSR1,
#ifdef SIGUNUSED
  SIGUNUSED,
#endif
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
#ifdef SIGUNUSED
  case SIGUNUSED:
#endif
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
  case SIGINT:
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

    /* Try to shutdown the video driver if it's on */
    if (vid && !in_init)
      VID(close)();

    /* It would also be nice not to hose the console.. */
    if (!in_init)
      cleanup_inlib();
     
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

  /* Some platforms (at least Linux <= 2.0.x) unregister the signal
     handler once the signal occured. So we need to register it back */
  if (signal(sig,&signals_handler)==SIG_ERR) {
    prerror(mkerror(PG_ERRT_INTERNAL,54));
    exit(1);
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
