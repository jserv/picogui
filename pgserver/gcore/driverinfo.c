/* $Id: driverinfo.c,v 1.16 2001/02/17 05:18:40 micahjd Exp $
 *
 * driverinfo.c - has a static array with information about
 *                installed drivers.
 *                This file is all macro junk, mostly to implement
 *                the run-time function pointer tables needed by 
 *                ucLinux. The stuff you want to change to add new
 *                drivers should be in videodrivers.inc and
 *                inputdrivers.inc
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/video.h>
#include <pgserver/input.h>

/*********** Normal version *****/
#ifndef RUNTIME_FUNCPTR

#define DRV(name,reg) {name,reg},

struct vidinfo videodrivers[] = {
#include "videodrivers.inc"
   DRV(NULL,NULL)
};

struct inputinfo inputdrivers[] = {
#include "inputdrivers.inc"
  /* End */ DRV(NULL,NULL)
};

/*********** Runtime table junk ***/

#else /* RUNTIME_FUNCPTR */

/***** Video */

/* First count the elements to allocate the array 
 * (yes, I do abuse the preprocessor...) 
 */

#define DRV(x,y) +1
struct vidinfo videodrivers[1
#include "videodrivers.inc"
			    ];
struct inputinfo inputdrivers[1
#include "inputdrivers.inc"
			      ];
#undef DRV

/* Initialization function */

#define DRV(x,y) p->name = x; p->regfunc = y; p++;

void drivertab_init(void) {
     {
	struct vidinfo *p = videodrivers;
#include "videodrivers.inc"
	DRV(NULL,NULL);
     }
     {
	struct inputinfo *p = inputdrivers;
#include "inputdrivers.inc"
	DRV(NULL,NULL);
     }
}

#endif /* RUNTIME_FUNCPTR */
  
/* The End */
