/* $Id: os.h,v 1.1 2002/11/03 04:54:24 micahjd Exp $
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

/* Os-specific init shutdown, always called first and last respectively.
 * os_init gets a copy of the flags from pgserver_init, it is expected
 * to obey applicable flags like PGINIT_NO_SIGNALS
 */
g_error os_init(int flags);
void os_shutdown(void);

/* Present a g_error message to the user */
void os_show_error(g_error e);

/* Run a child process with the given command line */
g_error os_child_run(const char *cmdline);

/* Get the return code from the most recently exited child process. */
int os_child_returncode(void);

#endif /* __H_OS */

/* The End */
