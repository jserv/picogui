/* $Id$
 *
 * vncinput.h - input driver for VNC events
 *
 *    NOTE: The keysym translation code here was
 *          lifted from the SDL_x11events.c file in SDL.
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
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/configfile.h>
#include "../video/libvncserver/rfb.h"

extern rfbScreenInfoPtr vncserver_screeninfo;      /* VNC server's main structure */
int vncinput_pipe[2];                              /* Pipe for sending events from the VNC server
						    * thread to the input driver in this thread.*/

/* The structure sent across our pipe */
struct vncinput_packet {
  bool createCursor, destroyCursor;
  struct cursor **pcursor;
  struct cursor *cursor;
  u32 trigger;
  union trigparam param;
};

int vncinput_scroll_distance;

void vncinput_ptrAddEvent(int buttonMask, int x, int y, struct _rfbClientRec* cl);
void vncinput_kbdAddEvent(Bool down, KeySym keySym, struct _rfbClientRec* cl);
enum rfbNewClientAction vncinput_newClientHook(struct _rfbClientRec* cl);
void vncinput_clientGoneHook(struct _rfbClientRec* cl);
void vnc_translate_key(bool press, KeySym xsym, s16 *psym, s16 *pmod, s16 *pchr);
void vncinput_init_keymap(void);
static s16 ODD_keymap[256];
static s16 MISC_keymap[256];


/******************************************************** Keymaps */

/* From SDL's X11_TranslateKey() function */
void vnc_translate_key(bool press, KeySym xsym, s16 *psym, s16 *pmod, s16 *pchr) {
  s16 sym,chr;
  static s16 mod = 0;

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

  /* Keep track of modifier state */
  if (sym) {
    if (press) {
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
    else {
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

  /* Get the Unicode translation */
  chr = 0;
  if (sym > 0 && sym <= 0xFF) {

    if (mod == 0) {
      /* Unmodified key */
      chr = sym;
    }

    else if ((mod & PGMOD_SHIFT) && !(mod & ~(PGMOD_SHIFT | PGMOD_CAPS | PGMOD_NUM))) {
      /* Shifted key */
      if (mod & PGMOD_CAPS)
	chr = sym;
      else
	chr = isupper(sym) ? tolower(sym) : toupper(sym);
      
      switch (chr) {
      case '`':  chr = '~'; break;
      case '1':  chr = '!'; break;
      case '2':  chr = '@'; break;
      case '3':  chr = '#'; break;
      case '4':  chr = '$'; break;
      case '5':  chr = '%'; break;
      case '6':  chr = '^'; break;
      case '7':  chr = '&'; break;
      case '8':  chr = '*'; break;
      case '9':  chr = '('; break;
      case '0':  chr = ')'; break;
      case '-':  chr = '_'; break;
      case '=':  chr = '+'; break;
      case '[':  chr = '{'; break;
      case ']':  chr = '}'; break;
      case '\\': chr = '|'; break;
      case ';':  chr = ':'; break;
      case '\'': chr = '"'; break;
      case ',':  chr = '<'; break;
      case '.':  chr = '>'; break;
      case '/':  chr = '?'; break;
      }
    }

    else if ((mod & PGMOD_CAPS) && !(mod & ~(PGMOD_CAPS | PGMOD_NUM))) {
      /* Caps locked key */
      if (mod & PGMOD_CAPS)
	chr = isupper(sym) ? tolower(sym) : toupper(sym);      
    }

    else if ((mod & PGMOD_CTRL) && !(mod & ~(PGMOD_CTRL | PGMOD_NUM)) && sym >= PGKEY_a && sym <= PGKEY_z) {
      /* Control key */
      chr = sym - PGKEY_a + 1;
    }
  }
  
  /* Return all the values we're interested in */
  if (psym) *psym = sym;  
  if (pmod) *pmod = mod;
  if (pchr) *pchr = chr;
}

/* This is all from SDL's X11_InitKeymap() function */
void vncinput_init_keymap(void) {
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


/******************************************************** VNC Hooks */

void vncinput_clientGoneHook(struct _rfbClientRec* cl) {
  struct vncinput_packet pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.pcursor = (struct cursor**) &cl->clientData;
  pkt.cursor = (struct cursor*) cl->clientData;

  pkt.destroyCursor = 1;

  write(vncinput_pipe[1], &pkt, sizeof(pkt));
}

enum rfbNewClientAction vncinput_newClientHook(struct _rfbClientRec* cl) {
  struct vncinput_packet pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.pcursor = (struct cursor**) &cl->clientData;
  pkt.cursor = (struct cursor*) cl->clientData;

  pkt.createCursor = 1;

  write(vncinput_pipe[1], &pkt, sizeof(pkt));
  cl->clientGoneHook = &vncinput_clientGoneHook;
}

void vncinput_kbdAddEvent(Bool down, KeySym keySym, struct _rfbClientRec* cl) {
  struct vncinput_packet pkt;
  s16 pgkey = 0, pgchar = 0, pgmods = 0;

  vnc_translate_key(down, keySym, &pgkey, &pgmods, &pgchar);
  
  if (down && pgchar) {
    memset(&pkt, 0, sizeof(pkt));
    pkt.trigger = PG_TRIGGER_CHAR;
    pkt.param.kbd.key = pgchar;
    pkt.param.kbd.mods = pgmods;
    write(vncinput_pipe[1], &pkt, sizeof(pkt));
  }

  memset(&pkt, 0, sizeof(pkt));
  pkt.trigger = down ? PG_TRIGGER_KEYDOWN : PG_TRIGGER_KEYUP;
  pkt.param.kbd.key = pgkey;
  pkt.param.kbd.mods = pgmods;
  write(vncinput_pipe[1], &pkt, sizeof(pkt));
}

void vncinput_ptrAddEvent(int buttonMask, int x, int y, struct _rfbClientRec* cl) {
  struct vncinput_packet pkt;
  memset(&pkt, 0, sizeof(pkt));
  pkt.pcursor = (struct cursor**) &cl->clientData;
  pkt.cursor = (struct cursor*) cl->clientData;


  /* Map scroll wheel events */
  if (buttonMask & 0x18) {
    pkt.trigger = PG_TRIGGER_SCROLLWHEEL;
    if (buttonMask & 0x08)
      pkt.param.mouse.y = -vncinput_scroll_distance;
    else
      pkt.param.mouse.y = vncinput_scroll_distance;
  }
  else {
    pkt.trigger = PG_TRIGGER_PNTR_STATUS;
    pkt.param.mouse.x = x;
    pkt.param.mouse.y = y;
    pkt.param.mouse.btn = buttonMask;
  }

  write(vncinput_pipe[1], &pkt, sizeof(pkt));
}


/******************************************************** Public Methods */

g_error vncinput_init(void) {
  vncinput_init_keymap();
  vncinput_scroll_distance = get_param_int("input-vnc","scroll_distance",20);

  /* Create a pipe for sending events from the VNC server thread to the input driver */
  pipe(vncinput_pipe);

  vncserver_screeninfo->newClientHook = &vncinput_newClientHook;
  vncserver_screeninfo->kbdAddEvent = &vncinput_kbdAddEvent;
  vncserver_screeninfo->ptrAddEvent = &vncinput_ptrAddEvent;
  return success;
}

void vncinput_close(void) {
  close(vncinput_pipe[0]);
  close(vncinput_pipe[1]);
}

void vncinput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if ((*n)<(vncinput_pipe[0]+1))
    *n = vncinput_pipe[0]+1;
  if (vncinput_pipe[0]>0)
    FD_SET(vncinput_pipe[0],readfds);
}

int vncinput_fd_activate(int fd) {
  struct vncinput_packet pkt;

  if (fd != vncinput_pipe[0])
    return 0;
  read(vncinput_pipe[0], &pkt, sizeof(pkt));

  if (pkt.createCursor)
    cursor_new(pkt.pcursor, NULL, -1);

  if (pkt.destroyCursor)
    pointer_free(-1, pkt.cursor);

  if (pkt.trigger & PG_TRIGGERS_MOUSE)
    pkt.param.mouse.cursor = pkt.cursor;

  if (pkt.trigger)
    infilter_send(NULL, pkt.trigger, &pkt.param);

  return 1;
}


/******************************************************** Registration */

g_error vncinput_regfunc(struct inlib *i) {
  i->init = &vncinput_init;
  i->close = &vncinput_close;
  i->fd_init = &vncinput_fd_init;
  i->fd_activate = &vncinput_fd_activate;
  return success;
}

/* The End */
