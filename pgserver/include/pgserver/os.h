/* $Id: os.h,v 1.2 2002/11/03 22:44:47 micahjd Exp $
 *
 * os.h - Interface to OS-specific functions used by pgserver, independent
 *        of the actual OS in use. Functions that only exist in a particular
 *        OS should go in a separate os_*.h header.
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
 * os_getticks() passes the given 'ticks' value.
 */
void os_set_timer(u32 ticks);

/* Return the timer value most recently set with os_set_timer() */
u32 os_get_timer(void);

#endif /* __H_OS */

/* The End */
