/* $Id$
 *
 * init.h - High level pgserver initialization, main loop, and shutdown
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

#ifndef __H_INIT
#define __H_INIT

/* Flags for init */
#define PGINIT_NO_CONFIGFILE   (1<<1)
#define PGINIT_NO_COMMANDLINE  (1<<2)

/* Initialize all pgserver subsystems.
 * Normally this will initialize everything, if you want to change
 * that, pass it some of the above flags. If you want to pass extra
 * config parameters, you can use any of the configfile functions
 * before calling this.
 */
g_error pgserver_init(int flags, int argc, char **argv);

/* Shut down all pgserver subsystems */
g_error pgserver_shutdown(void);

/* Init, mainloop, shutdown */
g_error pgserver_main(int flags, int argc, char **argv);

/* Push this command line onto a queue of child processes to run.
 * If 'name' is NULL, this is ignored. This does not make a copy
 * of 'name' since it is assumed that it's something persistent, like
 * a config database value or a constant!
 */
g_error childqueue_push(const char *cmdline);

/* Start the next child from the queue if there is one
 * Note that since this may be called from a signal handler,
 * it only makes a note to run the child later. The child process
 * isn't actually started until the next main loop iteration.
 */
void childqueue_pop(void);

void childqueue_shutdown(void);

int childqueue_is_empty(void);

/* Start the main loop, run iterations while mainloop_is_running */
g_error pgserver_mainloop(void);

/* Affect the state of the main loop, and the value
 * returned by mainloop_is_running. Note that mainloop_start
 * doesn't actually run the main loop, just sets the run flag.
 */
void pgserver_mainloop_start(void);
void pgserver_mainloop_stop(void);

/* Return nonzero if the main loop should still be running */
int pgserver_mainloop_is_running(void);

/* Run one iteration of all pgserver's main loop activities,
 * including network, input, and rendering.
 */
g_error pgserver_mainloop_iteration(void);

#endif /* __H_INIT */

/* The End */










