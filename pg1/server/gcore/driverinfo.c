/* $Id$
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

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/input.h>

#define DRV(name,reg) {name,reg},

struct vidinfo videodrivers[] = {
#include "videodrivers.inc"
  DRV(NULL,NULL)
};

struct inputinfo inputdrivers[] = {
#include "inputdrivers.inc"
  DRV(NULL,NULL)
};
  
/* The End */
