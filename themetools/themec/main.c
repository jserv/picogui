/* $Id: main.c,v 1.3 2000/09/25 06:19:28 micahjd Exp $
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
#include "y.tab.h"

int lineno = 1;
int errors = 0;

char *filename;

int main(int argc, char **argv) {

  /* I will put in a nice getopt-based arg processor later */

  filename = "stdin";
  if (argc==2) {
    yyin = fopen(filename = argv[1],"r");
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

/* Symbol table lookup, optionally putting the symbol's
   value in *value.  The symbol's type is returned. */
int symlookup(const char *sym,unsigned long *value) {
  struct symnode *n = symboltab;

  while (n->name) {
    if (!strcmp(sym,n->name)) {
      if (value) *value = n->value;
      return n->type;
    }
    n++;
  }
  
  yyerror("Unrecognized symbol");
  return UNKNOWNSYM;
}

/* This is called when a completed theme object
   definition is read, including a linked list of
   statement nodes */
void add_objectdef(unsigned long thobj,struct propnode *props) {
  printf("Theme object %d:\n",thobj);
  while (props) {
    printf("\t property %d = 0x%08X with loader %d\n",props->propid,
	   props->data,props->loader);
    props = props->next;
  }
}

/* The End */
