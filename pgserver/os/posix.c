/* $Id: posix.c,v 1.1 2002/11/03 04:54:24 micahjd Exp $
 *
 * posix.c - Implementation of OS-specific functions for POSIX-compatible systems
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micah@homesoftware.com>
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

#include <pgserver/common.h>
#include <pgserver/os.h>
#include <pgserver/init.h>
#include <pgserver/os_posix.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
extern char **environ;

/* Return value of the last process to exit */
int os_posix_child_return;


/********************************************** Public OS interface */

/* OS-specific init shutdown, always called first and last respectively.
 * os_init gets a copy of the flags from pgserver_init, it is expected
 * to obey applicable flags like PGINIT_NO_SIGNALS
 */
g_error os_init(int flags) {
  g_error e;

  if (!(flags & PGINIT_NO_SIGNALS))
    os_posix_signals_install();
  
  return success;
}

void os_shutdown(void) {
}

/* Present a g_error message to the user */
void os_show_error(g_error e) {
  prerror(e);
}

/* Run a child process with the given command line */
g_error os_child_run(const char *cmdline) {
  int my_pid = getpid();

  if (!cmdline)
    return success;

# ifdef CONFIG_OS_UCLINUX
  if (!vfork()) {
# else
  if (!fork()) {
# endif
    char *sargv[4];
    sargv[0] = "sh";
    sargv[1] = "-c";
    sargv[2] = (char *) cmdline;
    sargv[3] = 0;
    execve("/bin/sh",sargv,environ);
    prerror(mkerror(PG_ERRT_BADPARAM,55));
    kill(my_pid,SIGTERM);
    exit(1);
  }
  
  return success;
}

/* Get the return code from the most recently exited child process. */
int os_child_returncode(void) {
  return os_posix_child_return;
}

/* The End */
