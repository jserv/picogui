/*
 * mainloop.c - initializes and shuts down everything, main loop
 * $Revision: 1.3 $
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <video.h>
#include <divtree.h>
#include <handle.h>
#include <request.h>
#include <g_error.h>
#include <appmgr.h>
#include <input.h>

#include <unistd.h>
#include <signal.h>

volatile int proceed;
extern long memref;

void sigterm_handler(int x);

int main(int argc, char **argv) {
  struct dtstack *s;

  /*************************************** Initialization */

  /* Subsystem initialization and error check */
  if (prerror(dts_new(&s)).type != ERRT_NONE) exit(1);
  if (prerror(req_init(s)).type != ERRT_NONE) exit(1);
  if (prerror(appmgr_init(s)).type != ERRT_NONE) exit(1);
  if (prerror(hwr_init()).type != ERRT_NONE) exit(1);
  if (prerror(input_init()).type != ERRT_NONE) exit(1);

  /* Signal handler (it's usually good to have a way to exit!) */
  if (signal(SIGTERM,&sigterm_handler)==SIG_ERR) {
    prerror(mkerror(ERRT_INTERNAL,"error setting signal handler"));
    exit(1);
  }

  /* initial update */
  update(s);

  /*************************************** Main loop */

  proceed = 1;
  while (proceed && reqproc());

  /*************************************** cleanup time */
  input_release();
  handle_cleanup(-1);
  dts_free(s);
  hwr_release();
  req_free();
  if (memref!=0) prerror(mkerror(ERRT_MEMORY,"Memory leak detected on exit"));
  exit(0);
}

void sigterm_handler(int x) {
  proceed = 0;
}

/* The End */









