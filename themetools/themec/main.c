/* $Id: main.c,v 1.1 2000/09/25 00:50:48 micahjd Exp $
 *
 * main.c - main() and some parser utility functions for
 *          the PicoGUI theme compiler.  The actual parsing
 *          is done in the lex and yacc files pgtheme.l and
 *          pgtheme.y
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

#include "themec.h"

int lineno = 1;
int errors = 0;

extern FILE *yyin;

int main(int argc, char **argv) {

  if (argc==2) {
    yyin = fopen(argv[1],"r");
    if (!yyin) {
      perror("Error opening file");
      return 2;
    }
  }

  do {
    yyparse();
  } while (!feof(yyin));
  return (errors ? 1 : 0);
}

int yyerror(const char *s) {
  fprintf(stderr,"Error on line %d: %s\n",lineno,s);
  errors++;
  return 1;
}

/* The End */
