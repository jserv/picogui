/* $Id: posix_signals.c 3929 2003-05-15 02:23:35Z lalo $
 *
 * posix_signals.c - Handle signals necessary for subprocess termination,
 *                   quit requests, and VT switching
 * 
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/os_posix.h>
#include <pgserver/video.h>   /* drivermessage() */
#include <pgserver/input.h>   /* cleanup_inlib() */

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef __CYGWIN__
#undef DEBUG_SIGTRACE
#endif

/* Signals we need to handle... */
static int pgserver_signals[] = {
  /* Not-so-fatal signals */
  SIGPIPE,
  SIGCHLD,
  SIGALRM,
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
#ifdef DEBUG_SIGTRACE   /* Use a sigaction handler if we need a trace */
void signals_handler(int sig, siginfo_t *siginfo, void *context) {
#else
void signals_handler(int sig) {
#endif
  static volatile u8 lock = 0;
  int i;

  switch (sig) {

  case SIGCHLD:
    /* Wait for child so we don't have zombies */
    waitpid(-1, &i, WNOHANG);
    os_posix_child_return = WEXITSTATUS(i);
    childqueue_pop();
    break;

  case SIGALRM:
    master_timer();
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
    guru("SIGPIPE received (ignoring it)");
    break;

  case SIGTERM:
  case SIGINT:
    pgserver_mainloop_stop();
    break;

  default:
    /* Argh, it's a fatal signal... terminate as quickly
     * as we can while trying not to leave the console
     * in a confusing or unusable state.
     */

#ifdef DEBUG_SIGTRACE   /* Print some extra info about the signal before quitting */
    if (siginfo)
      printf("*** PicoGUI Oops!\n"
	     "***          si_signo: %d (%p)\n"
	     "***          si_errno: %d\n"
	     "***           si_code: %d\n"
	     "***            si_pid: %d\n"
	     "***            si_uid: %d\n"
	     "***         si_status: %d\n"
	     "***          si_value: %d\n"
	     "***            si_int: %d\n"
	     "***            si_ptr: %p\n"
	     "***           si_addr: %p\n"
	     "***           si_band: %d\n"
	     "***             si_fd: %d\n",
	     siginfo->si_signo,(void*)siginfo->si_signo,
	     siginfo->si_errno,
	     siginfo->si_code,
	     siginfo->si_pid,
	     siginfo->si_uid,
	     siginfo->si_status,
	     siginfo->si_value,
	     siginfo->si_int,
	     siginfo->si_ptr,
	     siginfo->si_addr,
	     siginfo->si_band,
	     siginfo->si_fd);
#endif /* DEBUG_SIGTRACE */

    /* Prevent infinite recursion */
    if (lock++) break;

    /* Try to shutdown the video driver if it's on */
    if (vid)
      VID(close)();

    /* It would also be nice not to hose the console.. */
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

#ifndef DEBUG_SIGTRACE   /* If we're using sigaction, we can ask to be restarted automatically */
  /* Some platforms (at least Linux <= 2.0.x) unregister the signal
     handler once the signal occured. So we need to register it back */
  if (signal(sig,&signals_handler)==SIG_ERR) {
    prerror(mkerror(PG_ERRT_INTERNAL,54));
    exit(1);
  }
#endif
}


/* Someone set up us the signal!
 */
void os_posix_signals_install(void) {
  int *sig;

  for (sig=pgserver_signals;*sig;sig++) {
#ifdef DEBUG_SIGTRACE   /* Set a sigaction handler */
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &signals_handler;
    if (sigaction(*sig,&sa,NULL)<0) {
      prerror(mkerror(PG_ERRT_INTERNAL,54));
      exit(1);
    }
#else                   /* Normal signal handler */
    if (signal(*sig,&signals_handler)==SIG_ERR) {
      prerror(mkerror(PG_ERRT_INTERNAL,54));
      exit(1);
    }
#endif /* DEBUG_SIGTRACE */
  }
}

/* The End */
