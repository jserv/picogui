/* $Id$
 *
 * pgboard_api.h - high-level API to manipulate the PicoGUI virtual keyboard
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

#ifndef _PGBOARD_API_H
#define _PGBOARD_API_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#undef DPRINTF
#ifdef DEBUG
#  define DPRINTF(arg...) printf ("[pgboard_api] " arg)
#else
#  define DPRINTF(arg...)
#endif

/* Prototype declarations */

/* 
 * NOTE: 
 * All the commands will start a virtual keyboard process if none is running.
 */

/*
 * Make the virtual keyboard visible and set it to the given key pattern.
 *
 * key_pattern : the key pattern to which the virtual keyboard is to be set
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void showKeyboard (unsigned short key_pattern, int force);


/*
 * Hide the virtual keyboard.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void hideKeyboard (int force);


/*
 * Toggle the virtual keyboard.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void toggleKeyboard (int force);


/*
 * Disable the virtual keyboard.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void disableKeyboard (int force);


/*
 * Push the current virtual keyboard context.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void pushKeyboardContext (int force);


/*
 * Pop the last pushed virtual keyboard context.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void popKeyboardContext (int force);


/*
 * Block the keyboard.
 * The keyboard will ignore all further commands until it is released.
 * Usage of the keyboard (i.e. clicking on the keys), however, is unaffected.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void blockKeyboard (int force);


/*
 * Release the keyboard.
 * The keyboard will resume executing commands. All commands received since
 * the keyboard was blocked are lost.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void releaseKeyboard (int force);


#endif /* _PGBOARD_API_H */
