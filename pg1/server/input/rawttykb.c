/* $Id$
 *
 * rawttykb.c - A medium-raw TTY keyboard driver that accurately
 *              represents the keyboard at the risk of less compatibility
 *              than ttykb. It is based on the linux console driver from
 *              SDL. The original copyright from SDL_fbevents.c is below.
 *
 *              This is still rather messy.. but it works better than ttykb.
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
 */
/*
  SDL - Simple DirectMedia Layer
  Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Sam Lantinga
  slouken@libsdl.org
*/

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/configfile.h>
#include <pgserver/fbkeys.h>     /* Scancode key constants */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <termios.h>

#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/keyboard.h>

/* The translation tables from a console scancode to a SDL keysym */
#define NUM_VGAKEYMAPS	(1<<KG_CAPSSHIFT)
static u16 vga_keymap[NUM_VGAKEYMAPS][NR_KEYS];
static u16 keymap[128];
static u16 keymap_temp[128]; /* only used at startup */
int kbd_modstate;

#define SDL_TABLESIZE(table)    (sizeof(table)/sizeof(table[0]))

int keyboard_fd;
struct termios saved_kbd_termios;
int saved_kbd_mode;

struct keysym {
  int scancode, sym, mod, unicode;
};

static void rawttykb_vgainitkeymaps(int fd);
int rawttykb_EnterGraphicsMode(void);
void rawttykb_LeaveGraphicsMode(void);
void rawttykb_CloseKeyboard(void);
int rawttykb_OpenKeyboard(void);
static void handle_keyboard(void);
void rawttykb_InitOSKeymap(void);
static struct keysym *TranslateKey(int scancode, struct keysym *keysym);
int rawttykb_fd_activate(int fd);
void rawttykb_fd_init(int *n,fd_set *readfds,struct timeval *timeout);
void rawttykb_close(void);
g_error rawttykb_init(void);


/* Ugh, we have to duplicate the kernel's keysym mapping code...
   Oh, it's not so bad. :-)

   FIXME: Add keyboard LED handling code
*/
static void rawttykb_vgainitkeymaps(int fd) {
  struct kbentry entry;
  int map, i;

  /* Don't do anything if we are passed a closed keyboard */
  if ( fd < 0 ) {
    return;
  }

  /* Load all the keysym mappings */
  for ( map=0; map<NUM_VGAKEYMAPS; ++map ) {
    memset(vga_keymap[map], 0, NR_KEYS*sizeof(u16));
    for ( i=0; i<NR_KEYS; ++i ) {
      entry.kb_table = map;
      entry.kb_index = i;
      if ( ioctl(fd, KDGKBENT, &entry) == 0 ) {
	/* fill keytemp. This replaces SDL_fbkeys.h */
	if ( (map == 0) && (i<128) ) {
	  keymap_temp[i] = entry.kb_value;
	}
	/* The "Enter" key is a special case */
	if ( entry.kb_value == K_ENTER ) {
	  entry.kb_value = K(KT_ASCII,13);
	}
	/* Handle numpad specially as well */
	if ( KTYP(entry.kb_value) == KT_PAD ) {
	  switch ( entry.kb_value ) {
	  case K_P0:
	  case K_P1:
	  case K_P2:
	  case K_P3:
	  case K_P4:
	  case K_P5:
	  case K_P6:
	  case K_P7:
	  case K_P8:
	  case K_P9:
	    vga_keymap[map][i]=entry.kb_value;
	    vga_keymap[map][i]+= '0';
	    break;
	  case K_PPLUS:
	    vga_keymap[map][i]=K(KT_ASCII,'+');
	    break;
	  case K_PMINUS:
	    vga_keymap[map][i]=K(KT_ASCII,'-');
	    break;
	  case K_PSTAR:
	    vga_keymap[map][i]=K(KT_ASCII,'*');
	    break;
	  case K_PSLASH:
	    vga_keymap[map][i]=K(KT_ASCII,'/');
	    break;
	  case K_PENTER:
	    vga_keymap[map][i]=K(KT_ASCII,'\r');
	    break;
	  case K_PCOMMA:
	    vga_keymap[map][i]=K(KT_ASCII,',');
	    break;
	  case K_PDOT:
	    vga_keymap[map][i]=K(KT_ASCII,'.');
	    break;
	  default:
	    break;
	  }
	}
	/* Do the normal key translation */
	if ( (KTYP(entry.kb_value) == KT_LATIN) ||
	     (KTYP(entry.kb_value) == KT_ASCII) ||
	     (KTYP(entry.kb_value) == KT_LETTER) ) {
	  vga_keymap[map][i] = entry.kb_value;
	}
      }
    }
  }
}

int rawttykb_EnterGraphicsMode(void) {
  struct termios keyboard_termios;

  /* Set medium-raw keyboard mode */

  /* Set the terminal input mode */
  if ( tcgetattr(keyboard_fd, &saved_kbd_termios) < 0 ) {
    if ( keyboard_fd > 0 ) {
      close(keyboard_fd);
    }
    keyboard_fd = -1;
    return(-1);
  }
  if ( ioctl(keyboard_fd, KDGKBMODE, &saved_kbd_mode) < 0 ) {
    if ( keyboard_fd > 0 ) {
      close(keyboard_fd);
    }
    keyboard_fd = -1;
    return(-1);
  }
  keyboard_termios = saved_kbd_termios;
  keyboard_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
  keyboard_termios.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
  keyboard_termios.c_cc[VMIN] = 0;
  keyboard_termios.c_cc[VTIME] = 0;
  if (tcsetattr(keyboard_fd, TCSAFLUSH, &keyboard_termios) < 0) {
    rawttykb_CloseKeyboard();
    return(-1);
  }
  /* This will fail if we aren't root or this isn't our tty */
  if ( ioctl(keyboard_fd, KDSKBMODE, K_MEDIUMRAW) < 0 ) {
    rawttykb_CloseKeyboard();
    return(-1);
  }

  return(keyboard_fd);
}

void rawttykb_LeaveGraphicsMode(void) {
  ioctl(keyboard_fd, KDSKBMODE, saved_kbd_mode);
  tcsetattr(keyboard_fd, TCSAFLUSH, &saved_kbd_termios);
  saved_kbd_mode = -1;
}

void rawttykb_CloseKeyboard(void) {
  if ( keyboard_fd >= 0 ) {
    rawttykb_LeaveGraphicsMode();
    if ( keyboard_fd > 0 ) {
      close(keyboard_fd);
    }
  }
  keyboard_fd = -1;
}

int rawttykb_OpenKeyboard(void) {
  int dummy;

  keyboard_fd = open(get_param_str("input-rawttykb","device","/dev/tty"),O_RDWR);
  saved_kbd_mode = -1;
  
  /* Make sure that our input is a console terminal */
  if ( ioctl(keyboard_fd, KDGKBMODE, &dummy) < 0 ) {
    close(keyboard_fd);
    keyboard_fd = -1;
  }

  /* Set nonblocking I/O */
  dummy = 1;
  ioctl(keyboard_fd,FIONBIO,&dummy);
  
  /* Set up keymap */
  rawttykb_vgainitkeymaps(keyboard_fd);
  return(keyboard_fd);
}

static void handle_keyboard(void) {
  unsigned char keybuf[BUFSIZ];
  int i, nread;
  int pressed;
  int scancode;
  struct keysym keysym;
  static int locks = 0, oldlocks = 0;

  nread = read(keyboard_fd, keybuf, BUFSIZ);
  for ( i=0; i<nread; ++i ) {
    scancode = keybuf[i] & 0x7F;
    if ( keybuf[i] & 0x80 ) {
      pressed = 0;
    } else {
      pressed = 1;
    }

    TranslateKey(scancode, &keysym);
    /* Handle Alt-FN for vt switch */
    switch (keysym.sym) {
    case PGKEY_F1:
    case PGKEY_F2:
    case PGKEY_F3:
    case PGKEY_F4:
    case PGKEY_F5:
    case PGKEY_F6:
    case PGKEY_F7:
    case PGKEY_F8:
    case PGKEY_F9:
    case PGKEY_F10:
    case PGKEY_F11:
    case PGKEY_F12:
      if ( kbd_modstate & PGMOD_ALT ) {
	if ( pressed ) {
	  /* Switch VTs */
	  ioctl(keyboard_fd, VT_ACTIVATE, keysym.sym-PGKEY_F1+1);
	}
	break;
      }
      /* Fall through to normal processing */
    default:

      /* Handle modifiers */
      if (pressed)
	switch (keysym.sym) {
	case PGKEY_LSHIFT:   kbd_modstate |= PGMOD_LSHIFT; break;
	case PGKEY_RSHIFT:   kbd_modstate |= PGMOD_RSHIFT; break;
	case PGKEY_LCTRL:    kbd_modstate |= PGMOD_LCTRL;  break;
	case PGKEY_RCTRL:    kbd_modstate |= PGMOD_RCTRL;  break;
	case PGKEY_LALT:     kbd_modstate |= PGMOD_LALT;   break;
	case PGKEY_RALT:     kbd_modstate |= PGMOD_RALT;   break;
	case PGKEY_LMETA:    kbd_modstate |= PGMOD_LMETA;  break;
	case PGKEY_RMETA:    kbd_modstate |= PGMOD_RMETA;  break;
	case PGKEY_NUMLOCK:  locks        |= PGMOD_NUM;    break;
	case PGKEY_CAPSLOCK: locks        |= PGMOD_CAPS;   break;
	}
      else
	switch (keysym.sym) {
	case PGKEY_LSHIFT:   kbd_modstate &= ~PGMOD_LSHIFT; break;
	case PGKEY_RSHIFT:   kbd_modstate &= ~PGMOD_RSHIFT; break;
	case PGKEY_LCTRL:    kbd_modstate &= ~PGMOD_LCTRL;  break;
	case PGKEY_RCTRL:    kbd_modstate &= ~PGMOD_RCTRL;  break;
	case PGKEY_LALT:     kbd_modstate &= ~PGMOD_LALT;   break;
	case PGKEY_RALT:     kbd_modstate &= ~PGMOD_RALT;   break;
	case PGKEY_LMETA:    kbd_modstate &= ~PGMOD_LMETA;  break;
	case PGKEY_RMETA:    kbd_modstate &= ~PGMOD_RMETA;  break;
	case PGKEY_NUMLOCK:  locks        &= ~PGMOD_NUM;    break;
	case PGKEY_CAPSLOCK: locks        &= ~PGMOD_CAPS;   break;
	}

      /* When a lock key is pressed (but not released or repeated) toggle the modifier */
      if (locks & ~oldlocks) {
	u8 led = 0;

	/* Get the current keyboard LED state from the kernel */
	ioctl(keyboard_fd, KDGKBLED, &led);
	perror("KDGKBLED");
	kbd_modstate &= ~(PGMOD_CAPS | PGMOD_NUM);
	if (led & 4) kbd_modstate |= PGMOD_CAPS;
	if (led & 2) kbd_modstate |= PGMOD_NUM;

	printf("old %X %X\n",led,kbd_modstate);

	switch (keysym.sym) {
	case PGKEY_NUMLOCK:  kbd_modstate ^= PGMOD_NUM;    break;
	case PGKEY_CAPSLOCK: kbd_modstate ^= PGMOD_CAPS;   break;
	}

	/* Modify the kernel keyboard flags */
	led &= ~(4 | 2);
	if (kbd_modstate & PGMOD_CAPS) led |= 4;
	if (kbd_modstate & PGMOD_NUM)  led |= 2;
	ioctl(keyboard_fd, KDSKBLED, led);
	perror("KDSKBLED");

	printf("new %X %X\n",led,kbd_modstate);
      }
      oldlocks = locks;

#if 0      /* -- Debuggative cruft */
      guru("Dispatching key:\n"
	   "pressed = %d\n"
	   "scancode = 0x%02X\n"
	   "symbol = %d\n"
	   "unicode = %d\n"
	   "modifiers = 0x%04X",
	   pressed,
	   keysym.scancode,
	   keysym.sym,
	   keysym.unicode,
	   kbd_modstate);
#endif

      /* This maps quite nicely now to a picogui keyboard event or two */
      if (keysym.unicode && pressed)
	infilter_send_key(PG_TRIGGER_CHAR,keysym.unicode,kbd_modstate);
      infilter_send_key(pressed ? PG_TRIGGER_KEYDOWN : PG_TRIGGER_KEYUP,keysym.sym,kbd_modstate);

      break;
    }
  }
}

void rawttykb_InitOSKeymap(void) {
  int i;

  /* Initialize the Linux key translation table */

  /* First get the ascii keys and others not well handled */
  for (i=0; i<SDL_TABLESIZE(keymap); ++i) {
    switch(i) {
      /* These aren't handled by the x86 kernel keymapping (?) */
    case SCANCODE_PRINTSCREEN:
      keymap[i] = PGKEY_PRINT;
      break;
    case SCANCODE_BREAK:
      keymap[i] = PGKEY_BREAK;
      break;
    case SCANCODE_BREAK_ALTERNATIVE:
      keymap[i] = PGKEY_PAUSE;
      break;
    case SCANCODE_LEFTSHIFT:
      keymap[i] = PGKEY_LSHIFT;
      break;
    case SCANCODE_RIGHTSHIFT:
      keymap[i] = PGKEY_RSHIFT;
      break;
    case SCANCODE_LEFTCONTROL:
      keymap[i] = PGKEY_LCTRL;
      break;
    case SCANCODE_RIGHTCONTROL:
      keymap[i] = PGKEY_RCTRL;
      break;
    case SCANCODE_RIGHTWIN:
      keymap[i] = PGKEY_RSUPER;
      break;
    case SCANCODE_LEFTWIN:
      keymap[i] = PGKEY_LSUPER;
      break;
    case 127:
      keymap[i] = PGKEY_MENU;
      break;
      /* this should take care of all standard ascii keys */
    default:
      keymap[i] = KVAL(vga_keymap[0][i]);
      break;
    }
  }
  for (i=0; i<SDL_TABLESIZE(keymap); ++i) {
    switch(keymap_temp[i]) {
    case K_F1:  keymap[i] = PGKEY_F1;  break;
    case K_F2:  keymap[i] = PGKEY_F2;  break;
    case K_F3:  keymap[i] = PGKEY_F3;  break;
    case K_F4:  keymap[i] = PGKEY_F4;  break;
    case K_F5:  keymap[i] = PGKEY_F5;  break;
    case K_F6:  keymap[i] = PGKEY_F6;  break;
    case K_F7:  keymap[i] = PGKEY_F7;  break;
    case K_F8:  keymap[i] = PGKEY_F8;  break;
    case K_F9:  keymap[i] = PGKEY_F9;  break;
    case K_F10: keymap[i] = PGKEY_F10; break;
    case K_F11: keymap[i] = PGKEY_F11; break;
    case K_F12: keymap[i] = PGKEY_F12; break;

    case K_DOWN:  keymap[i] = PGKEY_DOWN;  break;
    case K_LEFT:  keymap[i] = PGKEY_LEFT;  break;
    case K_RIGHT: keymap[i] = PGKEY_RIGHT; break;
    case K_UP:    keymap[i] = PGKEY_UP;    break;

    case K_P0:     keymap[i] = PGKEY_KP0; break;
    case K_P1:     keymap[i] = PGKEY_KP1; break;
    case K_P2:     keymap[i] = PGKEY_KP2; break;
    case K_P3:     keymap[i] = PGKEY_KP3; break;
    case K_P4:     keymap[i] = PGKEY_KP4; break;
    case K_P5:     keymap[i] = PGKEY_KP5; break;
    case K_P6:     keymap[i] = PGKEY_KP6; break;
    case K_P7:     keymap[i] = PGKEY_KP7; break;
    case K_P8:     keymap[i] = PGKEY_KP8; break;
    case K_P9:     keymap[i] = PGKEY_KP9; break;
    case K_PPLUS:  keymap[i] = PGKEY_KP_PLUS; break;
    case K_PMINUS: keymap[i] = PGKEY_KP_MINUS; break;
    case K_PSTAR:  keymap[i] = PGKEY_KP_MULTIPLY; break;
    case K_PSLASH: keymap[i] = PGKEY_KP_DIVIDE; break;
    case K_PENTER: keymap[i] = PGKEY_KP_ENTER; break;
    case K_PDOT:   keymap[i] = PGKEY_KP_PERIOD; break;

    case K_SHIFT:  if ( keymap[i] != PGKEY_RSHIFT )
      keymap[i] = PGKEY_LSHIFT;
      break;
    case K_SHIFTL: keymap[i] = PGKEY_LSHIFT; break;
    case K_SHIFTR: keymap[i] = PGKEY_RSHIFT; break;
    case K_CTRL:  if ( keymap[i] != PGKEY_RCTRL )
      keymap[i] = PGKEY_LCTRL;
      break;
    case K_CTRLL:  keymap[i] = PGKEY_LCTRL;  break;
    case K_CTRLR:  keymap[i] = PGKEY_RCTRL;  break;
    case K_ALT:    keymap[i] = PGKEY_LALT;   break;
    case K_ALTGR:  keymap[i] = PGKEY_RALT;   break;

    case K_INSERT: keymap[i] = PGKEY_INSERT;   break;
    case K_REMOVE: keymap[i] = PGKEY_DELETE;   break;
    case K_PGUP:   keymap[i] = PGKEY_PAGEUP;   break;
    case K_PGDN:   keymap[i] = PGKEY_PAGEDOWN; break;
    case K_FIND:   keymap[i] = PGKEY_HOME;     break;
    case K_SELECT: keymap[i] = PGKEY_END;      break;

    case K_NUM:  keymap[i] = PGKEY_NUMLOCK;   break;
    case K_CAPS: keymap[i] = PGKEY_CAPSLOCK;  break;

    case K_F13:   keymap[i] = PGKEY_PRINT;     break;
    case K_HOLD:  keymap[i] = PGKEY_SCROLLOCK; break;
    case K_PAUSE: keymap[i] = PGKEY_PAUSE;     break;

    case 127: keymap[i] = PGKEY_BACKSPACE; break;
	     
    default: break;
    }
  }
}

static struct keysym *TranslateKey(int scancode, struct keysym *keysym) {
  int map;
  int modstate;
  
  /* Set the keysym information */
  keysym->scancode = scancode;
  keysym->sym = keymap[scancode];
  keysym->mod = 0;
  keysym->unicode = 0;

  /* get the UNICODE value for the key */
  modstate = kbd_modstate;
  map = 0;
  if ( modstate & PGMOD_SHIFT ) {
    map |= (1<<KG_SHIFT);
  }
  if ( modstate & PGMOD_CTRL ) {
    map |= (1<<KG_CTRL);
  }
  if ( modstate & PGMOD_ALT ) {
    map |= (1<<KG_ALT);
  }
  if ( modstate & PGMOD_MODE ) {
    map |= (1<<KG_ALTGR);
  }
  if ( KTYP(vga_keymap[map][scancode]) == KT_LETTER ) {
    if ( modstate & PGMOD_CAPS ) {
      map ^= (1<<KG_SHIFT);
    }
  }
  if ( KTYP(vga_keymap[map][scancode]) == KT_PAD ) {
    if ( modstate & PGMOD_NUM ) {
      keysym->unicode=KVAL(vga_keymap[map][scancode]);
    }
  } else {
    keysym->unicode = KVAL(vga_keymap[map][scancode]);
  }

  return(keysym);
}

int rawttykb_fd_activate(int fd) {
  if (fd != keyboard_fd)
    return 0;
  handle_keyboard();
  return 1;
}

void rawttykb_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if (keyboard_fd > 0) {
    if ((*n)<(keyboard_fd+1))
      *n = keyboard_fd+1;
    FD_SET(keyboard_fd,readfds);
  }
}

void rawttykb_close(void) {
  rawttykb_LeaveGraphicsMode();
  rawttykb_CloseKeyboard();
}

g_error rawttykb_init(void) {
  if (rawttykb_OpenKeyboard() < 0 || rawttykb_EnterGraphicsMode() < 0)
    return mkerror(PG_ERRT_IO, 73);   /* Error initializing keyboard */
  rawttykb_InitOSKeymap();
  return success;
}

g_error rawttykb_regfunc(struct inlib *i) {
  i->init = &rawttykb_init;
  i->close = &rawttykb_close;
  i->fd_init = &rawttykb_fd_init;
  i->fd_activate = &rawttykb_fd_activate;
  return success;
}

/* The End */
