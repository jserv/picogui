/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 *
 *  Seriously modified by Fredrik Hübinette <hubbe@hubbe.net>
 */

/*
 * x.c - functions to deal with X display.
 */

#include <sys/types.h>
#include <unistd.h>
#include "x2pgui.h"
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

static s16 ODD_keymap[256];
static s16 MISC_keymap[256];

Display *dpy;
static Window topLevel;
static int topLevelWidth, topLevelHeight;

int resurface;

static Atom wmProtocols, wmDeleteWindow, wmState;
static Bool modifierPressed[256];

static Bool HandleTopLevelEvent(XEvent *ev);
static Bool HandleRootEvent(XEvent *ev);
static int displayWidth, displayHeight;
static int grabbed;
Cursor  grabCursor;

enum edge_enum edge = EDGE_EAST;
int edge_width=1;
int emulate_wheel=1;

/*
 * This variable is true (1) if the mouse is on the same screen as the one
 * we're monitoring, or if there is only one screen on the X server.
 * - GRM
 */
static Bool mouseOnScreen;

void SendKeyInput(int trigger, int key, int mods) {
  static union pg_client_trigger trig;
  trig.content.type = trigger;
  trig.content.u.kbd.key = key;
  trig.content.u.kbd.mods = mods;
  pgInFilterSend(&trig);
  pgFlushRequests();
}

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


/*
 * CreateXWindow.
 */

Bool CreateXWindow(void)
{
    XSetWindowAttributes attr;
    XEvent ev;
    char defaultGeometry[256];
    XSizeHints wmHints;
    XGCValues gcv;
    int i;

    Pixmap    nullPixmap;
    XColor    dummyColor;

    if (!(dpy = XOpenDisplay(displayname))) {
	fprintf(stderr,"%s: unable to open display %s\n",
		programName, XDisplayName(displayname));
	return False;
    }

    x11input_init_keymap();

    for (i = 0; i < 256; i++)
	modifierPressed[i] = False;

    /* Try to work out the geometry of the top-level window */

    displayWidth = WidthOfScreen(DefaultScreenOfDisplay(dpy));
    displayHeight = HeightOfScreen(DefaultScreenOfDisplay(dpy));

    topLevelWidth=edge_width;
    topLevelHeight=edge_width;
    wmHints.x=0;
    wmHints.y=0;

    switch(edge)
    {
      case EDGE_EAST: wmHints.x=displayWidth-edge_width;
      case EDGE_WEST: topLevelHeight=displayHeight;
	break;

      case EDGE_SOUTH: wmHints.y=displayHeight-edge_width;
      case EDGE_NORTH: topLevelWidth=displayWidth;
	break;
    }

    wmHints.flags = PMaxSize | PMinSize |PPosition |PBaseSize;

    wmHints.min_width = topLevelWidth;
    wmHints.min_height = topLevelHeight;

    wmHints.max_width = topLevelWidth;
    wmHints.max_height = topLevelHeight;

    wmHints.base_width = topLevelWidth;
    wmHints.base_height = topLevelHeight;

    sprintf(defaultGeometry, "%dx%d+%d+%d",
	    topLevelWidth, topLevelHeight,
	    wmHints.x, wmHints.y);

    XWMGeometry(dpy, DefaultScreen(dpy), geometry, defaultGeometry, 0,
		&wmHints, &wmHints.x, &wmHints.y,
		&topLevelWidth, &topLevelHeight, &wmHints.win_gravity);

    /* Create the top-level window */

    attr.border_pixel = 0; /* needed to allow 8-bit cmap on 24-bit display -
			      otherwise we get a Match error! */
    attr.background_pixel = BlackPixelOfScreen(DefaultScreenOfDisplay(dpy));
    attr.event_mask = ( LeaveWindowMask|
			StructureNotifyMask|
			ButtonPressMask|
			ButtonReleaseMask|
			PointerMotionMask|
			KeyPressMask|
			KeyReleaseMask|
			EnterWindowMask|
			(resurface?VisibilityChangeMask:0) );
      
    attr.override_redirect=True;

    topLevel = XCreateWindow(dpy, DefaultRootWindow(dpy), wmHints.x, wmHints.y,
			     topLevelWidth, topLevelHeight, 0, CopyFromParent,
			     InputOutput, CopyFromParent,
			     (CWBorderPixel|
			      CWEventMask|
			      CWOverrideRedirect|
			      CWBackPixel),
			     &attr);

    wmHints.flags |= USPosition; /* try to force WM to place window */
    XSetWMNormalHints(dpy, topLevel, &wmHints);

    wmProtocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wmDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, topLevel, &wmDeleteWindow, 1);

    //    XStoreName(dpy, topLevel, desktopName);


    XMapRaised(dpy, topLevel);

    /*
     * For multi-screen setups, we need to know if the mouse is on the 
     * screen.
     * - GRM
     */
    if (ScreenCount(dpy) > 1) {
      Window root, child;
      int root_x, root_y;
      int win_x, win_y;
      unsigned int keys_buttons;

      XSelectInput(dpy, DefaultRootWindow(dpy), PropertyChangeMask |
                   EnterWindowMask | LeaveWindowMask);
        /* Cut buffer happens on screen 0 only. */
      if (DefaultRootWindow(dpy) != RootWindow(dpy, 0)) {
          XSelectInput(dpy, RootWindow(dpy, 0), PropertyChangeMask);
      }
      XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &child,
                    &root_x, &root_y, &win_x, &win_y, &keys_buttons);
      mouseOnScreen = root == DefaultRootWindow(dpy);
    } else {
      XSelectInput(dpy, DefaultRootWindow(dpy), PropertyChangeMask);
      mouseOnScreen = 1;
    }

    nullPixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), 1, 1, 1);
    grabCursor = 
      XCreatePixmapCursor(dpy, nullPixmap, nullPixmap,
			  &dummyColor, &dummyColor, 0, 0);
    
    return True;
}



/*
 * ShutdownX.
 */

void
ShutdownX()
{
    XCloseDisplay(dpy);
}


/*
 * HandleXEvents.
 */

Bool
HandleXEvents()
{
    XEvent ev;

    while (1) {
      XMaskEvent(dpy, ~0, &ev);
      if (ev.xany.window == topLevel) {

	    if (!HandleTopLevelEvent(&ev))
		return False;

	} else if (ev.xany.window == DefaultRootWindow(dpy) ||
                   ev.xany.window == RootWindow(dpy, 0)) {

	    if (!HandleRootEvent(&ev))
		return False;

	} else if (ev.type == MappingNotify) {

	    XRefreshKeyboardMapping(&ev.xmapping);
	}
    }

    return True;
}

#define EW (edge == EDGE_EAST || edge==EDGE_WEST)
#define NS (edge == EDGE_NORTH || edge==EDGE_SOUTH)
#define ES (edge == EDGE_EAST || edge==EDGE_SOUTH)
#define NS (edge == EDGE_NORTH || edge==EDGE_SOUTH)

static int enter_translate(int isedge, int width, int pos)
{
  if(!isedge) return pos;
  if(ES) return 0;
  return width-1;
}

static int leave_translate(int isedge, int width, int pos)
{
  if(!isedge) return pos;
  if(ES) return width-edge_width;
  return 0;
}


#define EDGE(X) ((X)?edge_width:0)

#define SCALEX(X) \
( ((X) - EDGE(edge==EDGE_WEST))*mi.lxres/(displayWidth - EDGE(EW)) )

#define SCALEY(Y) \
( ((Y)-EDGE(edge==EDGE_NORTH))*mi.lyres/(displayHeight-EDGE(NS)) )

/*
 * HandleTopLevelEvent.
 */

static void shortsleep(int usec)
{
  struct timeval timeout;
  timeout.tv_sec=0;
  timeout.tv_usec=usec;
  select(0,0,0,0,&timeout);
}

static Bool
HandleTopLevelEvent(XEvent *ev)
{
  Bool grab;
  int i;
  int x, y;
  s16 mod,sym,chr;

  int buttonMask;
  KeySym ks;
  
  switch (ev->type) {
    case VisibilityNotify:
      /*
       * I avoid resurfacing when the window becomes fully obscured, because
       * that *probably* means that somebody is running xlock.
       * Please tell me if you have a problem with this.
       * - Hubbe
       */
      if (ev->xvisibility.state == VisibilityPartiallyObscured && resurface)
      {
	static long last_resurface=0;
	long t=time(0);

	if(t == last_resurface)
	  shortsleep(5000);

	last_resurface=t;
	  
	XRaiseWindow(dpy, topLevel);
      }
      return 1;

      /*
       * We don't need to worry about multi-screen here; that's handled
       * below, as a root event.
       * - GRM
       */
    case EnterNotify:
      if(!grabbed && ev->xcrossing.mode==NotifyNormal)
      {
	XGrabPointer(dpy, topLevel, True,
		     PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		     GrabModeAsync, GrabModeAsync,
		     None, grabCursor, CurrentTime);
	XGrabKeyboard(dpy, topLevel, True, 
		      GrabModeAsync, GrabModeAsync,
		      CurrentTime);
	XFlush(dpy);
	XWarpPointer(dpy,None, DefaultRootWindow(dpy),0,0,0,0,
		     enter_translate(EW,displayWidth ,ev->xcrossing.x_root),
		     enter_translate(NS,displayHeight,ev->xcrossing.y_root));
        mouseOnScreen = 1;

	SendPointerEvent(SCALEX(ev->xcrossing.x_root),
			 SCALEY(ev->xcrossing.y_root),
			 (ev->xmotion.state & 0x1f00) >> 8);
	grabbed=1;
      }
      return 1;
      
    case MotionNotify:
      
      if(grabbed)
      {
	int i, d;
        int x, y;
        Window warpWindow;
        while (XCheckTypedWindowEvent(dpy, topLevel, MotionNotify, ev))
	  ;	/* discard all queued motion notify events */
	
	i=SendPointerEvent(SCALEX(ev->xmotion.x_root),
			   SCALEY(ev->xmotion.y_root),
			   (ev->xmotion.state & 0x1f00) >> 8);

	if(ev->xmotion.state & 0x1f00) return 1;

          /*
           * Things get complicated for multi-screen X servers,
           * particularly if the PC screen is "inserted" between two
           * X screens.
           * - GRM
           */
        /* First, check for normal edges (applies to single screen or
         * screen edge that doesn't switch screens)
         */
        if (ev->xmotion.same_screen) {
          x = ev->xmotion.x_root;
          y = ev->xmotion.y_root;
          warpWindow = DefaultRootWindow(dpy);
	  switch(edge)
	  {
	    case EDGE_NORTH: 
              d=ev->xmotion.y_root == displayHeight-1;
              y = edge_width;
              break;
	    case EDGE_SOUTH:
              d=ev->xmotion.y_root == 0;
              y = displayHeight - edge_width -1;
              break;
	    case EDGE_EAST:
              d=ev->xmotion.x_root == 0;
              x = displayWidth -  edge_width -1;
              break;
	    case EDGE_WEST:
              d=ev->xmotion.x_root == displayWidth-1;
              x = edge_width;
              break;
	  }
        } else {
            /*
             * Different screen. Depending on where the pointer ended up,
             * we warp to either our default screen or the new screen.
             * 
             * If the pointer is "near" the edge we're watching, then the
             * user moved the pointer off the _opposite_ side of the PC
             * screen, and the pointer should reappear on the original screen.
             * 
             * If not, then the pointer was moved off some other edge - and
             * we just release the pointer.
             * 
             * In either case, the pointer's location - relative to the
             * screen - is not moved.
             * 
             * - GRM
             */
          warpWindow = ev->xmotion.root;
          x = ev->xmotion.x_root;
          y = ev->xmotion.y_root;
	  switch(edge)
	  {
	    case EDGE_NORTH:
              d=ev->xmotion.y_root < displayHeight / 2;
              break;
	    case EDGE_SOUTH:
              d=ev->xmotion.y_root > displayHeight / 2;
              break;
	    case EDGE_EAST:
              d=ev->xmotion.x_root > displayWidth / 2;
              break;
	    case EDGE_WEST:
              d=ev->xmotion.x_root < displayWidth / 2;
              break;
	  }
          if (d) {
            warpWindow = DefaultRootWindow(dpy);
          }
          d = 1;
              
        }
          
	if(d)
	{
	  HidePointer();
	  XWarpPointer(dpy,None, warpWindow, 0,0,0,0, x, y);
	  XUngrabKeyboard(dpy, CurrentTime);
	  XUngrabPointer(dpy, CurrentTime);
          mouseOnScreen = warpWindow == DefaultRootWindow(dpy);
	  XFlush(dpy);

	  /*	  
	  for (i = 255; i >= 0; i--) {
	    if (modifierPressed[i]) {
	      if (!SendKeyEvent(XKeycodeToKeysym(dpy, i, 0), False))
		return False;
	      modifierPressed[i]=False;
	    }
	  }
	  */
	  
	  grabbed=0;
	}
	return i;
      }
      return 1;
      
    case ButtonPress:
    case ButtonRelease:
#if 0
      if (ev->xbutton.button > 3 && emulate_wheel)
      {
	if (ev->xbutton.button == 4)
	  ks = XK_Up;
	else
	  ks = XK_Down;

	if (ev->type == ButtonPress)
	  SendKeyEvent(ks, 1); /* keydown */
	else
	  SendKeyEvent(ks, 0); /* keyup */
	break;
      }
#endif

      if (ev->type == ButtonPress) {
	buttonMask = (((ev->xbutton.state & 0x1f00) >> 8) |
		      (1 << (ev->xbutton.button - 1)));
      } else {
	buttonMask = (((ev->xbutton.state & 0x1f00) >> 8) &
		      ~(1 << (ev->xbutton.button - 1)));
      }

      return SendPointerEvent(SCALEX(ev->xbutton.x_root),
			      SCALEY(ev->xbutton.y_root),
			      buttonMask);

      /* FIXME: This code was ripped from pgserver's x11input driver,
       * so it probably has the same bugs.
       */
    case KeyPress:
      x11_translate_key(dpy, &ev->xkey, ev->xkey.keycode, &sym, &mod, &chr);
      if (sym)
	SendKeyInput(PG_TRIGGER_KEYDOWN,sym,mod);
      if (chr)
	SendKeyInput(PG_TRIGGER_CHAR,chr,mod);
      pgFlushRequests();
      break;

    case KeyRelease:
      x11_translate_key(dpy, &ev->xkey, ev->xkey.keycode, &sym, &mod, &chr);
      if (x11_key_repeat(dpy, ev)) {
	if (chr)
	  SendKeyInput(PG_TRIGGER_CHAR,chr,mod);
      }
      else {
	if (sym)
	  SendKeyInput(PG_TRIGGER_KEYUP,sym,mod);
      }
      pgFlushRequests();
      break;
      
    case ClientMessage:
      if ((ev->xclient.message_type == wmProtocols) &&
	  (ev->xclient.data.l[0] == wmDeleteWindow))
	{
	    ShutdownX();
	    exit(0);
	}
	break;
    }

    return True;
}



/*
 *RootEvent.
 */

static Bool
HandleRootEvent(XEvent *ev)
{
    char *str;
    int len;

    Bool nowOnScreen;
    Bool grab;
    
    int x, y;

    switch (ev->type) {

    case EnterNotify:
    case LeaveNotify:
      /*
       * Ignore the event if it's due to leaving our window. This will
       * be after an ungrab.
       */
      if (ev->xcrossing.subwindow == topLevel &&
          !ev->xcrossing.same_screen) {
          break;
      }

      grab = 0;
      if(!grabbed) {
          nowOnScreen =  ev->xcrossing.same_screen;
          if (mouseOnScreen != nowOnScreen) {
              /*
               * Mouse has left, or entered, the screen. We must grab if
               * the mouse is now near the edge we're watching.
               * 
               * If we do grab, the mouse coordinates are left alone.
               * The test, however, depends on whether the mouse entered
               * or left the screen.
               * 
               * - GRM
               */
              x = ev->xcrossing.x_root;
              y = ev->xcrossing.x_root;
              if (!nowOnScreen) {
                  x = enter_translate(EW,displayWidth,ev->xcrossing.x_root);
                  y = enter_translate(NS,displayHeight,ev->xcrossing.y_root);
              }
	      switch(edge)
	      {
	        case EDGE_NORTH:
                  grab=y < displayHeight / 2;
                  break;
	        case EDGE_SOUTH:
                  grab=y > displayHeight / 2;
                  break;
	        case EDGE_EAST:
                  grab=x > displayWidth / 2;
                  break;
	        case EDGE_WEST:
                  grab=x < displayWidth / 2;
                  break;
              }
          }
          mouseOnScreen = nowOnScreen;
      }

        /*
         * Do not grab if this is the result of an ungrab
         * or grab (caused by us, usually).
         * 
         * - GRM
         */
      if(grab && ev->xcrossing.mode == NotifyNormal)
      {
	XWarpPointer(dpy,None, DefaultRootWindow(dpy),0,0,0,0, 
                     ev->xcrossing.x_root, ev->xcrossing.y_root);
	XGrabPointer(dpy, topLevel, True,
		     PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		     GrabModeAsync, GrabModeAsync,
		     None, grabCursor, CurrentTime);
	XGrabKeyboard(dpy, topLevel, True, 
		      GrabModeAsync, GrabModeAsync,
		      CurrentTime);
	XFlush(dpy);
	     

	SendPointerEvent(SCALEX(ev->xcrossing.x_root), SCALEY(ev->xcrossing.y_root),
			 (ev->xcrossing.state & 0x1f00) >> 8);
	grabbed=1;
        mouseOnScreen = 1;
      }
      break;

    case PropertyNotify:
	if (ev->xproperty.atom == XA_CUT_BUFFER0) {

	    str = XFetchBytes(dpy, &len);
	    if (str) {
	      /*
		if (!SendClientCutText(str, len))
		    return False;
	      */
		XFree(str);
	    }
	}
	break;
    }

    return True;
}


/*
 * AllXEventsPredicate is needed to make XCheckIfEvent return all events.
 */

Bool
AllXEventsPredicate(Display *dpy, XEvent *ev, char *arg)
{
    return True;
}



