/* $Id$
 * 
 * version.h - Define the version of the picogui server
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
 */

#ifndef __VERSION_H
#define __VERSION_H

/* This macro packs a version number into the numeric format used by pgserver.
 * 2 bits are reserved, 5 used for the major version, 8 for the minor version,
 * and 1 for a flag indicating whether this is a release or not.
 */
#define PGVERSION(major,minor,release) (((major)<<9)|((minor)<<1)|(release))

/* This is the version number of the pgserver these headers are from.
 * The client should not rely on this to know what version of pgserver it
 * has connected to, it should use the version returned in the pghello packet!
 */
#define PGSERVER_VERSION_NUMBER PGVERSION(0,46,0)
#define PGSERVER_VERSION_STRING "pre-0.46 CVS"

#endif /* __WT_H */

/* The End */
