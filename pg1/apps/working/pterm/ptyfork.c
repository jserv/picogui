/* $Id$
 *
 * ptyfork.c - Create a subprocess running under a pty
 *
 * WARNING!!!
 * This file is exremely OS-specific. Only tested under Linux, but should
 * theoretically work on BSD. Other OSes maybe with small modifications?
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include "pterm.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <grp.h>
#include <sys/ioctl.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/* Error details when enabled */
#define DEBUG

/* Returns -1 on error, 0 for child, and a pid for parent 
 * For the parent, returns the pty's fd in *ptyfd
 *
 * I dedicate this function to chapter 19 of
 * Advanced Programming in the UNIX Environment
 * by W. Richard Stevens
 *                               ;-)
 */

int ptyfork(int * ptyfd, char ** cmd) {

  char fname[11];
  char *chr1,*chr2;
  int master,slave;
  int pid;

#ifdef HAVE_FORKPTY

  if ( (pid = forkpty (& master, NULL, NULL, NULL)) < 0 ) {

#else /* HAVE_FORKPTY */

  strcpy(fname,"/dev/ptyAB");

  /* Find an available pair */
  for (chr1 = "abcdefpqrstuvwxyzPQRST";*chr1;chr1++) {
    fname[8] = *chr1;      /* Replace the A */

    for (chr2 = "0123456789abcdef";*chr2;chr2++) {
      fname[9] = *chr2;    /* Replace the B */
      
      /* Try to open each */
      if ((master = open(fname,O_RDWR)) < 0) {
	if (errno == ENOENT) {
	  /* maybe there is no such group of ttys, check for the next one */
	  break;
	}
	else
	  continue;
      }

      /* We have the master open. Change the 'pty' to a 'tty' */
      fname[5] = 't';

      /* See if we can open the tty. (The permissions might be fubar) */
      if ((slave = open(fname,O_RDWR)) < 0) {
	fname[5] = 'p';
	close(master);
	continue;         /* Argh. Try again */
      }
      else
	close(slave);     /* Good, but save this for later */

      /* Fork! */
      if ( (pid = fork()) < 0 ) {

#endif /* HAVE_FORKPTY */

#ifdef DEBUG
	perror("fork");
#endif
	return -1;
      }
      
      if (pid) {
	if (ptyfd) {
	  * ptyfd = master;
	}

	return pid;
      }

      /* Child */

#ifndef HAVE_FORKPTY	

      /* Shed our old controlling terminal and get a new session */
      if (setsid() < 0) {
#ifdef DEBUG
	perror("setsid");
#endif
	return -1;
      }
      
      /* Try to make the device our own, but this won't work unless the
       * terminal is setuid root */
      {
	struct group *grptr;
	int gid,fds;
	
	if ((grptr = getgrnam("tty")) != NULL)
	  gid = grptr->gr_gid;
	else
	  gid = -1;  /* no tty group */
	
	chown(fname,getuid(),gid);
	chmod(fname,S_IRUSR | S_IWUSR | S_IWGRP);  /* Good permissions for a tty */
      }
      
      /* Open the slave device */
      if ( (slave = open(fname,O_RDWR)) < 0) {
	close(master);
#ifdef DEBUG
	perror("opening slave pty");
#endif
	return -1;
      }
      
      /* Child doesn't need the master pty anymore */
      close(master);
      
      /* Acquire a controlling terminal */
      ioctl(slave,TIOCSCTTY,NULL);
      
      /* Make the slave pty our stdin/out/err */
      if (dup2(slave, STDIN_FILENO) != STDIN_FILENO) {
#ifdef DEBUG
	perror("dup2 stdin");
#endif
	return -1;
      }
      if (dup2(slave, STDOUT_FILENO) != STDOUT_FILENO) {
#ifdef DEBUG
	perror("dup2 stdout");
#endif
	return -1;
      }
      if (dup2(slave, STDERR_FILENO) != STDERR_FILENO) {
#ifdef DEBUG
	perror("dup2 stderr");
#endif
	return -1;
      }

      if (slave > STDERR_FILENO) close(slave);

#endif /* HAVE_FORKPTY */
      
      execvp (cmd [0], cmd);
      
      perror ("execvp");
      exit (127);

#ifndef HAVE_FORKPTY

    }
  }

#ifdef DEBUG
  perror("finding pseudoterminal");
#endif
  return -1;

#endif /* HAVE_FORKPTY */

}



/* The End */
