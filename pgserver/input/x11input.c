/* $Id: x11input.c,v 1.20 2002/11/04 10:29:34 micahjd Exp $
 *
 * x11input.h - input driver for X11 events
 *
 *    NOTE: Much of the code in here, especially the keyboard translation,
 *          was lifted from the SDL_x11events.c file in SDL.
 *          The Simple Directmedia Layer is:
 *             Copyright (C) 1997-2001 Sam Lantinga
 *             http://libsdl.org
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/configfile.h>
#include <pgserver/x11.h>

int x11input_scroll_distance;

/* Keyboard translation utilities */
void x11_translate_key(Display *display, XKeyEvent *xkey, KeyCode kc,
		       s16 *psym, s16 *pmod, s16 *pchr);
int x11_key_repeat(Display *display, XEvent *event);
void x11input_init_keymap(void);
static s16 ODD_keymap[256];
static s16 MISC_keymap[256];


/******************************************************** Public Methods */

g_error x11input_init(null) {
  g_error e;

  /* Get a file descriptor for the X11 display */
  if (!x11_display)
    return mkerror(PG_ERRT_BADPARAM,36);   /* No matching video driver */
  
  x11input_init_keymap();
  x11input_scroll_distance = get_param_int("input-x11","scroll_distance",20);

  return success;
}

void x11input_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if ((*n)<(x11_fd+1))
    *n = x11_fd+1;
  FD_SET(x11_fd,readfds);
}

/* Recieve and process X events */
int x11input_fd_activate(int fd) {
  int i;
  XEvent ev;
  s16 mod,sym,chr;
  Region expose_region = XCreateRegion();
  XRectangle rect;
  u32 btn;

  if(fd != x11_fd) return 0;

  for (i=XPending(x11_display);i;i--) {
    XNextEvent(x11_display, &ev);
    switch (ev.type) {

      /****************** Expose event 
       *
       * This is all handled by the video driver...
       */

    case Expose:
      /* Union our current expose rectangle with this one */
      rect.x = ev.xexpose.x;
      rect.y = ev.xexpose.y;
      rect.width = ev.xexpose.width;
      rect.height = ev.xexpose.height;
      XUnionRectWithRegion(&rect,expose_region,expose_region);

      /* If this is the last contiguous expose event, go ahead and draw */
      if (!ev.xexpose.count) {
	x11_expose(ev.xexpose.window,expose_region);
	XDestroyRegion(expose_region);
	expose_region = XCreateRegion();
      }
      break;

      /****************** Resizing */

    case ConfigureNotify: 
      video_setmode(ev.xconfigure.width, ev.xconfigure.height,
		    vid->bpp, PG_FM_ON,0); 
      update(NULL,1);
      break;

      /****************** Mouse events
       *
       * Pretty straightforward, except that we have to translate
       * the X11 mouse button codes. PicoGUI uses bits 0, 1, 2, etc.
       * to represent mouse buttons 1, 2, 3, and so on. X11 uses
       * a similar scheme but starts at bit 8. Also, the button state
       * passed along with each event is the _previous_ state, not
       * the state after the event in question. This little bit of
       * shifing and logical cruft fixes it.
       * We also need to translate X11's buttons 4 and 5 into scroll wheel events.
       */

    case MotionNotify:
      infilter_send_pointing(PG_TRIGGER_MOVE,ev.xmotion.x, ev.xmotion.y, 
			     (ev.xmotion.state >> 8) & 0x07, NULL);
      break;

    case ButtonPress:
      btn = (ev.xbutton.state >> 8) | (1 << (ev.xbutton.button-1));
      if (btn & 0x08)
	infilter_send_pointing(PG_TRIGGER_SCROLLWHEEL,0,-x11input_scroll_distance,0,NULL);
      if (btn & 0x10)
	infilter_send_pointing(PG_TRIGGER_SCROLLWHEEL,0,x11input_scroll_distance,0,NULL);
	
      infilter_send_pointing(PG_TRIGGER_DOWN,ev.xbutton.x, ev.xbutton.y,
			     btn & 0x07, NULL);
      break;

    case ButtonRelease:
      infilter_send_pointing(PG_TRIGGER_UP,ev.xbutton.x, ev.xbutton.y,
			     ((ev.xbutton.state >> 8) & (~(1 << (ev.xbutton.button-1)))) & 0x07,
			     NULL);
      break;

      /****************** Keyboard events
       *
       * In X11, keyboard repeats are release events, not press events...
       * This code needs to send KEYDOWN and KEYUP only when the key
       * is actually pressed or released, and send CHAR events when the
       * key is pressed and when it repeats.
       */

    case KeyPress:
      x11_translate_key(x11_display, &ev.xkey, ev.xkey.keycode, &sym, &mod, &chr);
      if (sym)
	infilter_send_key(PG_TRIGGER_KEYDOWN,sym,mod);
      if (chr)
	infilter_send_key(PG_TRIGGER_CHAR,chr,mod);
      break;

    case KeyRelease:
      x11_translate_key(x11_display, &ev.xkey, ev.xkey.keycode, &sym, &mod, &chr);
      if (x11_key_repeat(x11_display, &ev)) {
	if (chr)
	  infilter_send_key(PG_TRIGGER_CHAR,chr,mod);
      }
      else {
	if (sym)
	  infilter_send_key(PG_TRIGGER_KEYUP,sym,mod);
      }
      break;

    }
  }
  return 1;
}


/******************************************************** Internal Utilities */

/* Check to see if this is a repeated key.
 * This code was borrowed from SDL, which borrowed it from GII :)
 */
int x11_key_repeat(Display *display, XEvent *event) {
  XEvent peekevent;
  int repeated;
  
  repeated = 0;
  if ( XPending(display) ) {
    XPeekEvent(display, &peekevent);
    if ( (peekevent.type == KeyPress) &&
	 (peekevent.xkey.keycode == event->xkey.keycode) &&
	 ((peekevent.xkey.time-event->xkey.time) < 2) ) {
      repeated = 1;
      XNextEvent(display, &peekevent);
    }
  }
  return repeated;
}

/* From SDL's X11_TranslateKey() function */
void x11_translate_key(Display *display, XKeyEvent *xkey, KeyCode kc,
		       s16 *psym, s16 *pmod, s16 *pchr) {
  KeySym xsym;
  s16 sym,chr;
  static s16 mod = 0;

  xsym = XKeycodeToKeysym(display, kc, 0);

  /* Get the translated virtual keysym */
  sym = 0;
  if (xsym) {
    switch (xsym>>8) {
    case 0x1005FF:
#ifdef SunXK_F36
      if ( xsym == SunXK_F36 )
	sym = PGKEY_F11;
#endif
#ifdef SunXK_F37
      if ( xsym == SunXK_F37 )
	sym = PGKEY_F12;
#endif
      break;
    case 0x00:        /* Latin 1 */
    case 0x01:        /* Latin 2 */
    case 0x02:        /* Latin 3 */
    case 0x03:        /* Latin 4 */
    case 0x04:        /* Katakana */
    case 0x05:        /* Arabic */
    case 0x06:        /* Cyrillic */
    case 0x07:        /* Greek */
    case 0x08:        /* Technical */
    case 0x0A:        /* Publishing */
    case 0x0C:        /* Hebrew */
    case 0x0D:        /* Thai */
      sym = (s16)(xsym&0xFF);
      /* Map capital letter syms to lowercase */
      if ((sym >= 'A')&&(sym <= 'Z'))
	sym += ('a'-'A');
      break;
    case 0xFE:
      sym = ODD_keymap[xsym&0xFF];
      break;
    case 0xFF:
      sym = MISC_keymap[xsym&0xFF];
      break;
    default:
      /* Unknown key */
      break;
    }
  } 
  else {
    /* X11 doesn't know how to translate the key! */
    switch (kc) {
      /* Caution:
	 These keycodes are from the Microsoft Keyboard
      */
    case 115:
      sym = PGKEY_LSUPER;
      break;
    case 116:
      sym = PGKEY_RSUPER;
      break;
    case 117:
      sym = PGKEY_MENU;
      break;
    default:
      /*
       * no point in an error message; happens for
       * several keys when we get a keymap notify
       */
      break;
    }
  }

  /* Get the Unicode translation */
  chr = 0;
  if (xkey) {
    static XComposeStatus state;
    /* Until we handle the IM protocol, use XLookupString() */
    unsigned char keybuf[32];
    
    /* Look up the translated value for the key event */
    if ( XLookupString(xkey, (char *)keybuf, sizeof(keybuf),
		       NULL, &state) ) {
      /*
       * FIXME,: XLookupString() may yield more than one
       * character, so we need a mechanism to allow for
       * this (perhaps generate null keypress events with
       * a unicode value)
       */
      chr = keybuf[0];
    }
  }

  /* Keep track of modifier state */
  if (sym) {
    if (xkey->type == KeyPress) {
      switch (sym) {
	
      case PGKEY_LSHIFT:   mod |= PGMOD_LSHIFT; break;
      case PGKEY_RSHIFT:   mod |= PGMOD_RSHIFT; break;
      case PGKEY_LCTRL:    mod |= PGMOD_LCTRL;  break;
      case PGKEY_RCTRL:    mod |= PGMOD_RCTRL;  break;
      case PGKEY_LALT:     mod |= PGMOD_LALT;   break;
      case PGKEY_RALT:     mod |= PGMOD_RALT;   break;
      case PGKEY_LMETA:    mod |= PGMOD_LMETA;  break;
      case PGKEY_RMETA:    mod |= PGMOD_RMETA;  break;
      case PGKEY_NUMLOCK:  mod ^= PGMOD_NUM;    break;
      case PGKEY_CAPSLOCK: mod ^= PGMOD_CAPS;   break;

      }
    }
    else if (xkey->type == KeyRelease) {
      switch (sym) {
	
      case PGKEY_LSHIFT:   mod &= ~PGMOD_LSHIFT; break;
      case PGKEY_RSHIFT:   mod &= ~PGMOD_RSHIFT; break;
      case PGKEY_LCTRL:    mod &= ~PGMOD_LCTRL;  break;
      case PGKEY_RCTRL:    mod &= ~PGMOD_RCTRL;  break;
      case PGKEY_LALT:     mod &= ~PGMOD_LALT;   break;
      case PGKEY_RALT:     mod &= ~PGMOD_RALT;   break;
      case PGKEY_LMETA:    mod &= ~PGMOD_LMETA;  break;
      case PGKEY_RMETA:    mod &= ~PGMOD_RMETA;  break;

      }     
    }
  }
  
  /* Return all the values we're interested in */
  if (psym) *psym = sym;  
  if (pmod) *pmod = mod;
  if (pchr) *pchr = chr;
}

/* This is all from SDL's X11_InitKeymap() function */
void x11input_init_keymap(void) {
  /* Odd keys used in international keyboards */
  
#ifdef XK_dead_circumflex
  /* These X keysyms have 0xFE as the high byte */
  ODD_keymap[XK_dead_circumflex&0xFF] = PGKEY_CARET;
#endif
  
  /* Map the miscellaneous keys */
  
  /* These X keysyms have 0xFF as the high byte */
  MISC_keymap[XK_BackSpace&0xFF] = PGKEY_BACKSPACE;
  MISC_keymap[XK_Tab&0xFF] = PGKEY_TAB;
  MISC_keymap[XK_Clear&0xFF] = PGKEY_CLEAR;
  MISC_keymap[XK_Return&0xFF] = PGKEY_RETURN;
  MISC_keymap[XK_Pause&0xFF] = PGKEY_PAUSE;
  MISC_keymap[XK_Escape&0xFF] = PGKEY_ESCAPE;
  MISC_keymap[XK_Delete&0xFF] = PGKEY_DELETE;
  
  MISC_keymap[XK_KP_0&0xFF] = PGKEY_KP0;                /* Keypad 0-9 */
  MISC_keymap[XK_KP_1&0xFF] = PGKEY_KP1;
  MISC_keymap[XK_KP_2&0xFF] = PGKEY_KP2;
  MISC_keymap[XK_KP_3&0xFF] = PGKEY_KP3;
  MISC_keymap[XK_KP_4&0xFF] = PGKEY_KP4;
  MISC_keymap[XK_KP_5&0xFF] = PGKEY_KP5;
  MISC_keymap[XK_KP_6&0xFF] = PGKEY_KP6;
  MISC_keymap[XK_KP_7&0xFF] = PGKEY_KP7;
  MISC_keymap[XK_KP_8&0xFF] = PGKEY_KP8;
  MISC_keymap[XK_KP_9&0xFF] = PGKEY_KP9;
  MISC_keymap[XK_KP_Insert&0xFF] = PGKEY_KP0;
  MISC_keymap[XK_KP_End&0xFF] = PGKEY_KP1;        
  MISC_keymap[XK_KP_Down&0xFF] = PGKEY_KP2;
  MISC_keymap[XK_KP_Page_Down&0xFF] = PGKEY_KP3;
  MISC_keymap[XK_KP_Left&0xFF] = PGKEY_KP4;
  MISC_keymap[XK_KP_Begin&0xFF] = PGKEY_KP5;
  MISC_keymap[XK_KP_Right&0xFF] = PGKEY_KP6;
  MISC_keymap[XK_KP_Home&0xFF] = PGKEY_KP7;
  MISC_keymap[XK_KP_Up&0xFF] = PGKEY_KP8;
  MISC_keymap[XK_KP_Page_Up&0xFF] = PGKEY_KP9;
  MISC_keymap[XK_KP_Delete&0xFF] = PGKEY_KP_PERIOD;
  MISC_keymap[XK_KP_Decimal&0xFF] = PGKEY_KP_PERIOD;
  MISC_keymap[XK_KP_Divide&0xFF] = PGKEY_KP_DIVIDE;
  MISC_keymap[XK_KP_Multiply&0xFF] = PGKEY_KP_MULTIPLY;
  MISC_keymap[XK_KP_Subtract&0xFF] = PGKEY_KP_MINUS;
  MISC_keymap[XK_KP_Add&0xFF] = PGKEY_KP_PLUS;
  MISC_keymap[XK_KP_Enter&0xFF] = PGKEY_KP_ENTER;
  MISC_keymap[XK_KP_Equal&0xFF] = PGKEY_KP_EQUALS;
  
  MISC_keymap[XK_Up&0xFF] = PGKEY_UP;
  MISC_keymap[XK_Down&0xFF] = PGKEY_DOWN;
  MISC_keymap[XK_Right&0xFF] = PGKEY_RIGHT;
  MISC_keymap[XK_Left&0xFF] = PGKEY_LEFT;
  MISC_keymap[XK_Insert&0xFF] = PGKEY_INSERT;
  MISC_keymap[XK_Home&0xFF] = PGKEY_HOME;
  MISC_keymap[XK_End&0xFF] = PGKEY_END;
  MISC_keymap[XK_Page_Up&0xFF] = PGKEY_PAGEUP;
  MISC_keymap[XK_Page_Down&0xFF] = PGKEY_PAGEDOWN;
  
  MISC_keymap[XK_F1&0xFF] = PGKEY_F1;
  MISC_keymap[XK_F2&0xFF] = PGKEY_F2;
  MISC_keymap[XK_F3&0xFF] = PGKEY_F3;
  MISC_keymap[XK_F4&0xFF] = PGKEY_F4;
  MISC_keymap[XK_F5&0xFF] = PGKEY_F5;
  MISC_keymap[XK_F6&0xFF] = PGKEY_F6;
  MISC_keymap[XK_F7&0xFF] = PGKEY_F7;
  MISC_keymap[XK_F8&0xFF] = PGKEY_F8;
  MISC_keymap[XK_F9&0xFF] = PGKEY_F9;
  MISC_keymap[XK_F10&0xFF] = PGKEY_F10;
  MISC_keymap[XK_F11&0xFF] = PGKEY_F11;
  MISC_keymap[XK_F12&0xFF] = PGKEY_F12;
  MISC_keymap[XK_F13&0xFF] = PGKEY_F13;
  MISC_keymap[XK_F14&0xFF] = PGKEY_F14;
  MISC_keymap[XK_F15&0xFF] = PGKEY_F15;
  
  MISC_keymap[XK_Num_Lock&0xFF] = PGKEY_NUMLOCK;
  MISC_keymap[XK_Caps_Lock&0xFF] = PGKEY_CAPSLOCK;
  MISC_keymap[XK_Scroll_Lock&0xFF] = PGKEY_SCROLLOCK;
  MISC_keymap[XK_Shift_R&0xFF] = PGKEY_RSHIFT;
  MISC_keymap[XK_Shift_L&0xFF] = PGKEY_LSHIFT;
  MISC_keymap[XK_Control_R&0xFF] = PGKEY_RCTRL;
  MISC_keymap[XK_Control_L&0xFF] = PGKEY_LCTRL;
  MISC_keymap[XK_Alt_R&0xFF] = PGKEY_RALT;
  MISC_keymap[XK_Alt_L&0xFF] = PGKEY_LALT;
  MISC_keymap[XK_Meta_R&0xFF] = PGKEY_RMETA;
  MISC_keymap[XK_Meta_L&0xFF] = PGKEY_LMETA;
  MISC_keymap[XK_Super_L&0xFF] = PGKEY_LSUPER;     /* Left "Windows" */
  MISC_keymap[XK_Super_R&0xFF] = PGKEY_RSUPER;     /* Right "Windows */
  MISC_keymap[XK_Mode_switch&0xFF] = PGKEY_MODE;   /* "Alt Gr" key */
#ifdef PGKEY_COMPOSE
  MISC_keymap[XK_Multi_key&0xFF] = PGKEY_COMPOSE;  /* Multi-key compose */
#endif  

  MISC_keymap[XK_Help&0xFF] = PGKEY_HELP;
  MISC_keymap[XK_Print&0xFF] = PGKEY_PRINT;
  MISC_keymap[XK_Sys_Req&0xFF] = PGKEY_SYSREQ;
  MISC_keymap[XK_Break&0xFF] = PGKEY_BREAK;
  MISC_keymap[XK_Menu&0xFF] = PGKEY_MENU;
  MISC_keymap[XK_Hyper_R&0xFF] = PGKEY_MENU;       /* Windows "Menu" key */
}


/******************************************************** Registration */

g_error x11input_regfunc(struct inlib *i) {
  i->init = &x11input_init;
  i->fd_init = &x11input_fd_init;
  i->fd_activate = &x11input_fd_activate;
  return success;
}

/* The End */
