/* $Id$
 *
 * pgboard.h - declarations associated with the virtual keyboard (pgboard)
 * 
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
 *    Christian Grigis <christian.grigis@smartdata.ch>
 *            Initial release
 * 
 */

#ifndef _PGBOARD_H
#define _PGBOARD_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#undef DPRINTF
#ifdef DEBUG
#  define DPRINTF(arg...) printf ("[pgboard] " arg)
#else
#  define DPRINTF(arg...)
#endif

/* Keyboard application name */
#define PG_KEYBOARD_APPNAME "app/Keyboard"


/* Constants for keyboard messages */

#define PG_KEYBOARD_SHOW            1  /* Make the keyboard app visible */
#define PG_KEYBOARD_HIDE            2  /* Make the keyboard app's size 0 */
#define PG_KEYBOARD_TOGGLE          3  /* Toggle between SHOW and HIDE */
#define PG_KEYBOARD_ENABLE          4  /* Enable the keyboard app */
#define PG_KEYBOARD_DISABLE         5  /* Disable the keyboard app */
#define PG_KEYBOARD_TOGGLE_DISPLAY  6  /* Toggle between ENABLE and DISABLE */
#define PG_KEYBOARD_SELECT_PATTERN  7  /* Select a different pattern */
#define PG_KEYBOARD_PUSH_CONTEXT    8  /* Push the current keyboard context */
#define PG_KEYBOARD_POP_CONTEXT     9  /* Pop the last pushed keyboard context */
#define PG_KEYBOARD_BLOCK          10  /* Block the keyboard in its current state (make it ignore further commands) */
#define PG_KEYBOARD_RELEASE        11  /* Release the keyboard (allow it to take new commands) */


/* Constants for keyboard patterns */

#define PG_KBPATTERN_NORMAL               1
#define PG_KBPATTERN_SHIFT                2
#define PG_KBPATTERN_CAPSLOCK             3
#define PG_KBPATTERN_CONTROL              4
#define PG_KBPATTERN_NUMERIC              5
#define PG_KBPATTERN_SYMBOLS              6
#define PG_KBPATTERN_INTERNATIONAL        7
#define PG_KBPATTERN_SHIFT_INTERNATIONAL  8


/* Data structure for a keyboard command */

struct keyboard_command
{
  unsigned short type;    /* Network byte order, 16-bits */

  /* Command-dependent data */
  union
  {
    unsigned short pattern;
  } data;
};


#endif /* _PGBOARD_H */
