/* $Id: themec.h,v 1.1 2000/09/25 00:15:26 micahjd Exp $
 *
 * themec.h - definitions used internally in the theme compiler
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#ifndef _H_THEMEC
#define _H_THEMEC

#include <string.h>
#include <stdio.h>

#include <picogui/constants.h>
#include <picogui/theme.h>

/* Parser globals */
extern int lineno;
extern int errors;

/* An entry in the symbol table */
struct symnode {
  int type;
  const char *name;
  unsigned long value;
};
 
#endif /* _H_THEMEC */
/* The End */
