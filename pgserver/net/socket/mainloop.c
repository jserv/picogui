/*
 * mainloop.c - initializes and shuts down everything, main loop
 * $Revision: 1.1 $
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <video.h>
#include <divtree.h>
#include <widget.h>
#include <font.h>
#include <handle.h>
#include <request.h>
#include <g_error.h>

#include <unistd.h>
#include <signal.h>

volatile int proceed;
extern long memref;

void sigterm_handler(int x);

int main(int argc, char **argv) {
  struct dtstack *s;
  handle hbgbits,hbgwidget;
  struct bitmap *bgbits;
  struct widget *bgwidget;

  /*************************************** Initialization */

  /* Subsystem initialization and error check */
  if (prerror(dts_new(&s)).type != ERRT_NONE) exit(1);
  if (prerror(req_init(s)).type != ERRT_NONE) exit(1);
  if (prerror(hwr_init()).type != ERRT_NONE) exit(1);

  /* Signal handler (it's usually good to have a way to exit!) */
  if (signal(SIGTERM,&sigterm_handler)==SIG_ERR) {
    prerror(mkerror(ERRT_INTERNAL,"error setting signal handler"));
    exit(1);
  }

  /* Allocate default font */
  if (prerror(findfont(&defaultfont,-1,NULL,0,
		       FSTYLE_DEFAULT)).type != ERRT_NONE) exit(1);

  /* Allocate the background pattern 'filler widget' */
#if 0
  if (prerror(hwrbit_pnm(&bgbits,bg_bits,bg_len)).type != 
	      ERRT_NONE) exit(1);
  else if (prerror(mkhandle(&hbgbits,TYPE_BITMAP,-1,bgbits)).type != 
	ERRT_NONE) exit(1);
  else if (prerror(widget_create(&bgwidget,WIDGET_BITMAP,s,
				  s->top,&s->top->head->next)).type != 
	   ERRT_NONE) exit(1);
  else if (prerror(widget_set(bgwidget,WP_BITMAP_BITMAP,(glob)hbgbits)).
	type != ERRT_NONE) exit(1);
  else if (prerror(widget_set(bgwidget,WP_BITMAP_ALIGN,A_ALL)).
	type != ERRT_NONE) exit(1);
  else if (prerror(mkhandle(&hbgwidget,TYPE_WIDGET,-1,bgwidget)).type != 
	ERRT_NONE) exit(1);	   
#endif

  /* initial update */
  update(s);

  /*************************************** Main loop */

  proceed = 1;
  while (proceed && reqproc());

  /*************************************** cleanup time */
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









