/* $Id: sdlinput.c 3978 2003-05-23 10:19:38Z micah $
 *
 * directfbinput.h - input driver for DirectFB
 *
 *    NOTE: Much of the code in here, especially the keyboard translation,
 *          was lifted from the SDL_x11events.c file in SDL.
 *          The Simple Directmedia Layer is:
 *             Copyright (C) 1997-2001 Sam Lantinga
 *             http://libsdl.org
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
 * Lalo Martins <lalo@laranja.org> - initial implementation (11/2003)
 *   (based on Micah's sdlinput)
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
#include <stdio.h>

#include <directfb.h>

/* the main interface, initialized in the video driver */
/* (should probably move to a directfb.h? */
extern IDirectFB *directfb;

/* the event buffer */
// FIXME: dfb seems to support multiple pointer devices. We should give each
// its own picogui cursor - for that we'd probably need separate event buffers
IDirectFBEventBuffer *evb;

/* This polling constant can be tweaked to balance responsiveness with system
 * load. At 100, it's nice and responsive yet doesn't even show up on my
 * development machine's load meter. Mileage will vary.
 */
#define POLL_USEC 100

static u16 keymap[256];
static u32 directfb_translatekey (DFBInputEvent *ev);

/* Options from the config file */
u8 directfbinput_foldbuttons;
u8 directfbinput_upmove;
u8 directfbinput_nomouse;
u8 directfbinput_nokeyboard;

/* A cursor, if we have one */
struct cursor *directfbinput_cursor = NULL;

/* Distance the scroll wheel should move */
int directfbinput_scroll_distance;

// FIXME: we probably shouldn't die on all errors (hence the name)
/* (should probably move to a directfb.h? */
#define dfb_errcheck_die                                       \
  {                                                            \
    if (err != DFB_OK)                                         \
      {                                                        \
        fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
        DirectFBErrorFatal( "", err );                         \
      }                                                        \
  }

#define DrawPixel(surface, x, y)                               \
   surface->DrawLine (surface, x, y, x, y);                    \

/******************************************** Implementations */

static void init_keymap() {
  int i;
	
  /* Initialize the DirectFB key translation table */
  for (i=0; i<256; ++i)
    keymap[i] = 0;

  keymap[DIKI_A - DIKI_UNKNOWN] = PGKEY_a;
  keymap[DIKI_B - DIKI_UNKNOWN] = PGKEY_b;
  keymap[DIKI_C - DIKI_UNKNOWN] = PGKEY_c;
  keymap[DIKI_D - DIKI_UNKNOWN] = PGKEY_d;
  keymap[DIKI_E - DIKI_UNKNOWN] = PGKEY_e;
  keymap[DIKI_F - DIKI_UNKNOWN] = PGKEY_f;
  keymap[DIKI_G - DIKI_UNKNOWN] = PGKEY_g;
  keymap[DIKI_H - DIKI_UNKNOWN] = PGKEY_h;
  keymap[DIKI_I - DIKI_UNKNOWN] = PGKEY_i;
  keymap[DIKI_J - DIKI_UNKNOWN] = PGKEY_j;
  keymap[DIKI_K - DIKI_UNKNOWN] = PGKEY_k;
  keymap[DIKI_L - DIKI_UNKNOWN] = PGKEY_l;
  keymap[DIKI_M - DIKI_UNKNOWN] = PGKEY_m;
  keymap[DIKI_N - DIKI_UNKNOWN] = PGKEY_n;
  keymap[DIKI_O - DIKI_UNKNOWN] = PGKEY_o;
  keymap[DIKI_P - DIKI_UNKNOWN] = PGKEY_p;
  keymap[DIKI_Q - DIKI_UNKNOWN] = PGKEY_q;
  keymap[DIKI_R - DIKI_UNKNOWN] = PGKEY_r;
  keymap[DIKI_S - DIKI_UNKNOWN] = PGKEY_s;
  keymap[DIKI_T - DIKI_UNKNOWN] = PGKEY_t;
  keymap[DIKI_U - DIKI_UNKNOWN] = PGKEY_u;
  keymap[DIKI_V - DIKI_UNKNOWN] = PGKEY_v;
  keymap[DIKI_W - DIKI_UNKNOWN] = PGKEY_w;
  keymap[DIKI_X - DIKI_UNKNOWN] = PGKEY_x;
  keymap[DIKI_Y - DIKI_UNKNOWN] = PGKEY_y;
  keymap[DIKI_Z - DIKI_UNKNOWN] = PGKEY_z;
  
  keymap[DIKI_0 - DIKI_UNKNOWN] = PGKEY_0;
  keymap[DIKI_1 - DIKI_UNKNOWN] = PGKEY_1;
  keymap[DIKI_2 - DIKI_UNKNOWN] = PGKEY_2;
  keymap[DIKI_3 - DIKI_UNKNOWN] = PGKEY_3;
  keymap[DIKI_4 - DIKI_UNKNOWN] = PGKEY_4;
  keymap[DIKI_5 - DIKI_UNKNOWN] = PGKEY_5;
  keymap[DIKI_6 - DIKI_UNKNOWN] = PGKEY_6;
  keymap[DIKI_7 - DIKI_UNKNOWN] = PGKEY_7;
  keymap[DIKI_8 - DIKI_UNKNOWN] = PGKEY_8;
  keymap[DIKI_9 - DIKI_UNKNOWN] = PGKEY_9;
  
  keymap[DIKI_F1 - DIKI_UNKNOWN] = PGKEY_F1;
  keymap[DIKI_F2 - DIKI_UNKNOWN] = PGKEY_F2;
  keymap[DIKI_F3 - DIKI_UNKNOWN] = PGKEY_F3;
  keymap[DIKI_F4 - DIKI_UNKNOWN] = PGKEY_F4;
  keymap[DIKI_F5 - DIKI_UNKNOWN] = PGKEY_F5;
  keymap[DIKI_F6 - DIKI_UNKNOWN] = PGKEY_F6;
  keymap[DIKI_F7 - DIKI_UNKNOWN] = PGKEY_F7;
  keymap[DIKI_F8 - DIKI_UNKNOWN] = PGKEY_F8;
  keymap[DIKI_F9 - DIKI_UNKNOWN] = PGKEY_F9;
  keymap[DIKI_F10 - DIKI_UNKNOWN] = PGKEY_F10;
  keymap[DIKI_F11 - DIKI_UNKNOWN] = PGKEY_F11;
  keymap[DIKI_F12 - DIKI_UNKNOWN] = PGKEY_F12;
  
  keymap[DIKI_ESCAPE - DIKI_UNKNOWN] = PGKEY_ESCAPE;
  keymap[DIKI_LEFT - DIKI_UNKNOWN] = PGKEY_LEFT;
  keymap[DIKI_RIGHT - DIKI_UNKNOWN] = PGKEY_RIGHT;
  keymap[DIKI_UP - DIKI_UNKNOWN] = PGKEY_UP;
  keymap[DIKI_DOWN - DIKI_UNKNOWN] = PGKEY_DOWN;
  keymap[DIKI_CONTROL_L - DIKI_UNKNOWN] = PGKEY_LCTRL;
  keymap[DIKI_CONTROL_R - DIKI_UNKNOWN] = PGKEY_RCTRL;
  keymap[DIKI_SHIFT_L - DIKI_UNKNOWN] = PGKEY_LSHIFT;
  keymap[DIKI_SHIFT_R - DIKI_UNKNOWN] = PGKEY_RSHIFT;
  keymap[DIKI_ALT_L - DIKI_UNKNOWN] = PGKEY_LALT;
  keymap[DIKI_ALTGR - DIKI_UNKNOWN] = PGKEY_RALT;
  keymap[DIKI_TAB - DIKI_UNKNOWN] = PGKEY_TAB;
  keymap[DIKI_ENTER - DIKI_UNKNOWN] = PGKEY_RETURN;
  keymap[DIKI_SPACE - DIKI_UNKNOWN] = PGKEY_SPACE;
  keymap[DIKI_BACKSPACE - DIKI_UNKNOWN] = PGKEY_BACKSPACE;
  keymap[DIKI_INSERT - DIKI_UNKNOWN] = PGKEY_INSERT;
  keymap[DIKI_DELETE - DIKI_UNKNOWN] = PGKEY_DELETE;
  keymap[DIKI_HOME - DIKI_UNKNOWN] = PGKEY_HOME;
  keymap[DIKI_END - DIKI_UNKNOWN] = PGKEY_END;
  keymap[DIKI_PAGE_UP - DIKI_UNKNOWN] = PGKEY_PAGEUP;
  keymap[DIKI_PAGE_DOWN - DIKI_UNKNOWN] = PGKEY_PAGEDOWN;
  keymap[DIKI_CAPS_LOCK - DIKI_UNKNOWN] = PGKEY_CAPSLOCK;
  keymap[DIKI_NUM_LOCK - DIKI_UNKNOWN] = PGKEY_NUMLOCK;
  keymap[DIKI_SCROLL_LOCK - DIKI_UNKNOWN] = PGKEY_SCROLLOCK;
  keymap[DIKI_PRINT - DIKI_UNKNOWN] = PGKEY_PRINT;
  keymap[DIKI_PAUSE - DIKI_UNKNOWN] = PGKEY_PAUSE;
  keymap[DIKI_KP_DIV - DIKI_UNKNOWN] = PGKEY_KP_DIVIDE;
  keymap[DIKI_KP_MULT - DIKI_UNKNOWN] = PGKEY_KP_MULTIPLY;
  keymap[DIKI_KP_MINUS - DIKI_UNKNOWN] = PGKEY_KP_MINUS;
  keymap[DIKI_KP_PLUS - DIKI_UNKNOWN] = PGKEY_KP_PLUS;
  keymap[DIKI_KP_ENTER - DIKI_UNKNOWN] = PGKEY_KP_ENTER;
}


static u32 directfb_translatekey (DFBInputEvent *ev) {
  u32 key;

  if (ev->key_symbol > 0 && ev->key_symbol < 128)
    return ev->key_symbol;
  else
    return keymap[ev->key_id - DIKI_UNKNOWN];
}

static u32 directfb_translatemods (DFBInputEvent *ev) {
  u32 mods = 0;
  if (ev->modifiers && DIMKI_SHIFT)
    mods |= PGMOD_SHIFT;

  if (ev->modifiers && DIMKI_CONTROL)
    mods |= PGMOD_CTRL;

  if (ev->modifiers && DIMKI_ALT)
    mods |= PGMOD_ALT;

  if (ev->modifiers && DIMKI_META)
    mods |= PGMOD_META;
  return mods;
}

void directfbinput_poll(void) {
  DFBResult err;
  DFBEvent event;
  u32 key, mods;
  static int pgx = 0, pgy = 0;

  while (evb->HasEvent (evb) == DFB_OK) {
    err = evb->GetEvent (evb, &event);
    dfb_errcheck_die;

    /* should never happen. Still... */
    if (event.clazz != DFEC_INPUT)
      continue;

    key = directfb_translatekey(&event.input);
    mods = directfb_translatemods(&event.input);

    switch (event.input.type) {
    case DIET_KEYPRESS:
      /* a key is been pressed */
      infilter_send_key(PG_TRIGGER_KEYDOWN, key, mods);
      if ((event.input.key_symbol >= 32) && (event.input.key_symbol < 127))
	infilter_send_key(PG_TRIGGER_CHAR, event.input.key_symbol, mods);
      break;

    case DIET_KEYRELEASE:
      /* a key is been released */
      infilter_send_key(PG_TRIGGER_KEYUP, key, mods);
      break;

    case DIET_BUTTONPRESS:
      /* a (mouse) button is been pressed */
      infilter_send_pointing(PG_TRIGGER_DOWN, pgx, pgy, 
			     event.input.buttons, directfbinput_cursor);
      break;

    case DIET_BUTTONRELEASE:
      /* a (mouse) button is been released */
      infilter_send_pointing(PG_TRIGGER_UP, pgx, pgy, 
			     event.input.buttons, directfbinput_cursor);
      break;

    case DIET_AXISMOTION:
      /* mouse/joystick movement */
      switch (event.input.axis) {
      case 0:
	pgx = event.input.axisabs;
	break;
      case 1:
	pgy = event.input.axisabs;
	break;
      /* why a switch? because we may have more than 2 axii
       * (eg, in a 3d mouse or a mouse with wheels).
       * Currently, other axii are ignored.
       */
      }
      infilter_send_pointing(PG_TRIGGER_MOVE, pgx, pgy, event.input.buttons,
			     directfbinput_cursor);
      break;
    }
  }
}

g_error directfbinput_init(void) {
  g_error e;
  DFBResult err;

  if (!directfb)
    return mkerror(PG_ERRT_BADPARAM,36);   /* No matching video driver */

  err = directfb->CreateInputEventBuffer(directfb, DICAPS_ALL, DFB_FALSE, &evb);
  dfb_errcheck_die;

  init_keymap();

  //e = cursor_new(&directfbinput_cursor, NULL, -1);
  /* we don't want two cursors drawn...
   * dfb's one is already good enough.
   * when sprites are fixed, we might want to make this a
   * pair of configuration options, like in sdlinput
   */

  return success;
}

/* Polling time for the input driver */ 
void directfbinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if (timeout->tv_sec > 0 || timeout->tv_usec > POLL_USEC) {
    timeout->tv_sec = 0;
    timeout->tv_usec = POLL_USEC;
  }
}

void directfbinput_close(void) {
  if (directfbinput_cursor)
    pointer_free(-1,directfbinput_cursor);
}

void directfbinput_message(u32 message, u32 param, u32 *ret) {
  switch (message) {
    
  case PGDM_MOUSEWARP:
    //SDL_WarpMouse(param>>16, param & 0xFFFF);
    break;

  }
}


/******************************************** Driver registration */

g_error directfbinput_regfunc(struct inlib *i) {
  i->init = &directfbinput_init;
  i->poll = &directfbinput_poll;
  i->fd_init = &directfbinput_fd_init;
  i->close = &directfbinput_close;
  i->message = &directfbinput_message;

  return success;
}

/* The End */
