/* $Id$
 *
 * pterm.h - PicoGUI Terminal (the 'p' is silent :)
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef _H_PTERM
#define _H_PTERM

/****** Functions provided by ptyfork.c ***/

/* Returns -1 on error, 0 for child, and a pid for parent 
 * For the parent, returns the pty's fd in *ptyfd
 *
 * I dedicate this function to chapter 19 of
 * Advanced Programming in the UNIX Environment
 * by W. Richard Stevens
 *                               ;-)
 */

int ptyfork (int * ptyfd, char ** cmd);

#endif /* _H_PTERM */

/* The End */
