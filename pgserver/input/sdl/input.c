/*
 * input.c - Input layer for SDL
 * $Revision: 1.1 $
 * 
 * Creates a seperate thread that waits on the SDL events and sends
 * them over to widget.c
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <g_error.h>
#include <input.h>
#include <widget.h>

#include <SDL.h>
#include <SDL_thread.h>

extern SDL_Surface *screen;
int threadfunc(void *p);
SDL_Thread *thread;
void (*quitreq)(void);

/* Create the thread */
g_error input_init(void (*request_quit)(void)) {
  quitreq = request_quit;
  thread = SDL_CreateThread(&threadfunc,NULL);
  if (!thread) return mkerror(ERRT_IO,"Can't create input thread");
  return sucess;
}

/* Terminate the thread */
void input_release() {
  SDL_KillThread(thread);
}

/* The actual thread */
int threadfunc(void *p) {
  SDL_Event evt;
  int ox=-1,oy=-1;
  for (;;) {
    SDL_WaitEvent(&evt);
    switch (evt.type) {

    case SDL_MOUSEMOTION:
      if ((evt.motion.x==ox) && (evt.motion.y==oy)) break;
      dispatch_pointing(TRIGGER_MOVE,ox = evt.motion.x,
			oy = evt.motion.y,evt.motion.state);
      break;

    case SDL_MOUSEBUTTONDOWN:
      dispatch_pointing(TRIGGER_DOWN,evt.button.x,
			evt.button.y,SDL_GetMouseState(NULL,NULL));
      break;

    case SDL_MOUSEBUTTONUP:
      dispatch_pointing(TRIGGER_UP,evt.button.x,
			evt.button.y,SDL_GetMouseState(NULL,NULL));
      break;

    case SDL_QUIT:
      (*quitreq)();
      break;

    }
  }
}

/* The End */










