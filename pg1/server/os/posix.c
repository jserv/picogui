/* $Id$
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
#include <pgserver/os_posix.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#ifndef __CYGWIN__
/* FIXME: this can be fixed by finding the proper ipc and shm stuff in cygwin
 * and making it work... but I'm not doing it right now ;-) -- lalo */
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* __CYGWIN__ */
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>

#ifndef NAME_MAX
#warning "NAME_MAX is not defined on your system: Guessing..."
#define NAME_MAX 512
#endif /* NAME_MAX */

extern char **environ;

/* Return value of the last process to exit */
int os_posix_child_return;

/* Reference point for os_getticks */
static struct timeval os_posix_first_tick;

/* Value set with os_set_timer */
u32 os_posix_timer;

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
  os_posix_signals_install();
  
  /* Get a reference point for os_getticks */
  gettimeofday(&os_posix_first_tick,NULL);

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
    /* FIXME: This should pass the error back to os_child_run
     *        somehow instead of just exiting here!
     */
    os_show_error(mkerror(PG_ERRT_BADPARAM,55));
    kill(my_pid,SIGTERM);
    exit(1);
  }
  
  return success;
}

/* Get the return code from the most recently exited child process. */
int os_child_returncode(void) {
  return os_posix_child_return;
}

/* Get the number of milliseconds since an arbitrary epoch */
u32 os_getticks(void) {
    static struct timeval now;

  gettimeofday(&now,NULL);
  return (now.tv_sec  - os_posix_first_tick.tv_sec ) * 1000 + 
         (now.tv_usec - os_posix_first_tick.tv_usec) / 1000;
}

/* Set a timer that will call master_timer() 'ticks' milliseconds
 * from now. If 'ticks' is 0, no new timer will be set. In any
 * event, the previously set timer is cancelled.
 *
 * This implementation sets an itimer that will generate SIGALRM,
 * the signal handler then calls master_timer()
 */
void os_set_timer(u32 ticks) {
  struct itimerval itv;
  memset(&itv,0,sizeof(struct itimerval));
  os_posix_timer = ticks;

  if (ticks) {
    ticks -= os_getticks();
    itv.it_value.tv_sec  = (ticks/1000);
    itv.it_value.tv_usec = (ticks%1000)*1000;
  }
  setitimer(ITIMER_REAL,&itv,NULL);
}

u32 os_get_timer(void) {
  return os_posix_timer;
}

/* Create a new shared memory segment, returning a key, id, and pointer.
 * The key is passed to the client so it can attach to the section, the id
 * is passed to os_shm_free(), and the pointer is self explanatory.
 * The segment will have ownership set to the supplied uid.
 */
g_error os_shm_alloc(u8 **shmaddr, u32 size, s32 *id, s32 *key, int secure) {
#ifndef __CYGWIN__
  /* Find an unused SHM key, starting with a magic number. */
  while ((*id = shmget(os_posix_magic,size,IPC_CREAT | IPC_EXCL | 
		       (secure ? 0600 : 0666))) < 0) {
    if (errno != EEXIST)
      return mkerror(PG_ERRT_IO,5);     /* Error creating SHM segment */
    os_posix_magic++;
  }
  *key = os_posix_magic;

  /* Attach it to our address space */
  *shmaddr = shmat(*id,NULL,0);
  if (!*shmaddr) {
    shmctl(*key, IPC_RMID, NULL);
    return mkerror(PG_ERRT_IO,5);     /* Error creating SHM segment */
  }

#endif /* ! __CYGWIN__ */
  return success;
}

void os_shm_set_uid(s32 id, u32 uid) {
#ifndef __CYGWIN__
  struct shmid_ds ds;
  shmctl(id,IPC_STAT,&ds);
  ds.shm_perm.uid = uid;
  shmctl(id,IPC_SET,&ds);
#endif /* ! __CYGWIN__ */
}

void os_shm_free(u8 *shmaddr, s32 id) {
#ifndef __CYGWIN__
  /* Unmap this segment and remove the key itself */
  shmdt(shmaddr);
  shmctl(id, IPC_RMID, NULL);
#endif /* ! __CYGWIN__ */
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
  DIR *d = opendir(directory);
  struct dirent *dent;
  char buf[NAME_MAX];
  struct stat st;
  
  if (!d)
    return;
  
  while ((dent = readdir(d))) {
    /* Skip hidden files, "..", and "." */
    if (dent->d_name[0] == '.')
      continue;
    
    /* Find the path of this entry */
    buf[NAME_MAX-1] = 0;
    strncpy(buf,directory,NAME_MAX-2);
    strcat(buf,"/");
    strncat(buf,dent->d_name,NAME_MAX-1-strlen(buf));
    
    /* If it's a directory, recurse */
    if (stat(buf,&st) < 0)
      continue;
    if (S_ISDIR(st.st_mode)) {
      r_os_dir_scan(buf,base_len,callback);
      continue;
    }
    
    (*callback)(buf,base_len);
  }
  
  closedir(d);
}

/* The End */
