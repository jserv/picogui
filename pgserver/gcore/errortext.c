/* $Id: errortext.c,v 1.25 2001/06/28 04:24:56 micahjd Exp $
 *
 * errortext.c - optional error message strings
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
 * Brandon Smith <lottabs2@yahoo.com>
 * 
 * 
 */

#include <pgserver/common.h>

/************************ Numeric Errors */

/* If CONFIG_TEXT is not defined, no error
 * strings are stored- the user gets a nice
 * hexadecimal error code that is in most
 * cases not too much worse than the string :)
 */

#ifndef CONFIG_TEXT

/* Eek! Cryptic, but saves lots of space */
const char *errortext(g_error e) {
  static char err[11];
  sprintf(err,"Err 0x%04X",e);
  return err;
}

#else /* CONFIG_TEXT */

/************************ String errors */

static const char *errors[];

const char *errortext(g_error e) {
	return errors[(e & 0xFF)-1];
}

/* Builtin error table, can be overrided by translations */
static const char *errors[] = {
  /* 1  */  "The parameter of a bitmap, PG_WP_SIDE, is not right",
  /* 2  */  "Unknown side specified for widget",
  /* 3  */  "What LGOP is that in your bitmap? I suppose I will have to guess.",
  /* 4  */  "Your PG_WP_BITMAP, a droplet in the mist, has escaped its handle",
  /* 5  */  "PG_WP_BITMASK is invalid. No transparency for you!",
  /* 6  */  "Invalid widget property",
  /* 7  */  "Indicator with negative size not render well. Invert space-time and try again",
  /* 8  */  "Bitmap format not recognized by any active loaders",
  /* 9  */  "Cannot render to the display without registering exclusive access",
  /* 10 */  "Another application already has exclusive access to the display",
  /* 11 */  "Nice try... Bad ALIGN value for label",
  /* 12 */  "Your label font is nowhere to be seen...",
  /* 13 */  "Bad handle for widget text property",
  /* 14 */  NULL,
  /* 15 */  NULL,
  /* 16 */  NULL,
  /* 17 */  "In binding the scrollbar, you have misplaced the handle: cannot lift widget",
  /* 18 */  "The widget you wish to bind, has no PG_WP_SCROLL",
  /* 19 */  NULL,
  /* 20 */  "The widget you seek to create cannot exist",
  /* 21 */  "A mess of pointers; something must go wrong. widget has no in and out pointers",
  /* 22 */  "Derive constant not understood. Do not drink and derive.",
  /* 23 */  "Invalid widget in widget_set",
  /* 24 */  NULL,
  /* 25 */  "Insufficient... um... what's that word?",
  /* 26 */  "You step in the stream, but the water has moved on; This handle is not here",
  /* 27 */  "Access Denied! No handle for you!",
  /* 28 */  "The handle you seek has been found, but it is not what you are looking for",
  /* 29 */  "The handle you try to bequeath is invalid. Your heirs are disappointed",
  /* 30 */  "The application type you need does not exist.",
  /* 31 */  NULL,
  /* 32 */  "Button can't be aligned with that",
  /* 33 */  "The bitmap you specify for a button can not be found",
  /* 34 */  "Button bitmask is missing!",
  /* 35 */  "Button font can't be found",
  /* 36 */  "Doh! Bad string handle for button!",
  /* 37 */  NULL,
  /* 38 */  "Yet another error; Bad PG_WP_SIDE for a panel",
  /* 39 */  NULL,
  /* 40 */  NULL,
  /* 41 */  NULL,
  /* 42 */  NULL,
  /* 43 */  "Invalid side property for the field",
  /* 44 */  "Bad font for field",
  /* 45 */  NULL,
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
  /* 67 */  NULL,
  /* 68 */  NULL,
  /* 69 */  "Can't find a connection buffer for the client! Network code must be haunted",
  /* 70 */  "Context underflow",
  /* 71 */  "Can't create the input thread!",
  /* 72 */  NULL,
  /* 73 */  "Error initializing keyboard",
  /* 74 */  "Error initializing mouse",
  /* 75 */  "Nonexistant input driver",
  /* 76 */  "This input driver is already loaded",
  /* 77 */  "Nonexistant video driver",
  /* 78 */  "All installed video drivers failed!",
  /* 79 */  "Unknown PG_APPSPEC constant!",
  /* 80 */  "This is not a PicoGUI theme file (bad magic number)",
  /* 81 */  "Length mismatch in PicoGUI theme file (possible file truncation)",
  /* 82 */  "Bad checksum in PicoGUI theme file (probable file corruption)",
  /* 83 */  "Theme file does not have a header!",
  /* 84 */  "Unexpected EOF in theme file (bug in theme compiler?)",
  /* 85 */  "Theme heap overflow (bug in theme compiler?)",
  /* 86 */  "Out-of-range offset in theme file (bug in theme compiler?)",
  /* 87 */  "Unknown loader in theme (theme is newer than server?)",
  /* 88 */  "Stack underflow in fillstyle interpreter",
  /* 89 */  "Stack overflow in fillstyle interpreter",
  /* 90 */  "Local variable out of range in fillstyle",
  /* 91 */  "Fillstyle opcode parameter truncated",
  /* 92 */  "Invalid handle in handle_group()",
  /* 93 */  "Dereferenced string handle is null in getstring",
  /* 94 */  "Request packet too big; memory allocation failed",
  /* 95 */  "Error opening framebuffer device",
  /* 96 */  "Error mapping framebuffer device",
  /* 97 */  "Framebuffer ioctl error",
  /* 98 */  "Another application is already registered for system events",
  /* 99 */  "Application attempting to own unknown system resource",
  /* 100 */ "Theme is not word-aligned, use a newer version of themec",
  /* 101 */ "Unsupported color depth",
  /* 102 */ "Support for the terminal widget was not compiled in to this server",
  /* 103 */ "Support for the canvas widget was not compiled in to this server",
  /* 104 */ "Remote input devices have been disabled",
  /* 105 */ "Exclusive resource access has been disabled",
};

#endif /* !CONFIG_TEXT */

/* The End */




