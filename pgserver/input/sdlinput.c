/* $Id: sdlinput.c,v 1.20 2001/08/12 22:35:15 micahjd Exp $
 *
 * sdlinput.h - input driver for SDL
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>

#include <SDL.h>
#include <SDL_thread.h>

/* I have tried various methods for this SDL input driver. For quite a
 * while it was threaded. This offered fast response and no CPU overhead,
 * but it was dangerous (all inputs end up as video commands, which aren't
 * reentrant)
 *
 * Another option is IPC, to send the events back to the main thread.
 * This worked, but with CPU overhead (maybe I wasn't doing it right?)
 *
 * Anyway, this will suffice until I write something better. I'll not be as
 * paranoid about this driver as the SVGAlib and EZ328 are much more important.
 * This driver is for debugging: it's easier to debug programs without
 * threads.
 *
 * This polling constant can be tweaked to balance responsiveness with system
 * load. At 100, it's nice and responsive yet doesn't even show up on my
 * development machine's load meter. Mileage will vary.
 */
#define POLL_USEC 100

/* Options from the config file */
u8 sdlinput_autowarp;
u8 sdlinput_pgcursor;
u8 sdlinput_foldbuttons;
u8 sdlinput_upmove;
s16 sdlinput_translate_x;
s16 sdlinput_translate_y;


/******************************************** Implementations */

void sdlinput_poll(void) {
  SDL_Event evt;
  int ox=-1,oy=-1;
  static int btnstate=0;
  s16 cursorx,cursory;

  if (!SDL_PollEvent(&evt)) return;

  /* Get the physical position of PicoGUI's cursor */
  cursorx = cursor->x;
  cursory = cursor->y;
  VID(coord_physicalize)(&cursorx,&cursory);
  cursorx -= sdlinput_translate_x;
  cursory -= sdlinput_translate_y;

  switch (evt.type) {
    
  case SDL_MOUSEMOTION:
    /* If SDL's old mouse position doesn't jive with our cursor position,
     * warp the mouse and try again.
     */
    if (sdlinput_autowarp)
      if ((evt.motion.x-evt.motion.xrel)!=cursorx ||
	  (evt.motion.y-evt.motion.yrel)!=cursory) {
	SDL_WarpMouse(cursorx,
		      cursory);
	break;
      }

    if ((evt.motion.x==ox) && (evt.motion.y==oy)) break;
    if (sdlinput_upmove || evt.motion.state)
      dispatch_pointing(TRIGGER_MOVE,
			sdlinput_translate_x + (ox = evt.motion.x),
			sdlinput_translate_y + (oy = evt.motion.y),
			btnstate=evt.motion.state);
    if (sdlinput_pgcursor)
      drivermessage(PGDM_CURSORVISIBLE,1);
    break;
    
  case SDL_MOUSEBUTTONDOWN:
    /* Also auto-warp for button clicks */
    if (sdlinput_autowarp)
      if (evt.button.x!=cursorx ||
	  evt.button.y!=cursory)
	SDL_WarpMouse(cursorx,cursory);

    dispatch_pointing(TRIGGER_DOWN,
		      sdlinput_translate_x + 
		      (sdlinput_autowarp ? cursorx : evt.button.x),
		      sdlinput_translate_y + 
		      (sdlinput_autowarp ? cursory : evt.button.y),
		      btnstate |= 1<<(evt.button.button-1));
    break;
    
  case SDL_MOUSEBUTTONUP:
    /* Also auto-warp for button clicks */
    if (sdlinput_autowarp)
      if (evt.button.x!=cursorx ||
	  evt.button.y!=cursory)
	SDL_WarpMouse(cursorx,
		      cursory);
    
    dispatch_pointing(TRIGGER_UP,
		      sdlinput_translate_x + 
		      (sdlinput_autowarp ? cursorx : evt.button.x),
		      sdlinput_translate_y + 
		      (sdlinput_autowarp ? cursory : evt.button.y),
		      btnstate &= ~(1<<(evt.button.button-1)));
    break;
    
  case SDL_KEYDOWN:
    if (evt.key.keysym.unicode)
      dispatch_key(TRIGGER_CHAR,evt.key.keysym.unicode,evt.key.keysym.mod);
    dispatch_key(TRIGGER_KEYDOWN,evt.key.keysym.sym,evt.key.keysym.mod);
    break;
    
  case SDL_KEYUP:
    dispatch_key(TRIGGER_KEYUP,evt.key.keysym.sym,evt.key.keysym.mod);
    break;
      
  case SDL_QUIT:
    request_quit();
    break;

  case SDL_VIDEORESIZE:
    video_setmode(evt.resize.w,evt.resize.h,vid->bpp,PG_FM_SET,vid->flags);
    update(NULL,1);
    break;
  }
}

g_error sdlinput_init(void) {
  sdlinput_autowarp = get_param_int("input-sdlinput","autowarp",1);
  sdlinput_pgcursor = get_param_int("input-sdlinput","pgcursor",1);
  sdlinput_foldbuttons = get_param_int("input-sdlinput","foldbuttons",0);
  sdlinput_upmove = get_param_int("input-sdlinput","upmove",1);
  sdlinput_translate_x = get_param_int("input-sdlinput","translate_x",0);
  sdlinput_translate_y = get_param_int("input-sdlinput","translate_y",0);
  SDL_ShowCursor(get_param_int("input-sdlinput","sdlcursor",0));

  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
  return sucess;
}

/* Polling time for the input driver */ 
void sdlinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  timeout->tv_sec = 0;
  timeout->tv_usec = POLL_USEC;
}

/* Check the input queue */
int sdlinput_ispending(void) {
   sdlinput_poll();
   return SDL_PollEvent(NULL);
}

/******************************************** Driver registration */

g_error sdlinput_regfunc(struct inlib *i) {
  i->init = &sdlinput_init;
  i->poll = &sdlinput_poll;
  i->fd_init = &sdlinput_fd_init;
  i->ispending = &sdlinput_ispending;
  return sucess;
}

/* The End */
