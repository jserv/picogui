/* $Id: xmlparse.c,v 1.1 2002/04/07 06:37:49 micahjd Exp $
 *
 * xmlparse.c - XML-parsing half of the Widget Template compiler.
 *              Requires expat.
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
#include <expat.h>
#include <picogui/types.h>
#include <picogui/wt.h>

/* Install Expat handlers on the parser */
void wtInstallHandlers(wtParser p) {
  XML_SetElementHandler(p, (XML_StartElementHandler) wtStartElementHandler,
			(XML_EndElementHandler) wtEndElementHandler);
  XML_SetCharacterDataHandler(p, (XML_CharacterDataHandler) wtCharacterDataHandler);
}

void wtStartElementHandler(struct wtparse_data *d, char *name, char *atts) {
  printf("Element %s start\n",name);
}

void wtEndElementHandler(struct wtparse_data *d, char *name){ 
  printf("Element %s end\n",name);
}

void wtCharacterDataHandler(struct wtparse_data *d, char *s, int len) {
}

/* The End */
