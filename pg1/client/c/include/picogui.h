/* $Id$
 *
 * picogui.h - Include file for your average PicoGUI client using the
 *             C client library
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Contributors: Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 * 
 */
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <picogui/constants.h>   /* PicoGUI client/server shared constants */
#include <picogui/network.h>     /* Client/server shared data structures */
#include <picogui/client_c.h>    /* Client API */
#include <picogui/stddialog.h>   /* Client-side standard dialogs */
#include <picogui/canvas.h>      /* Canvas widget low-level interface */
#include <picogui/pgfx.h>        /* PGFX abstract graphics interface */
#include <picogui/applet.h>	 /* PGL applet messaging system */

#ifdef __cplusplus
}
#endif // __cplusplus

/* The End */
