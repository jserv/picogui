/*
 * Copyright (C) 2003 SMARTDATA (SA), Alain Paschoud 
 *
 * File written by Alain Paschoud <alain.paschoud@smartdata.ch>, 2003
 *
 * See README.btkey for more information
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
 * Author:
 *   Alain Paschoud <alain.paschoud@smartdata.ch>
 * 
 * Contributors:
 *   Olivier Bornet <Olivier.Bornet@smardata.ch>
 *
 * $Id$
 */


#include <termios.h>
#include <sys/poll.h>

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/configfile.h>
#include <picogui/pgkeys.h>
#include <picogui/constants.h>

/* The next include (btkey-report.h) is not distributed with
   PicoGUI. Please see README.btkey for details. */
#include <btkey-report.h>

#define	BTKEY_FIFO_PATH	"/dev/btfifo"  

#define LOCAL_DEBUG 	0

#if LOCAL_DEBUG
# define DPRINTF(x...) 	printf(__FILE__": " x)
# define TRACE(x...) 	printf (__FILE__" " __FUNCTION__ "\n")
#else
# define DPRINTF(x...)
# define TRACE(x...)
#endif

int btkey_fd;	/* File descriptor for incoming bt keys */
struct termios old;	/* original terminal modes */

/*
 * Open the FIFO.
 * This is realy simple, we just use a special file handle
 * that allows non-blocking I/O, and put the terminal into
 * character mode.
 */
g_error btkey_init(void)
{
  struct termios	 new;
  struct stat 	 file_infos;
  
  TRACE ();
  
  /* If FIFO doesn't exist, create it */
  if (stat(BTKEY_FIFO_PATH, &file_infos) != 0) {
    if (errno == ENOENT) {
      /* Create the file */
      mkfifo(BTKEY_FIFO_PATH, 0666);
    }
    else {
      error ("BTkey error : Can't create FIFO to communicate"
	     " with PicoGUI. Exit");
    }
  }
  
  /* create the fifo */
  mkfifo (BTKEY_FIFO_PATH, 0666);

  /* Open it in non-blocking mode */
  btkey_fd = open (BTKEY_FIFO_PATH, O_RDONLY | O_NONBLOCK);
  
  if (btkey_fd < 0)
    return -1;
  
  return 1;
 err:
  close(btkey_fd);
  btkey_fd = -1;
  return 0;
}

/*
 * Close the keyboard.
 * This resets the terminal modes.
 */	
void btkey_close(void)
{
  if (btkey_fd == 0)
    return;

  close(btkey_fd);
  btkey_fd = 0;
}

void btkey_fd_init(int *n,fd_set *readfds,struct timeval *timeout)
{
  if ((*n)<(btkey_fd+1))
    *n = btkey_fd+1;
  FD_SET(btkey_fd,readfds);
}


int btkey_fd_activate(int fd)
{
  int curkey[2];
  int key;
  int pressed;
  int cc;	/* nb characters read */
  int pg_key;
  
  TRACE ();
  
  if( fd != btkey_fd ) 
    return 0;
  
  DPRINTF ("Now reading data\n");
  
  /* Read 2 bytes. The first indicates it the key has been pressed
     or released, and the second one give the character */
  cc = read(fd, curkey, 2 * sizeof(int));

  if (cc < 2 * sizeof(int)) {
    /* incorrect reading, maybe end of file on fifo, so close it and
       open again */
    close (btkey_fd);
    btkey_fd = open (BTKEY_FIFO_PATH, O_RDONLY | O_NONBLOCK);
    return 1;
  }

  pressed = curkey[0];
  key = curkey[1];
  
  /* If pressed == BTKEYREPORT_MESSAGE, this means that this is not a character that
     should be transmitted, this is a specific event */
  
  DPRINTF ("Read bytes(%d), key: %c, pressed: %d\n", cc, key, pressed);
  
  if ( cc ) {
    if (pressed == BTKEYREPORT_MESSAGE) {
      /* To Do : interpret event */
      switch (key) {
      case BTKEYREPORT_KBD_CONNECTED:
	pg_key = PG_KBD_CONNECTED;
	break;
      case BTKEYREPORT_KBD_NOT_CONNECTED:
	pg_key = PG_KBD_NOT_CONNECTED;
	break;
      case BTKEYREPORT_PHONE_CONNECTED:
	pg_key = PG_PHONE_CONNECTED;
	break;
      case BTKEYREPORT_PHONE_NOT_CONNECTED:
	pg_key = PG_PHONE_NOT_CONNECTED;
	break;
      case BTKEYREPORT_PHONE_KEY_FORWARDING_ENABLED:
	pg_key = PG_PHONE_KEY_FORWARDING_ENABLED;
	break;
      case BTKEYREPORT_PHONE_KEY_FORWARDING_DISABLED:
	pg_key = PG_PHONE_KEY_FORWARDING_DISABLED;
	break;
      case BTKEYREPORT_PHONE_KEY_BT_STOP:
	pg_key = PG_PHONE_KEY_BT_STOP;
	break;
      case BTKEYREPORT_PHONE_KEY_BT_START:
	pg_key = PG_PHONE_KEY_BT_START;
	break;
      default:
	goto end;
      }
      /* Send the message */
      infilter_send_key(PG_TRIGGER_KEYDOWN,pg_key,0);
    }
    
    else if (key != 0) {
      /* Give key-up or key-down event */
      if (pressed) {
	if (! (key & 0xFF0000) ) { /* Printable */
	  infilter_send_key(PG_TRIGGER_CHAR,key,0);
	}
	infilter_send_key(PG_TRIGGER_KEYDOWN,key & 0xFFFF,0);
      }
      else {
	infilter_send_key(PG_TRIGGER_KEYUP,key & 0xFFFF,0);
      }
    }
    
    return 1;			/* keypress*/
  }
  if ((cc < 0) && (errno != EINTR) && (errno != EAGAIN)) {
    return -1;
  }
end:
  return 0;
}

g_error btkey_regfunc(struct inlib *i)
{
  TRACE ();
  i->init = &btkey_init;
  i->close = &btkey_close;
  i->fd_init = &btkey_fd_init;
  i->fd_activate = &btkey_fd_activate;
  return success;
}
