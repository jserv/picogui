/* $Id: mainloop.c,v 1.13 2000/08/01 06:31:39 micahjd Exp $
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
#include <pgnet.h>
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

volatile int proceed = 1;
volatile int in_shutdown = 0;
extern long memref;
struct dtstack *dts;

#ifndef WINDOWS
pid_t my_pid;
void sigterm_handler(int x);
#endif

void request_quit(void);

int main(int argc, char **argv) {

#ifndef WINDOWS
  my_pid = getpid();
#endif

#ifdef WINDOWS
void windows_inputpoll_hack(void);
#endif

  /*************************************** Initialization */

  /* Subsystem initialization and error check */
  if (prerror(hwr_init()).type != ERRT_NONE) exit(1);
  if (prerror(dts_new()).type != ERRT_NONE) exit(1);
  if (prerror(req_init()).type != ERRT_NONE) exit(1);
  if (prerror(appmgr_init()).type != ERRT_NONE) exit(1);
  if (prerror(input_init(&request_quit)).type != ERRT_NONE) exit(1);

#ifndef WINDOWS
  /* Signal handler (it's usually good to have a way to exit!) */
  if (signal(SIGTERM,&sigterm_handler)==SIG_ERR) {
    prerror(mkerror(ERRT_INTERNAL,"error setting signal handler"));
    exit(1);
  }
#endif

  /* initial update */
  update();

  /*************************************** Main loop */

  while (proceed && reqproc())
#ifndef WINDOWS
    ;
#else
    windows_inputpoll_hack();
#endif

  /*************************************** cleanup time */
  in_shutdown = 1;
  input_release();
  handle_cleanup(-1,-1);
  dts_free();
  req_free();
  appmgr_free();
  hwr_release();
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









