/* $Id: backend.c,v 1.1 2002/04/07 06:37:49 micahjd Exp $
 *
 * backend.c - Take the parsed objects and generate a stream of requests
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

#include "wtcompile.h"
#include <stdio.h>
#include <picogui/types.h>
#include <picogui/wt.h>

/* Given a parser data structure, allocates the compiled template
 * buffer then fills it in. Returns an error message or NULL.
 */
const char *wtBuildTemplate(struct wtparse_data *d, void **template, int *template_len) {

  *template_len = 0;

  return NULL;
}

/* The End */
