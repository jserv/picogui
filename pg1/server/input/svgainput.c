/* $Id$
 *
 * svgainput.h - input driver for SVGAlib
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
#include <picogui/constants.h>

#include <vga.h>
#include <vgakeyboard.h>
#include <linux/kd.h>
#include <linux/keyboard.h>

/* Yes, this is a hack, but it's more efficient this way.
   Note to the authors of SVGAlib: why hide these vars?
   Oh well, at least I read some of the SVGAlib source code ;-)
*/
extern int __svgalib_mouse_fd;
extern int __svgalib_kbd_fd;
extern void (*__svgalib_mouse_eventhandler)(int, int, int, int, int, int, int);

void (*default_svgalib_mousehandler)(int, int, int, int, int, int, int);

struct cursor *svga_cursor;

/******************************************** Keyboard handler */
/* Where indicated, code has been used from SDL (www.libsdl.org) */

/* Keyboard maps (from SDL_svgaevents.c -- see below) */
#define NUM_VGAKEYMAPS	(1<<KG_CAPSSHIFT)
static short svgainput_vga_keymap[NUM_VGAKEYMAPS][NR_KEYS];
void svgainput_initkeymaps(void);
/* NOTE: this table maps SCANCODE_* constants to PGKEY_*
   constants! Rebuild if either are changed!  */
short svgainput_keymap[128] = {
  0x0000,0x001B,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,
  0x0037,0x0038,0x0039,0x0030,0x002D,0x003D,0x0008,0x0009,
  0x0071,0x0077,0x0065,0x0072,0x0074,0x0079,0x0075,0x0069,
  0x006F,0x0070,0x005B,0x005D,0x000D,0x0132,0x0061,0x0073,
  0x0064,0x0066,0x0067,0x0068,0x006A,0x006B,0x006C,0x003B,
  0x0027,0x0060,0x0130,0x005C,0x007A,0x0078,0x0063,0x0076,
  0x0062,0x006E,0x006D,0x002C,0x002E,0x002F,0x012F,0x010C,
  0x0134,0x0020,0x012D,0x011A,0x011B,0x011C,0x011D,0x011E,
  0x011F,0x0120,0x0121,0x0122,0x0123,0x012C,0x012E,0x0107,
  0x0108,0x0109,0x010D,0x0104,0x0105,0x0106,0x010E,0x0101,
  0x0102,0x0103,0x0100,0x010A,0x0000,0x0000,0x003C,0x0124,
  0x0125,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x010F,0x0131,0x010B,0x013C,0x0133,0x013E,0x0116,0x0111,
  0x0118,0x0114,0x0113,0x0117,0x0112,0x0119,0x0115,0x007F,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0013,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0137,0x0138,0x013F
};

/* Current modifier state (in PGKEY values) */
int svgainput_mod;

/* Nonzero means the keyboard mouse control (via arrow keys) is on */
unsigned char kbdmouse;

/* Initialize the keymaps. Almost all of this is from
   SDL_svgaevents.c in SDL.
   Duplicates the kernel's keymapping code, so it's kinda
   yucky but it's the only way I know of to get both raw
   and mapped keys at the same time like PicoGUI requires.
*/
void svgainput_initkeymaps(void) {
  struct kbentry entry;
  int map, i;
  
  /* Load all the keysym mappings */
  for ( map=0; map<NUM_VGAKEYMAPS; ++map ) {
    memset(svgainput_vga_keymap[map], 0, NR_KEYS*sizeof(unsigned short));
    for ( i=0; i<NR_KEYS; ++i ) {
      entry.kb_table = map;
      entry.kb_index = i;
      if ( ioctl(__svgalib_kbd_fd, KDGKBENT, &entry) == 0 ) {
	/* The "Enter" key is a special case */
	if ( entry.kb_value == K_ENTER ) {
	  entry.kb_value = K(KT_ASCII,13);
	}
	/* The delete key */
	else if ( entry.kb_value == K_REMOVE ) {
	  entry.kb_value = K(KT_ASCII,127);
	}
	/* Backspace */
	else if ( entry.kb_value == 127) {
	   entry.kb_value = K(KT_ASCII,8);
	}
	/* Handle numpad specially as well */
	else if ( KTYP(entry.kb_value) == KT_PAD ) {
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
	    svgainput_vga_keymap[map][i]=entry.kb_value;
	    svgainput_vga_keymap[map][i]+= '0';
	    break;
	  case K_PPLUS:
	    svgainput_vga_keymap[map][i]=K(KT_ASCII,'+');
	    break;
	  case K_PMINUS:
	    svgainput_vga_keymap[map][i]=K(KT_ASCII,'-');
	    break;
	  case K_PSTAR:
	    svgainput_vga_keymap[map][i]=K(KT_ASCII,'*');
	    break;
	  case K_PSLASH:
	    svgainput_vga_keymap[map][i]=K(KT_ASCII,'/');
	    break;
	  case K_PENTER:
	    svgainput_vga_keymap[map][i]=K(KT_ASCII,'\r');
	    break;
	  case K_PCOMMA:
	    svgainput_vga_keymap[map][i]=K(KT_ASCII,',');
	    break;
	  case K_PDOT:
	    svgainput_vga_keymap[map][i]=K(KT_ASCII,'.');
	    break;
	  default:
	    break;
	  }
	}
	/* Do the normal key translation */
	if ( (KTYP(entry.kb_value) == KT_LATIN) ||
	     (KTYP(entry.kb_value) == KT_ASCII) ||
	     (KTYP(entry.kb_value) == KT_LETTER) ) {
	  svgainput_vga_keymap[map][i] = entry.kb_value;
	}
      }
    }
  }
}

/* Keyboard handler called by SVGAlib */
void svgainput_kbdhandler(int scancode,int press) {
  short x=0,led=0;
  static short previouskey = 0;

  /******* Handle modifiers */

  switch (scancode) {
    /* locks */

  case SCANCODE_NUMLOCK:
    if (!press) break;
    svgainput_mod ^= PGMOD_NUM;
    goto updatelights;
  case SCANCODE_SCROLLLOCK:
    if (!press) break;
    kbdmouse ^= 1;
    goto updatelights;
   case SCANCODE_CAPSLOCK:
    if (!press) break;
    svgainput_mod ^= PGMOD_CAPS;
  updatelights:

    /* Update the keyboard LEDs */
    if (svgainput_mod & PGMOD_CAPS)  led |= LED_CAP;
    if (svgainput_mod & PGMOD_NUM)   led |= LED_NUM;
    if (kbdmouse)                    led |= LED_SCR;
    ioctl(__svgalib_kbd_fd,KDSETLED,led);
    break;
    
    /* normal modifiers */
  case SCANCODE_LEFTSHIFT:    x = PGMOD_LSHIFT;  break;
  case SCANCODE_RIGHTSHIFT:   x = PGMOD_RSHIFT;  break;
  case SCANCODE_LEFTCONTROL:  x = PGMOD_LCTRL;   break;
  case SCANCODE_RIGHTCONTROL: x = PGMOD_RCTRL;   break;
  case SCANCODE_LEFTALT:      x = PGMOD_LALT;    break;
  case SCANCODE_RIGHTALT:     x = PGMOD_RALT;    break;
  }
  if (press)
    svgainput_mod |= x;
  else
    svgainput_mod &= ~x;

  /******* Handle mouse keys */
  if (kbdmouse) {
     static char n=0,e=0,s=0,w=0,b=0;
     int scale = (svgainput_mod & PGMOD_SHIFT) ? 0 : 2;

          if (scancode == SCANCODE_CURSORBLOCKUP)    n = press;
     else if (scancode == SCANCODE_CURSORBLOCKRIGHT) e = press;
     else if (scancode == SCANCODE_CURSORBLOCKDOWN)  s = press;
     else if (scancode == SCANCODE_CURSORBLOCKLEFT)  w = press;
     else if (scancode == SCANCODE_INSERT || scancode == SCANCODE_REMOVE) {
        if (press) {
	   if (previouskey==scancode) return;
	   previouskey = scancode;
	   b |= scancode == SCANCODE_REMOVE ? 2 : 1;
	}
	else {
	  previouskey = 0;
	  b &= scancode == SCANCODE_REMOVE ? ~2 : ~1;
	}
	infilter_send_pointing(press ? PG_TRIGGER_DOWN : PG_TRIGGER_UP,
			       svga_cursor->x,svga_cursor->y,b,svga_cursor);
	return;
     }
     else
       goto nomousekey;

	  infilter_send_pointing(PG_TRIGGER_MOVE,
				 svga_cursor->x+((e-w)<<scale),
				 svga_cursor->y+((s-n)<<scale),b,svga_cursor);
     mouse_setposition(svga_cursor->x,svga_cursor->y);
     return;
  }
   nomousekey:
   
  /******* Handle ascii character events */

  if (press) {
    short map=0,c=0;

    /* Convert out modifier flags to a map number 
     * (this code adapted from SDL_svgaevents.c)   */

    if (svgainput_mod & PGMOD_SHIFT) map |= 1 << KG_SHIFT;
    if (svgainput_mod & PGMOD_CTRL)  map |= 1 << KG_CTRL;
    if (svgainput_mod & PGMOD_ALT)   map |= 1 << KG_ALT;
    if (svgainput_mod & PGMOD_MODE)  map |= 1 << KG_ALTGR;
    if ((KTYP(svgainput_vga_keymap[map][scancode]) == KT_LETTER)
	&&(svgainput_mod&PGMOD_CAPS)) map ^= 1 << KG_SHIFT;

    /* Lookup the ascii/unicode value in the tables */
    
    if (KTYP(svgainput_vga_keymap[map][scancode]) == KT_PAD) {
      if (svgainput_mod & PGMOD_NUM)
	c = KVAL(svgainput_vga_keymap[map][scancode]);
    }
    else
      c = KVAL(svgainput_vga_keymap[map][scancode]);


#ifdef DEBUG_EVENT
    //    guru("Translated key: %c (%d) (mods: %d)",c,c,svgainput_mod);
#endif

    if (c)
      infilter_send_key(PG_TRIGGER_CHAR,c,svgainput_mod);
  }

  /******* Handle raw key press/release events */
  
  /* Dispatch to the rest of PicoGUI */
  infilter_send_key(press ? PG_TRIGGER_KEYDOWN : PG_TRIGGER_KEYUP,svgainput_keymap[scancode],svgainput_mod);
}

/******************************************** Mouse handler */

void svgainput_mousehandler(int button,int dx,int dy,int dz,
			    int drx,int dry,int drz) {
  int x,y,trigger;
  static int prevbutton = 0;
  
  /* This is ugly, but better than the alternative... */
  (*default_svgalib_mousehandler)(button,dx,dy,dz,drx,dry,drz);
  x = mouse_getx();
  y = mouse_gety();

#ifdef DEBUG_EVENT
  /*
  guru("svgainput_mousehandler:\n"
       "button = %d\ndx = %d\ndy = %d\ndz = %d\ndrx = %d\ndry = %d\ndrz = %d"
       "\nx = %d\ny = %d",
       button,dx,dy,dz,drx,dry,drx,x,y);
  */
#endif

  /* So, what just happened? */
  if (button & (~prevbutton))
    trigger = PG_TRIGGER_DOWN;
  else if (prevbutton & (~button))
    trigger = PG_TRIGGER_UP;
  else
    trigger = PG_TRIGGER_MOVE;      
  prevbutton = button;

  /* Dispatch to PicoGUI */
  infilter_send_pointing(trigger,x,y,
			 ((button>>2)&1) ||
			 ((button<<2)&4) ||
			 (button&2),svga_cursor);
}

/******************************************** Implementations */

/* Enable keyboard and mouse support */
g_error svgainput_init(void) {
  if (keyboard_init()==-1)
    return mkerror(PG_ERRT_IO,73);
  vga_setmousesupport(1);
  svgainput_initkeymaps();
  keyboard_seteventhandler(&svgainput_kbdhandler);
  default_svgalib_mousehandler = __svgalib_mouse_eventhandler;
  mouse_seteventhandler(&svgainput_mousehandler);
  return cursor_new(&svga_cursor,NULL,-1);
}
 
void svgainput_close(void) {
  ioctl(__svgalib_kbd_fd,KDSETLED,0);   /* Turn off the lights */
  keyboard_close();
  pointer_free(-1,svga_cursor);
}

/* watch SVGAlib's file handles */
void svgainput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if ((*n) < (__svgalib_mouse_fd+1)) *n = __svgalib_mouse_fd+1;
  if ((*n) < (__svgalib_kbd_fd+1)) *n = __svgalib_kbd_fd+1;
  FD_SET(__svgalib_mouse_fd,readfds);
  FD_SET(__svgalib_kbd_fd,readfds);
}

/* Pass on any fd activity to SVGAlib's *_update() functions */
int svgainput_fd_activate(int fd) {
  if (fd==__svgalib_mouse_fd)
    mouse_update();
  else if (fd==__svgalib_kbd_fd)
    keyboard_update();
  else 
    return 0;
  return 1;
}

/******************************************** Driver registration */

g_error svgainput_regfunc(struct inlib *i) {
  i->init = &svgainput_init;
  i->close = &svgainput_close;
  i->fd_init = &svgainput_fd_init;
  i->fd_activate = &svgainput_fd_activate;
  return success;
}

/* The End */
