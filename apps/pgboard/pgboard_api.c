/* $Id: pgboard_api.c,v 1.13 2002/02/05 15:46:34 cgrigis Exp $
 *
 * pgboard_api.c - high-level API to manipulate the PicoGUI virtual keyboard
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

#ifdef USE_RM

#include <rm_client.h>

#endif /* USE_RM */

#ifdef USE_RM

/* Status code for all RM operations */
RMStatus rm_status;

#endif /* USE_RM */

/* Prototype declarations */

/*
 * Run the 'pgboard' process
 */
static int run_pgboard ();

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
 * Run the 'pgboard' process
 *
 * return : 1 if success, 0 if an error occurred
 */
int run_pgboard ()
{
  int retValue = 0;

#ifdef POCKETBEE  

#  ifdef USE_RM

  /* Initialize RM */
  if ( (rm_status = rm_init ()) != RM_OK ) {
    DPRINTF ("cannot init RM (error: %d)\n", rm_status);
  }

  DPRINTF ("about to wait on RM\n");

  /* Wait on the RM to start 'pgboard' */
  if ( (rm_status = rm_monitor_wait ("pgboard", 5, NULL)) == RM_OK ) {
    /* 'pgboard' properly started */
    retValue = 1;
  } else if (rm_status == RM_ERR_TIMEOUT) {
    /* 'pgboard' did not start within the timeout limit */
    DPRINTF ("timeout expired waiting on RM\n");
  } else {
    /* RM returned an error */
    DPRINTF ("error waiting on RM (error: %d)\n", rm_status);
  }

#  endif /* USE_RM */

#endif /* POCKETBEE */

  return retValue;
}


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

      if ( !(kb = pgFindWidget (PG_KEYBOARD_APPNAME)) )
	{
	  DPRINTF ("'pgboard' not running, attempting to start it ...\n");

	  /* Start the virtual keyboard */
 	  if (!run_pgboard ()) return;

	  /* Wait until it has started */
	  while ( !(kb = pgFindWidget (PG_KEYBOARD_APPNAME)) )
	    {
	      DPRINTF ("Waiting for pgboard ...\n");
	      sleep (1);
	    }

	  /* 
	   * If the user command is TOGGLE, send a HIDE command first, so as
	   * to have the intended result.
	   */
	  if (cmd->type == PG_KEYBOARD_TOGGLE)
	    {
	      struct keyboard_command hide_cmd = {htons (PG_KEYBOARD_HIDE)};
	      pgAppMessage (kb, pgFromMemory (&hide_cmd, sizeof (struct keyboard_command)));
	    }

	  DPRINTF ("done.\n");
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
