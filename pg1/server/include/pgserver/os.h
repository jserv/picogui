/* $Id$
 *
 * os.h - Interface to OS-specific functions used by pgserver, independent
 *        of the actual OS in use. Functions that only exist in a particular
 *        OS should go in a separate os_*.h header.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#ifndef __H_OS
#define __H_OS

/* OS-specific init shutdown, always called first and last respectively.
 */
g_error os_init(void);
void os_shutdown(void);

/* Present a g_error message to the user */
void os_show_error(g_error e);

/* Run a child process with the given command line */
g_error os_child_run(const char *cmdline);

/* Get the return code from the most recently exited child process. */
int os_child_returncode(void);

/* Get the number of milliseconds since an arbitrary epoch */
u32 os_getticks(void);

/* Set a timer that will call master_timer() as soon as possible after
 * os_getticks() passes the given 'ticks' value. If 'ticks' is 0, disable
 * the timer.
 */
void os_set_timer(u32 ticks);

/* Return the timer value most recently set with os_set_timer() */
u32 os_get_timer(void);

/* Create a new shared memory segment, returning a key, id, and pointer.
 * The key is passed to the client so it can attach to the section, the id
 * is passed to os_shm_free(), and the pointer is self explanatory.
 * The segment will have ownership set to the supplied uid.
 *
 * If the 'secure' flag is set when calling os_shm_alloc, then only
 * the process set with os_shm_set_uid() can access the shm segment.
 * Otherwise, any process can connect to it.
 */
g_error os_shm_alloc(u8 **shmaddr, u32 size, s32 *id, s32 *key, int secure);
void os_shm_set_uid(s32 id, u32 uid);
void os_shm_free(u8 *shmaddr, s32 id);

/* Recursively scan through all files and directories in the given path,
 * calling the callback for each file found. The callback is given the
 * full path of the file, and the number of characters from the beginning
 * of that file that make up the original path.
 */
void os_dir_scan(const char *dir, void (*callback)(const char *file, int pathlen));

#endif /* __H_OS */

/* The End */
