/* $Id$
 *
 * tuxclient.h - Network interface to tuxphone
 *
 * Copyright (C) 2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 */

#ifndef __H_TUXCLIENT
#define __H_TUXCLIENT

#include "tuxphone.h"

extern int phone_fd;

int phone_open(void);
void phone_register_events(int fd, long mask);
void phone_get_event(int fd);
void phone_dial(int fd, const char *number);

/* If it has been more than some amount of time after a ring, the
 * call will be considered completed but unanswered
 */
void phone_check_ring_timeout(void);

#endif /* __H_TUXCLIENT */

/* The End */

