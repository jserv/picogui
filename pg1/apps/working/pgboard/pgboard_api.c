/* $Id$
 *
 * pgboard_api.c - high-level API to manipulate the PicoGUI virtual keyboard
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef POCKETBEE
#  include <sys/wait.h>
#  include <signal.h>
#endif /* POCKETBEE */
#include <netinet/in.h>
#include <picogui.h>
#include "pgboard.h"
#include "pgboard_api.h"

/* Prototype declarations */

/*
 * Test the presence of a physical keyboard.
 *
 * return : 1 if a physical keyboard is present, 0 otherwise
 */
static int physical_keyboard_available ();

/*
 * Send a command to the virtual keyboard.
 * A virtual keyboard process is started if none is running.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
static void send_command (struct keyboard_command * cmd, int force);


/*
 * Test the presence of a physical keyboard.
 *
 * return : 1 if a physical keyboard is present, 0 otherwise
 */
int physical_keyboard_available ()
{

#ifdef POCKETBEE

  /* Return 'false' until there is a way to access the physical keyboard */
  return 0;

#else

  return 1;

#endif /* POCKETBEE */

}


/*
 * Send a command to the virtual keyboard.
 * A virtual keyboard process is started if none is running.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void send_command (struct keyboard_command * cmd, int force)
{
  if ( cmd && (!physical_keyboard_available () || force) )
    {
      pghandle kb;

      DPRINTF ("sending command: %d\n", cmd->type);
      cmd->type = htons (cmd->type);

      while ( !(kb = pgFindWidget (PG_KEYBOARD_APPNAME)) )
	{
	  DPRINTF ("'pgboard' not running, please start it ...\n");

	  sleep (2);
	}

      /* Send the user command */
      pgAppMessage (kb, pgFromMemory (cmd, sizeof (struct keyboard_command)));

      /* Flush PG_APPMSG requests */
      pgFlushRequests ();
    }
}


/*
 * Make the virtual keyboard visible and set it to the given key pattern.
 *
 * key_pattern : the key pattern to which the virtual keyboard is to be set
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void showKeyboard (unsigned short key_pattern, int force)
{
  struct keyboard_command cmd;

  /* Select the given key pattern */
  cmd.type = PG_KEYBOARD_SELECT_PATTERN;
  cmd.data.pattern = htons (key_pattern);
  send_command (&cmd, force);

  /* Enable the keyboard */
  cmd.type = PG_KEYBOARD_ENABLE;
  send_command (&cmd, force);

  /* Show the keyboard */
  cmd.type = PG_KEYBOARD_SHOW;
  send_command (&cmd, force);
}


/*
 * Hide the virtual keyboard.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void hideKeyboard (int force)
{
  struct keyboard_command cmd;

  cmd.type = PG_KEYBOARD_HIDE;
  send_command (&cmd, force);
}


/*
 * Toggle the virtual keyboard.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void toggleKeyboard (int force)
{
  struct keyboard_command cmd;

  cmd.type = PG_KEYBOARD_TOGGLE;
  send_command (&cmd, force);
}


/*
 * Disable the virtual keyboard.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void disableKeyboard (int force)
{
  struct keyboard_command cmd;

  cmd.type = PG_KEYBOARD_DISABLE;
  send_command (&cmd, force);
}


/*
 * Push the current virtual keyboard context.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void pushKeyboardContext (int force)
{
  struct keyboard_command cmd;

  cmd.type = PG_KEYBOARD_PUSH_CONTEXT;
  send_command (&cmd, force);
}


/*
 * Pop the last pushed virtual keyboard context.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void popKeyboardContext (int force)
{
  struct keyboard_command cmd;

  cmd.type = PG_KEYBOARD_POP_CONTEXT;
  send_command (&cmd, force);
}


/*
 * Block the keyboard.
 * The keyboard will ignore all further commands until it is released.
 * Usage of the keyboard (i.e. clicking on the keys), however, is unaffected.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void blockKeyboard (int force)
{
  struct keyboard_command cmd;

  cmd.type = PG_KEYBOARD_BLOCK;
  send_command (&cmd, force);
}


/*
 * Release the keyboard.
 * The keyboard will resume executing commands. All commands received since
 * the keyboard was blocked are lost.
 *
 * force       : 0 --> the command is ignored if a physical keyboard is present
 *               1 --> the command is always sent to the virtual keyboard
 */
void releaseKeyboard (int force)
{
  struct keyboard_command cmd;

  cmd.type = PG_KEYBOARD_RELEASE;
  send_command (&cmd, force);
}
