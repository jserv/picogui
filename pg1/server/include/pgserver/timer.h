/* $Id$
 *
 * timer.h - OS-specific stuff for setting timers and
 *            figuring out how much time has passed
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

#ifndef __H_TIMER
#define __H_TIMER

#include <pgserver/g_error.h>
#include <pgserver/widget.h>

/* Initialize the timer subsystem */
void timer_init(void);

/* reset the inactivity timer */
void inactivity_reset();

/* retrieve the number of milliseconds since the last activity */
u32 inactivity_get();

/* Set the number of milliseconds since last activity.
 * inactivity_set(0) is equivalent to inactivity_reset().
 */
void inactivity_set(u32 t);

/*
 * Set a timer.  After the specified interval, in milliseconds,
 * the widget will recieve a PG_TRIGGER_TIMER
 */
void install_timer(struct widget *self,u32 interval);
void remove_timer(struct widget *w);

/* This is the callback triggered by os_set_timer, it is the entry
 * point for all widget timers and inactivity timers.
 */
void master_timer(void);

#endif /* __H_TIMER */

/* The End */
