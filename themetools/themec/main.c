/* $Id: main.c,v 1.6 2000/10/07 05:49:30 micahjd Exp $
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
int errors;
struct objectnode *objectlist;
unsigned long num_tags;
unsigned long num_thobj;
unsigned long num_totprop;
unsigned long datasz_loader;
unsigned long datasz_tags;
char *filename;

int main(int argc, char **argv) {

  /**** Command line */
  /* I will put in a nice getopt-based arg processor later */

  filename = "stdin";
  if (argc==2) {
    yyin = fopen(filename = argv[1],"r");
    if (!yyin) {
      perror("Error opening file");
      return 2;
    }
  }

  /**** Front end (Scanner/parser) */
  /* Parse the input file, generating linked lists */

  do {
    yyparse();
  } while (!feof(yyin));
  if (errors) return 1;

  /**** Back end */
  /* Process the parsed data */

  backend();
  
  /**** Summary info */

  fprintf(stderr,
	  "Generated theme. Summary:\n"
	  "\t    Objects: %d\n"
	  "\t Properties: %d\n"
	  "\t       Tags: %d\n"
	  "\t   Tag Data: %d\n"
	  "\tLoader Data: %d\n"
	  "\t Total size: %d\n",
	  num_thobj,num_totprop,num_tags,datasz_tags,
	  datasz_loader,themeheap_size);

  /**** Output */
  /* Write it to a file */

  fwrite(themeheap,themeheap_size,1,stdout);

  return 0;
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
  return UNKNOWNSYM;
}

/* The End */
