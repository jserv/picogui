/* $Id: posix.c 3929 2003-05-15 02:23:35Z lalo $
 *
 * posix.c - Implementation of OS-specific functions for POSIX-compatible systems
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micah@homesoftware.com>
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
#include <pgserver/os_mingw.h>

#include <io.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>


extern char **environ;

/* Return value of the last process to exit */
int os_posix_child_return;

/* Reference point for os_getticks */
static struct timeval os_posix_first_tick;

/* Value set with os_set_timer */
s32 os_timer;
s32 os_timer_id;

/* Magic number used to generate SHM keys
 * (Anybody nerdy enough to know where it comes from should be punished ;)
 */
int os_posix_magic = 3263827;

void r_os_dir_scan(const char *directory, int base_len, 
		   void (*callback)(const char *file, int pathlen));


/********************************************** Public OS interface */

g_error os_init(void) {
  g_error e;

  /* Signal handlers */
  //os_posix_signals_install();
  
  return success;
}

void os_shutdown(void) {
  /* Nothing to do here... */
}

/* Present a g_error message to the user */
void os_show_error(g_error e) {
  prerror(e);
}

/* Run a child process with the given command line */
g_error os_child_run(const char *cmdline) {
  /*if (!cmdline)
    return success;

    char *sargv[4];
    sargv[0] = "sh";
    sargv[1] = "-c";
    sargv[2] = (char *) cmdline;
    sargv[3] = 0;
    execve("/bin/sh",sargv,environ);
    /* FIXME: This should pass the error back to os_child_run
     *        somehow instead of just exiting here!
     */
    /*os_show_error(mkerror(PG_ERRT_BADPARAM,55));
    kill(my_pid,SIGTERM);
    exit(1);
  }*/
  printf ("Child Run: %s\n", cmdline);
  return success;
}

/* Get the return code from the most recently exited child process. */
int os_child_returncode(void) {
  return os_posix_child_return;
}

/* Get the number of milliseconds since an arbitrary epoch */
u32 os_getticks(void) {
    static struct timeval now;

  return GetTickCount ();
}

VOID CALLBACK timer_proc(HWND hwnd, UINT uMsg,
                        UINT_PTR idEvent, DWORD dwTime)
{
  KillTimer(0, os_timer_id);
  os_timer_id = 0;
  master_timer();
}


/* Set a timer that will call master_timer() 'ticks' milliseconds
 * from now. If 'ticks' is 0, no new timer will be set. In any
 * event, the previously set timer is cancelled.
 *
 * This implementation sets an itimer that will generate SIGALRM,
 * the signal handler then calls master_timer()
 */
void os_set_timer(u32 ticks) {
  if (os_timer_id != 0)
    KillTimer(0, os_timer_id);
  os_timer = ticks;
  os_timer_id = SetTimer(0, 0, ticks, timer_proc);
}

u32 os_get_timer(void) {

  return os_timer;
}

/* Create a new shared memory segment, returning a key, id, and pointer.
 * The key is passed to the client so it can attach to the section, the id
 * is passed to os_shm_free(), and the pointer is self explanatory.
 * The segment will have ownership set to the supplied uid.
 */
g_error os_shm_alloc(u8 **shmaddr, u32 size, s32 *id, s32 *key, int secure) {
  return success;
}

void os_shm_set_uid(s32 id, u32 uid) {
}

void os_shm_free(u8 *shmaddr, s32 id) {
}

/* Recursively scan through all files and directories in the given path,
 * calling the callback for each file found. The callback is given the
 * full path of the file, and the number of characters from the beginning
 * of that file that make up the original path.
 */
void os_dir_scan(const char *dir, void (*callback)(const char *file, int pathlen)) {
  r_os_dir_scan(dir,strlen(dir),callback);
}


/********************************************** Internal utilities */

/* Recursive guts for os_dir_scan() */
void r_os_dir_scan(const char *directory, int base_len, 
		   void (*callback)(const char *file, int pathlen)) {
  char buf[MAX_PATH];
  struct stat st;
  
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind;
  bool finished = FALSE;

  hFind = FindFirstFile (directory, &FindFileData);

  if (hFind != INVALID_HANDLE_VALUE)
    return;

  while (!finished)
  {
    strcpy(buf, directory);
    strcat(buf, "/");
    strcat(buf, FindFileData.cFileName);
    (*callback)(buf,base_len);
    finished = FindNextFile (hFind, &FindFileData);
  }
}

/* The End */
