/* $Id: sdlinput.c,v 1.9 2000/10/29 01:45:35 micahjd Exp $
 *
 * sdlinput.h - input driver for SDL
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifdef DRIVER_SDLINPUT

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>

#include <SDL.h>
#include <SDL_thread.h>

#if defined(__WIN32__) || defined(WIN32)
#define WINDOWS
#endif

#ifndef WINDOWS
SDL_Thread *sdlinput_thread;
#endif

/******************************************** Implementations */

/* This function checks for new input, and dispatches it.
   In windows, this is polled as often as possible because
   the threads don't work right. In other OS, it is called
   from a seperate thread. */

#ifndef WINDOWS
int sdlinput_threadfunc(void *p) {
#else
void sdlinput_poll(void) {
#endif
  SDL_Event evt;
  int ox=-1,oy=-1;
  static int prevsym=-1;
  static int btnstate=0;
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
      request_quit();
      break;

    }
#ifndef WINDOWS
  }
#endif
}

g_error sdlinput_init(void) {
  SDL_ShowCursor(0);    /* Handle our own cursor */
  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
#ifndef WINDOWS
  sdlinput_thread = SDL_CreateThread(&sdlinput_threadfunc,NULL);
  if (!sdlinput_thread) return mkerror(PG_ERRT_IO,71);
#endif
  return sucess;
}
 
void sdlinput_close(void) {
#ifndef WINDOWS
  SDL_KillThread(sdlinput_thread);
#endif
}

#ifdef WINDOWS
/* Make it poll as often as possible */
void sdlinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  timeout->tv_sec = 0;
  timeout->tv_usec = 0;
}
#endif


/******************************************** Driver registration */

g_error sdlinput_regfunc(struct inlib *i) {
  i->init = &sdlinput_init;
  i->close = &sdlinput_close;
#ifdef WINDOWS
  i->poll = &sdlinput_poll;
  i->fd_init = &sdlinput_fd_init;
#endif

  return sucess;
}

#endif /* DRIVER_SDLINPUT */
/* The End */
