/* $Id: sdlinput.c,v 1.23 2001/08/14 07:07:52 micahjd Exp $
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
#include <pgserver/configfile.h>

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

/* Munge coordinates for skin support */
#ifdef CONFIG_SDLSKIN
struct sdlinput_rect {
  s16 x1,y1,x2,y2,key;
} *sdlinput_map;
int sdlinput_mapsize;
extern s16 sdlfb_display_x;
extern s16 sdlfb_display_y;
extern u16 sdlfb_scale;
#else
#define sdlfb_display_x 0
#define sdlfb_display_y 0
#define sdlfb_scale     1
#endif

/******************************************** Implementations */

void sdlinput_poll(void) {
  SDL_Event evt;
  int ox=-1,oy=-1;
  static int btnstate=0;
  s16 cursorx,cursory;

  if (!SDL_PollEvent(&evt)) return;

#ifdef CONFIG_SDLSKIN
  /* check hotspot map */
  {
    int i;
    for (i=0;i<sdlinput_mapsize;i++)
      if (evt.button.x >= sdlinput_map[i].x1 &&
	  evt.button.y >= sdlinput_map[i].y1 &&
	  evt.button.x <= sdlinput_map[i].x2 &&
	  evt.button.y <= sdlinput_map[i].y2) {
	
	/* FIXME: This doesn't handle modifiers or anything
	 * fancy like that yet. It would also be nice to have some specialized
	 * button codes, like power or backlight.
	 */
	if (evt.type == SDL_MOUSEBUTTONDOWN) {
	  /* Not a weird key? */
	  if (sdlinput_map[i].key < 128)
	    dispatch_key(TRIGGER_CHAR, sdlinput_map[i].key, 0);
	  dispatch_key(TRIGGER_KEYDOWN, sdlinput_map[i].key, 0);
	}
	else if (evt.type == SDL_MOUSEBUTTONUP)
	  dispatch_key(TRIGGER_KEYUP, sdlinput_map[i].key, 0);
       
	return;
      }
  }
#endif

  /* Get the physical position of PicoGUI's cursor */
  cursorx = cursor->x;
  cursory = cursor->y;
  VID(coord_physicalize)(&cursorx,&cursory);
  cursorx *= sdlfb_scale;
  cursory *= sdlfb_scale;
  cursorx += sdlfb_display_x;
  cursory += sdlfb_display_y;

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

    if (sdlinput_foldbuttons)
      evt.motion.state = evt.motion.state!=0;

    if ((evt.motion.x==ox) && (evt.motion.y==oy)) break;
    if (sdlinput_upmove || evt.motion.state)
      dispatch_pointing(TRIGGER_MOVE,
			(-sdlfb_display_x + (ox = evt.motion.x)) / sdlfb_scale,
			(-sdlfb_display_y + (oy = evt.motion.y)) / sdlfb_scale,
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

    if (sdlinput_foldbuttons)
      evt.button.button = evt.button.button!=0;

    dispatch_pointing(TRIGGER_DOWN,
		      (-sdlfb_display_x + 
		      (sdlinput_autowarp ? cursorx : evt.button.x))
		      /sdlfb_scale,
		      (-sdlfb_display_y + 
		      (sdlinput_autowarp ? cursory : evt.button.y))
		      /sdlfb_scale,
		      btnstate |= 1<<(evt.button.button-1));
    break;
    
  case SDL_MOUSEBUTTONUP:
    /* Also auto-warp for button clicks */
    if (sdlinput_autowarp)
      if (evt.button.x!=cursorx ||
	  evt.button.y!=cursory)
	SDL_WarpMouse(cursorx,
		      cursory);

    if (sdlinput_foldbuttons)
      evt.button.button = evt.button.button!=0;
    
    dispatch_pointing(TRIGGER_UP,
		      (-sdlfb_display_x + 
		      (sdlinput_autowarp ? cursorx : evt.button.x))
		      /sdlfb_scale,
		      (-sdlfb_display_y + 
		      (sdlinput_autowarp ? cursory : evt.button.y))
		      /sdlfb_scale,
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
  SDL_ShowCursor(get_param_int("input-sdlinput","sdlcursor",0));

#ifdef CONFIG_SDLSKIN
  /* Load keyboard map */
  if (sdlinput_map)
    g_free(sdlinput_map);
  sdlinput_mapsize = 0;

  {
    const char *fname = get_param_str("input-sdlinput","map",NULL);
    FILE *f;
    char linebuf[80];
    int i;
    g_error e;
    char *p;
    if (fname && (f = fopen(fname,"r"))) {

      /* Count the number of rectangles */
      i = 0;
      while (fgets(linebuf,79,f))
	if (!strncmp(linebuf,"<AREA",5))
	  i++;

      /* Allocate map */
      sdlinput_mapsize = i;
      e = g_malloc((void**)&sdlinput_map,i * sizeof(struct sdlinput_rect));
      errorcheck;
      
      /* Really stupid parser thingy */
      rewind(f);
      i = 0;
      while (fgets(linebuf,79,f) && i<sdlinput_mapsize)
	if (!strncmp(linebuf,"<AREA",5)) {
	  p = strstr(linebuf,"COORDS=")+8;
	  sdlinput_map[i].x1 = atoi(p);
	  p = strchr(p,',')+1;
	  sdlinput_map[i].y1 = atoi(p);
	  p = strchr(p,',')+1;
	  sdlinput_map[i].x2 = atoi(p);
	  p = strchr(p,',')+1;
	  sdlinput_map[i].y2 = atoi(p);
	  p = strstr(p,"HREF=")+6;
	  sdlinput_map[i].key = atoi(p);
	  i++;
	}
    }
  }
#endif

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

#ifdef CONFIG_SDLSKIN
void sdlinput_close(void) {
  if (sdlinput_map)
    g_free(sdlinput_map);
  sdlinput_mapsize = 0;
}
#endif

/******************************************** Driver registration */

g_error sdlinput_regfunc(struct inlib *i) {
  i->init = &sdlinput_init;
  i->poll = &sdlinput_poll;
  i->fd_init = &sdlinput_fd_init;
  i->ispending = &sdlinput_ispending;
#ifdef CONFIG_SDLSKIN
  i->close = &sdlinput_close;
#endif

  return sucess;
}

/* The End */
