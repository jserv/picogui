/* $Id: timer.h,v 1.5 2002/01/06 09:22:58 micahjd Exp $
 *
 * timer.h - OS-specific stuff for setting timers and
 *            figuring out how much time has passed
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

#ifndef __H_TIMER
#define __H_TIMER

#include <pgserver/g_error.h>
#include <pgserver/widget.h>

g_error timer_init(void);
void timer_release(void);

/* General-purpose func to get the time
   in milliseconds
*/
u32 getticks(void);

/* reset the inactivity timer */
void inactivity_reset();

/* retrieve the number of milliseconds since the last activity */
u32 inactivity_get();

/* Set the number of milliseconds since last activity.
 * inactivity_set(0) is equivalent to inactivity_reset().
 */
void inactivity_set(u32 t);

#endif /* __H_TIMER */

/* The End */
