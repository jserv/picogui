/* $Id: errortext.c,v 1.26 2001/06/28 21:06:44 micahjd Exp $
 *
 * errortext.c - optional error message strings
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
 * Brandon Smith <lottabs2@yahoo.com>
 * 
 * 
 */

#include <pgserver/common.h>

/************************ Numeric Errors */

/* If CONFIG_TEXT is not defined, no error
 * strings are stored- the user gets a nice
 * hexadecimal error code that is in most
 * cases not too much worse than the string :)
 */

#ifndef CONFIG_TEXT

/* Eek! Cryptic, but saves lots of space */
const char *errortext(g_error e) {
  static char err[11];
  sprintf(err,"Err 0x%04X",e);
  return err;
}

#else /* CONFIG_TEXT */

/************************ String errors */

static const char *errors[];

const char *errortext(g_error e) {
	return errors[(e & 0xFF)-1];
}

/* Builtin error table, can be overrided by translations */
static const char *errors[] = {
#include "defaulttext.inc"
};

#endif /* !CONFIG_TEXT */

/* The End */




