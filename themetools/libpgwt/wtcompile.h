/* $Id: wtcompile.h,v 1.1 2002/04/07 06:37:49 micahjd Exp $
 *
 * wtcompile.h - Interface to libpgwt Widget Template compiler library
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

/* Node in a linked list of handle table entries */
struct wt_htable_node {
  int number;      /* Number in the handle table, -1 for unassigned */
  int refcount;    /* Number of times referred to */
  struct htable_node *next;
};

/* Node in a list of requests */
struct wt_request_node {
  
};

/* Data used by the parser */
struct wtparse_data {
  
};

typedef void * wtParser;

/********************************************* High-level APIs */

/* Create a Widget Template parser */
wtParser wtNewParser(void);

/* Parse a block of data, returns an error string
 * if there's an error, NULL if not.
 *
 * isFinal must be 1 for the last block parsed.
 */
const char *wtParse(wtParser wtp, const char *s, int len, int isFinal);

/* Finish parsing, and return the compiled template,
 * returns an error or NULL
 */
const char *wtFinishParser(wtParser wtp, void **template, int *template_len);

/********************************************* Internals */

/* Install Expat handlers on the parser */
void wtInstallHandlers(wtParser p);

/* Given a parser data structure, allocates the compiled template
 * buffer then fills it in. Returns an error message or NULL.
 */
const char *wtBuildTemplate(struct wtparse_data *d, void **template, int *template_len);

/* Handlers for the XML stream
 */
void wtStartElementHandler(struct wtparse_data *d, char *name, char *atts);
void wtEndElementHandler(struct wtparse_data *d, char *name);
void wtCharacterDataHandler(struct wtparse_data *d, char *s, int len);

/* The End */
