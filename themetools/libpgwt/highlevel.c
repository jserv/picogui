/* $Id: highlevel.c,v 1.1 2002/04/07 06:37:49 micahjd Exp $
 *
 * highlevel.c - High-level interface to the Widget Template compiler
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

#include <expat.h>
#include <malloc.h>
#include "wtcompile.h"

/* Create a Widget Template parser */
wtParser wtNewParser(void) {
  wtParser p;
  p = XML_ParserCreate(NULL);
  if (p) {
    /* Allocate our extra data */
    XML_SetUserData(p, malloc(sizeof(struct wtparse_data)));
  }
  wtInstallHandlers(p);
  return p;
}

/* Finish parsing, and return the compiled template */
const char *wtFinishParser(wtParser wtp, void **template, int *template_len) {
  const char *ret;

  ret = wtBuildTemplate((struct wtparse_data *) XML_GetUserData(wtp),template,template_len);

  free(XML_GetUserData(wtp));
  XML_ParserFree(wtp);
  return ret;
}

/* Parse a block of data, returns an error string
 * if there's an error, NULL if not.
 *
 * isFinal must be 1 for the last block parsed.
 */
const char *wtParse(wtParser wtp, const char *s, int len, int isFinal) {
  if (XML_Parse(wtp,s,len,isFinal)) {
    /* Parse error */
    return XML_ErrorString(XML_GetErrorCode(wtp));
  }
  return NULL;
}

/* The End */
