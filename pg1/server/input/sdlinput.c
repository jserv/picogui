/* $Id$
 *
 * sdlinput.h - input driver for SDL
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
 * 
 */

#include <pgserver/common.h>

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/configfile.h>
#include <pgserver/os.h>

#include <string.h>
#include <stdlib.h>

#include <SDL/SDL.h>

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
u8 sdlinput_foldbuttons;
u8 sdlinput_upmove;
u8 sdlinput_nomouse;
u8 sdlinput_nokeyboard;

/* A cursor, if we have one */
struct cursor *sdlinput_cursor;

/* Distance the scroll wheel should move */
int sdlinput_scroll_distance;

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
  int newx, newy;
  static int btnstate = 0;
  int pgx, pgy;

  while (SDL_PollEvent(&evt)) {

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
	  if (evt.type == SDL_MOUSEBUTTONDOWN)
	    infilter_send_key(PG_TRIGGER_KEY,sdlinput_map[i].key,0);
	  
	  return;
	}
    }
#endif
    
    if (!sdlinput_nomouse) {
      
      /* Process changes in the mouse position. We were using SDL's mouse events for this, but
       * that tends to create mouse 'lag' due to the buffering of events. This method should
       * always use the most up-to-date mouse data even if we're behind in the queue of events.
       */
      
      SDL_GetMouseState(&newx, &newy);
      
      /* Calculate the transformed coordinates we'll give picogui */
      pgx = (-sdlfb_display_x + newx) / sdlfb_scale;
      pgy = (-sdlfb_display_y + newy) / sdlfb_scale;
      
      /* The mouse might have moved... the input filters will sort it out */
      if (sdlinput_upmove || btnstate)
	infilter_send_pointing(PG_TRIGGER_MOVE, pgx, pgy, btnstate, sdlinput_cursor);
      
      switch (evt.type) {
	
      case SDL_MOUSEBUTTONDOWN:
	/* Scroll wheel events */
	if (evt.button.button == 4)
	  infilter_send_pointing(PG_TRIGGER_SCROLLWHEEL,0,-sdlinput_scroll_distance,0,sdlinput_cursor);
	else if (evt.button.button == 5)
	  infilter_send_pointing(PG_TRIGGER_SCROLLWHEEL,0,sdlinput_scroll_distance,0,sdlinput_cursor);
	else {
	  /* Normal button */
	  if (sdlinput_foldbuttons)
	    evt.button.button = evt.button.button!=0;

	  infilter_send_pointing(PG_TRIGGER_DOWN, pgx, pgy, 
				 btnstate |= 1<<(evt.button.button-1), sdlinput_cursor);
	}
	break;
	
      case SDL_MOUSEBUTTONUP:      
	/* Ignore up events for the scroll wheel if we get them */
	if (evt.button.button == 4 || evt.button.button == 5)
	  break;
	if (sdlinput_foldbuttons)
	  evt.button.button = evt.button.button!=0;
	infilter_send_pointing(PG_TRIGGER_UP, pgx, pgy,
			       btnstate &= ~(1<<(evt.button.button-1)), sdlinput_cursor);
	break;
      }
    }
    
    switch (evt.type) {
    case SDL_KEYDOWN:
      if (!sdlinput_nokeyboard) {
	if (evt.key.keysym.unicode)
	  infilter_send_key(PG_TRIGGER_CHAR,evt.key.keysym.unicode,evt.key.keysym.mod);
	infilter_send_key(PG_TRIGGER_KEYDOWN,evt.key.keysym.sym,evt.key.keysym.mod);
      }
      break;
      
    case SDL_KEYUP:
      if (!sdlinput_nokeyboard)
	infilter_send_key(PG_TRIGGER_KEYUP,evt.key.keysym.sym,evt.key.keysym.mod);
      break;
      
    case SDL_QUIT:
      pgserver_mainloop_stop();
      break;
      
    case SDL_VIDEORESIZE:
      if (evt.resize.w != vid->xres || evt.resize.h != vid->yres) {
	video_setmode(evt.resize.w,evt.resize.h,vid->bpp,PG_FM_ON,0);
	update(NULL,1);
      }
      break;
    }
  }
}

g_error sdlinput_init(void) {
  g_error e;

  if (get_param_int("input-sdlinput","pgcursor",1)) {
    e = cursor_new(&sdlinput_cursor,NULL,-1);
    errorcheck;
  }

  sdlinput_foldbuttons = get_param_int("input-sdlinput","foldbuttons",0);
  sdlinput_upmove = get_param_int("input-sdlinput","upmove",1);
  sdlinput_nomouse = get_param_int("input-sdlinput","nomouse",0);
  sdlinput_nokeyboard = get_param_int("input-sdlinput","nokeyboard",0);
  sdlinput_scroll_distance = get_param_int("input-sdlinput","scroll_distance",20);
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
    int mapscale_n,mapscale_d;
     
    sscanf(get_param_str("input-sdlinput","mapscale","1 1"), "%d %d",
	   &mapscale_n, &mapscale_d);
     
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
	  sdlinput_map[i].x1 = atoi(p) * mapscale_n / mapscale_d;
	  p = strchr(p,',')+1;
	  sdlinput_map[i].y1 = atoi(p) * mapscale_n / mapscale_d;
	  p = strchr(p,',')+1;
	  sdlinput_map[i].x2 = atoi(p) * mapscale_n / mapscale_d;
	  p = strchr(p,',')+1;
	  sdlinput_map[i].y2 = atoi(p) * mapscale_n / mapscale_d;
	  p = strstr(p,"HREF=")+6;
	  sdlinput_map[i].key = atoi(p);
	  i++;
	}
    }
  }
#endif

  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
  return success;
}

/* Polling time for the input driver */ 
void sdlinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if (timeout->tv_sec > 0 || timeout->tv_usec > POLL_USEC) {
    timeout->tv_sec = 0;
    timeout->tv_usec = POLL_USEC;
  }
}

void sdlinput_close(void) {
  if (sdlinput_cursor)
    pointer_free(-1,sdlinput_cursor);
#ifdef CONFIG_SDLSKIN
  if (sdlinput_map)
    g_free(sdlinput_map);
  sdlinput_mapsize = 0;
#endif
}

void sdlinput_message(u32 message, u32 param, u32 *ret) {
  switch (message) {
    
  case PGDM_MOUSEWARP:
    SDL_WarpMouse(param>>16, param & 0xFFFF);
    break;

  }
}


/******************************************** Driver registration */

g_error sdlinput_regfunc(struct inlib *i) {
  i->init = &sdlinput_init;
  i->poll = &sdlinput_poll;
  i->fd_init = &sdlinput_fd_init;
  i->close = &sdlinput_close;
  i->message = &sdlinput_message;

  return success;
}

/* The End */
