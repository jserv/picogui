/* $Id: errortext.c,v 1.27 2001/07/03 08:13:28 micahjd Exp $
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

/* A loadable error table */
char *loaded_errors[];
int   num_loaded_errors;

/* Builtin error table */
static const char *builtin_errors[] = {
#ifdef CONFIG_TEXT          /* Normal error table */
# include "defaulttext.inc"
#else                       /* Minimalist error table */
# include "tinytext.inc"
#endif
};

/* Return an error string for the given error code.
 * 
 * Search for a string in this order:
 * 1. Loaded error table 
 * 2. Builtin error table
 * 3. Numeric code
 *
 */
const char *errortext(g_error e) {
  const char *errtxt = NULL;
  int errnum = (e & 0xFF) - 1;
  static char errbuf[10];

  /* Loaded table */
  if (loaded_errors && (errnum < num_loaded_errors))
    errtxt = loaded_errors[errnum];
  if (errtxt)
    return errtxt;

  /* Builtin table */
  if (errnum < sizeof(builtin_errors))
    errtxt = builtin_errors[errnum];
  if (errtxt)
    return errtxt;

  /* Nothing left to do but give the raw numeric error code */
  sprintf(errbuf,"#%d/%d",e>>8,e & 0xFF);
  return errbuf;
}

/* The End */




