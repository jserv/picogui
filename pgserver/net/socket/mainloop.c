/* $Id: mainloop.c,v 1.6 2000/04/24 02:38:36 micahjd Exp $
 *
 * mainloop.c - initializes and shuts down everything, main loop
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

#include <video.h>
#include <divtree.h>
#include <handle.h>
#include <request.h>
#include <g_error.h>
#include <appmgr.h>
#include <input.h>

#if defined(__WIN32__) || defined(WIN32)
#define WINDOWS
#include <SDL.h> /* Currently we need this for windoze event processing */
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#endif

volatile int proceed;
extern long memref;

#ifndef WINDOWS
pid_t my_pid;
void sigterm_handler(int x);
#endif

void request_quit(void);

int main(int argc, char **argv) {
  struct dtstack *s;

#ifndef WINDOWS
  my_pid = getpid();
#endif

#ifdef WINDOWS
void windows_inputpoll_hack(void);
#endif

  /*************************************** Initialization */

  /* Subsystem initialization and error check */
  if (prerror(dts_new(&s)).type != ERRT_NONE) exit(1);
  if (prerror(req_init(s)).type != ERRT_NONE) exit(1);
  if (prerror(appmgr_init(s)).type != ERRT_NONE) exit(1);
  if (prerror(hwr_init()).type != ERRT_NONE) exit(1);
  if (prerror(input_init(&request_quit)).type != ERRT_NONE) exit(1);

#ifndef WINDOWS
  /* Signal handler (it's usually good to have a way to exit!) */
  if (signal(SIGTERM,&sigterm_handler)==SIG_ERR) {
    prerror(mkerror(ERRT_INTERNAL,"error setting signal handler"));
    exit(1);
  }
#endif

  /* initial update */
  update(s);

  /*************************************** Main loop */

  proceed = 1;
  while (proceed && reqproc())
#ifndef WINDOWS
    ;
#else
    windows_inputpoll_hack();
#endif

  /*************************************** cleanup time */
  input_release();
  handle_cleanup(-1);
  dts_free(s);
  hwr_release();
  req_free();
  if (memref!=0) prerror(mkerror(ERRT_MEMORY,"Memory leak detected on exit"));
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









