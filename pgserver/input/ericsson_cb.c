/* $Id: ericsson_cb.c,v 1.1 2002/03/01 08:04:54 bauermeister Exp $
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
 * Experimental keyboard driver for the Remora project
 * Copyright (C) 2001 Smartdata <www.smartdata.ch>
 *
 * Contributors:
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch>
 * 
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/configfile.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/termios.h>
#include <sys/ioctl.h>

/*****************************************************************************/



/*****************************************************************************/

g_error ericsson_cb_regfunc(struct inlib *i)
{
  return success;
}
