/* $Id: input.c,v 1.7 2000/08/08 16:59:11 micahjd Exp $
 *
 * input.c - Input layer for SDL
 * 
 * Creates a seperate thread that waits on the SDL events and sends
 * them over to widget.c
 *
 * In windoze waiting in a seperate thread doesn't work, so we poll the
 * input and the network.  This makes the 'normal' input_init and
 * input_release into stubs and expects windows_inputpoll_hack() to be
 * called on a regular basis.
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

#include <g_error.h>
#include <input.h>
#include <widget.h>

#include <SDL.h>
#include <SDL_thread.h>

#if defined(__WIN32__) || defined(WIN32)
#define WINDOWS
#endif

int btnstate;
extern SDL_Surface *screen;

#ifndef WINDOWS
int threadfunc(void *p);
SDL_Thread *thread;
#else
void windows_inputpoll_hack(void);
#endif
void (*quitreq)(void);

/* Create the thread */
g_error input_init(void (*request_quit)(void)) {
  quitreq = request_quit;
  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
#ifndef WINDOWS
  thread = SDL_CreateThread(&threadfunc,NULL);
  if (!thread) return mkerror(ERRT_IO,"Can't create input thread");
#endif
  return sucess;
}

/* Terminate the thread */
void input_release() {
#ifndef WINDOWS
  SDL_KillThread(thread);
#endif
}

/* The actual thread */
#ifndef WINDOWS
int threadfunc(void *p) {
#else
void windows_inputpoll_hack(void) {
#endif
  SDL_Event evt;
  int ox=-1,oy=-1;
  static int prevsym=-1;
#ifndef WINDOWS
  for (;;) {
    SDL_WaitEvent(&evt);
#else
  if (!SDL_PollEvent(&evt)) return;
#endif   

    switch (evt.type) {

    case SDL_MOUSEMOTION:
      if ((evt.motion.x==ox) && (evt.motion.y==oy)) break;
      dispatch_pointing(TRIGGER_MOVE,ox = evt.motion.x,
			oy = evt.motion.y,btnstate=evt.motion.state);
      break;

    case SDL_MOUSEBUTTONDOWN:
      dispatch_pointing(TRIGGER_DOWN,evt.button.x,
			evt.button.y,btnstate |= 1<<(evt.button.button-1));
      break;

    case SDL_MOUSEBUTTONUP:
      dispatch_pointing(TRIGGER_UP,evt.button.x,
			evt.button.y,btnstate &= ~(1<<(evt.button.button-1)));
      break;

    case SDL_KEYDOWN:
      /* Is this the special exit key? (CTRL-ALT-SLASH) */
      if (evt.key.keysym.sym==PGKEY_SLASH &&
	  (evt.key.keysym.mod & PGMOD_CTRL) &&
	  (evt.key.keysym.mod & PGMOD_ALT)) {
	(*quitreq)();
	break;
      }

#ifdef DEBUG
      /* Some magic keys for debugging */

      /* Blank the screen on alt-b */
      if (evt.key.keysym.sym==PGKEY_b &&
	  (evt.key.keysym.mod & PGMOD_ALT)) {
	hwr_clear();
	hwr_update();
	break;
      }
#endif

      if (evt.key.keysym.unicode)
	dispatch_key(TRIGGER_CHAR,evt.key.keysym.unicode,evt.key.keysym.mod);

      /* Make this ignore repeat */
      if (prevsym!=evt.key.keysym.sym)
	dispatch_key(TRIGGER_KEYDOWN,prevsym=evt.key.keysym.sym,
		     evt.key.keysym.mod);
      break;

    case SDL_KEYUP:
      prevsym = -1;
      dispatch_key(TRIGGER_KEYUP,evt.key.keysym.sym,evt.key.keysym.mod);
      break;

    case SDL_QUIT:
      (*quitreq)();
      break;

    }
#ifndef WINDOWS
  }
#endif
}

/* The End */










