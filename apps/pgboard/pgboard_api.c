/* $Id: pgboard_api.c,v 1.8 2001/11/21 09:25:22 cgrigis Exp $
 *
 * kbd_api.c - high-level API to manipulate the PicoGUI virtual keyboard
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
 *
 *    Christian Grigis <christian.grigis@smartdata.ch>
 *            Initial release
 * 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef POCKETBEE
#include <sys/wait.h>
#include <signal.h>
#endif /* POCKETBEE */
#include <netinet/in.h>
#include <picogui.h>
#include "pgboard.h"
#include "pgboard_api.h"

#ifdef POCKETBEE

/* Location of the 'pgboard' executable */
#define PGBOARD_PATH "/usr/bin/pgboard"
/* Location of the key pattern */
#define PGBOARD_KEYMAP "/usr/share/pgboard/us_qwerty_scalable.kb"

#endif /* POCKETBEE */


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


#ifdef POCKETBEE
/* Flag indicating proper start of 'pgboard' */
static int pgboard_started;

/*
 * Signal handler activated when 'pgboard' has normally started.
 */
void sig_pgboard_ok (int sig)
{
  fprintf (stderr, "'pgboard' started properly\n");
  pgboard_started = 1;
}
#endif /* POCKETBEE */


/*
 * Run the 'pgboard' process
 *
 * return : 1 if success, 0 if an error occurred
 */
int run_pgboard ()
{
  int retValue = 0;

#ifdef POCKETBEE  
  pid_t pid;
  int status;

  /* Register handler to receive signal from 'pgboard' */
  signal (SIGUSR1, sig_pgboard_ok);
  pgboard_started = 0;

  switch (pid = vfork ())
    {
    case 0:
      /* Child */
      if (!execl (PGBOARD_PATH, PGBOARD_PATH, PGBOARD_KEYMAP, NULL))
	{
	  perror ("pgboard_api/run_pgboard()/execl()");
	}
      break;

    case -1:
      /* Error */
      perror ("pgboard_api/run_pgboard()/vfork()");
      break;

    default:
      /* Parent */

      /* Wait on 'pgboard' */
      waitpid (pid, &status, 0);

      if (pgboard_started)
	{
	  /* 'pgboard' signalled a proper start */
	  retValue = 1;
	  /* Ignore termination signals from 'pgboard' */
	  signal (SIGCHLD, SIG_IGN);
	}
      else
	{
	  /* 'pgboard' did not start properly and exited */
	  retValue = 0;
	}

      break;
    }
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
/*       struct pgmemdata data = {cmd, sizeof (struct keyboard_command), 0}; */
      pghandle kb;

/*       printf ("[pgboard_api] sending command: %d\n", cmd->type); */
      cmd->type = htons (cmd->type);

      if ( !(kb = pgFindWidget (PG_KEYBOARD_APPNAME)) )
	{
	  printf ("[pgboard_api] 'pgboard' not running, attempting to start it ...\n");

	  /* Start the virtual keyboard */
 	  if (!run_pgboard ()) return;

	  /* Wait until it has started */
	  do
	    {
	      printf ("[pgboard_api] Waiting for pgboard ...\n");
	      sleep (1);
	    }
	  while ( !(kb = pgFindWidget (PG_KEYBOARD_APPNAME)) );

	  /* 
	   * If the user command is TOGGLE, send a HIDE command first, so as
	   * to have the intended result.
	   */
	  if (cmd->type == PG_KEYBOARD_TOGGLE)
	    {
	      struct keyboard_command hide_cmd = {htons (PG_KEYBOARD_HIDE)};
/* 	      struct pgmemdata hide_data = {&hide_cmd, sizeof (struct keyboard_command), 0}; */
	      pgAppMessage (kb, pgFromMemory (&hide_cmd, sizeof (struct keyboard_command)));
	    }

	  printf ("[pgboard_api] done.\n");
	}

      /* Send the user command */
      pgAppMessage (kb, pgFromMemory (cmd, sizeof (struct keyboard_command)));
      usleep (300000);
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

  /* Flush PG_APPMSG requests */
  pgFlushRequests ();
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

  /* Flush PG_APPMSG requests */
  pgFlushRequests ();
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

  /* Flush PG_APPMSG requests */
  pgFlushRequests ();
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

  /* Flush PG_APPMSG requests */
  pgFlushRequests ();
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

  /* Flush PG_APPMSG requests */
  pgFlushRequests ();
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

  /* Flush PG_APPMSG requests */
  pgFlushRequests ();
}
