/* $Id: errortext.c,v 1.6 2000/09/04 05:00:19 micahjd Exp $
 *
 * errortext.c - optional error message strings
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <pgserver/g_error.h>

#ifdef TINY_MESSAGES
/* Eek! Cryptic, but saves lots of space */
const char *errortext(g_error e) {
  static char err[11];
  sprintf(err,"Err 0x%04X",e);
  return err;
}
#else 

static const char *errors[];

const char *errortext(g_error e) {
  return errors[(e & 0xFF)-1];
}

static const char *errors[] = {
  /* 1  */  "The parameter of a bitmap, WP_SIDE, is not right",
  /* 2  */  "The attempt at alignment (in your bitmap) yields a munged WP_ALIGN",
  /* 3  */  "What LGOP is that in your bitmap? I suppose I will have to guess.",
  /* 4  */  "Your WP_BITMAP, a droplet in the mist, has escaped its handle",
  /* 5  */  "WP_BITMASK is invalid. No transparency for you!",
  /* 6  */  "Invalid property for bitmap widget. I shall complain about it!",
  /* 7  */  "Indicator with negative size not render well. Invert space-time and try again",
  /* 8  */  "Indicator was lost; invalid side",
  /* 9  */  "What strange property you specify for indicator; nowhere to be found",
  /* 10 */  "Invalid WP_SIDE for a label",
  /* 11 */  "Nice try... Bad ALIGN value for label",
  /* 12 */  "Your label font is nowhere to be seen...",
  /* 13 */  "Strings, labels, everywhere. Your handle not ever found.",
  /* 14 */  "Your client lib newer that I: label property not found",
  /* 15 */  "Invalid WP_SIDE of toolbar.  Where shall I put it?",
  /* 16 */  "Toolbars do not have many properties.  Do not invent new ones without telling me.",
  /* 17 */  "In binding the scrollbar, you have misplaced the handle: cannot lift widget",
  /* 18 */  "The widget you wish to bind, has no WP_SCROLL",
  /* 19 */  "The property for scrollbar is invalid. Abort, retry, fail?",
  /* 20 */  "The widget you seek to create cannot exist",
  /* 21 */  "A mess of pointers; something must go wrong. widget has no in and out pointers",
  /* 22 */  "Derive constant not understood. Do not drink and derive.",
  /* 23 */  "This widget is stubborn and antisocial",
  /* 24 */  "The pointer to memory has nowhere to point",
  /* 25 */  "Insufficient... um... what's that word?",
  /* 26 */  "You step in the stream, but the water has moved on; This handle is not here",
  /* 27 */  "Access Denied! No handle for you!",
  /* 28 */  "The handle you seek has been found, but it is not what you are looking for",
  /* 29 */  "The handle you try to bequeath is invalid. Your heirs are disappointed",
  /* 30 */  "The application type you need does not exist.",
  /* 31 */  "Bad WP_SIDE for button.",
  /* 32 */  "Button can't be aligned with that",
  /* 33 */  "The bitmap you specify for a button can not be found",
  /* 34 */  "Button bitmask is missing!",
  /* 35 */  "Button font can't be found",
  /* 36 */  "Doh! Bad string handle for button!",
  /* 37 */  "A nonexistant button property does not exist",
  /* 38 */  "Yet another error; Bad WP_SIDE for a panel",
  /* 39 */  "Invalid property for the panel",
  /* 40 */  "Ha! Popups have no properties!",
  /* 41 */  "Bad side parameter for a box",
  /* 42 */  "Invalid property for a box",
  /* 43 */  "Invalid side property for the field",
  /* 44 */  "Bad font for field",
  /* 45 */  "Invalid field property",
  /* 46 */  "Error initializing video",
  /* 47 */  "Error setting video mode",
  /* 48 */  "Invalid PNM bitmap",
  /* 49 */  "Error in WSAStartup???",
  /* 50 */  "Error in socket???",
  /* 51 */  "Something's wrong with setsockopt()",
  /* 52 */  "Error in bind() - Is there already a PicoGUI server running here?",
  /* 53 */  "Error in listen() Hmm...",
  /* 54 */  "Argh! Can't set up a signal handler",
  /* 55 */  "Unable to execute the session manager. Check the command line",
  /* 56 */  "Memory leak detected on exit.  Fire up gdb and call a plumber",
  /* 57 */  "Request data structure is too small. It's all the client lib's fault!",
  /* 58 */  "mkwidget can't create this widget... Try something else",
  /* 59 */  "<gasp>... You tried to derive a widget from a NULL parent!",
  /* 60 */  "Stay inside your root widget",
  /* 61 */  "XBM data is too small",
  /* 62 */  "An undefined request (just recieved); Nothing to do-- but give an error",
  /* 63 */  "Incomplete request header found in batch",
  /* 64 */  "Incomplete request data in batch",
  /* 65 */  "Can't get exclusive keyboard access. Another app is already being selfish",
  /* 66 */  "Can't get exclusive pointing device access, another app is using it",
  /* 67 */  "How can you expect to give up the keyboard when you are not the owner?",
  /* 68 */  "You don't own that pointing device!",
  /* 69 */  "Can't find a connection buffer for the client! Network code must be haunted",
  /* 70 */  "Context underflow",
  /* 71 */  "Can't create the input thread!",
  /* 72 */  "This driver doesn't support changing video modes",
  /* 73 */  "Error initializing keyboard",
  /* 74 */  "Error initializing mouse",
  /* 75 */  "Nonexistant input driver",
  /* 76 */  "This input driver is already loaded",
  /* 77 */  "Nonexistant video driver",
  /* 78 */  "All installed video drivers failed!",
};

#endif /* TINY_MESSAGES */

/* The End */




