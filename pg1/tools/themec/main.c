/* $Id$
 *
 * main.c - main() and some parser utility functions for
 *          the PicoGUI theme compiler.  The actual parsing
 *          is done in the lex and yacc files pgtheme.l and
 *          pgtheme.y
 *
 * MAGIC compiler
 * Magic Algorithm for General Interface Configurability
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

#include <unistd.h>

#include "themec.h"
#include "pgtheme.h"

int lineno = 1;
int errors;
struct loadernode *loaderlist;
struct objectnode *objectlist;
unsigned long num_tags;
unsigned long num_thobj;
unsigned long num_totprop;
unsigned long datasz_loader;
unsigned long datasz_tags;
char *filename = "stdin";
char *fsvartab[FS_MAX_LOCALS];
int fsvartab_pos;
struct symnode *symtab_head = symboltab;

int main(int argc, char **argv) {
  int quiet = 0, testrun = 0;
  char *outfile = NULL;
  FILE *out;
  int c;

  /**** Command line */

  while (1) {
    c = getopt(argc,argv,"hqto:");
    if (c==-1)
      break;
    
    switch (c) {
      
    case 'q':
      quiet = 1;
      break;
      
    case 't':
      testrun = 1;
      break;
      
    case 'o':
      outfile = strdup(optarg);
      break;
      
    case '?':        /* Need help */
    case 'h':
      puts(" PicoGUI \"Magic\" theme compiler (http://picogui.org)\n"
           " Magic Algorithm for General Interface Configurability\n"
           "\n"
	   "usage: themec [-q] [-t] [-o thfile] [thsfile]\n\n"
	   "  q         : Quiet, suppress theme statistics\n"
	   "  t         : Test run, parse and output stats but don't write file\n"
	   "  o thfile  : specify a name for the compiled theme (*.th)\n"
	   "              - defaults to input file with a .th extension\n"
	   "              - if input is stdin, defaults to stdout\n"
	   "  thsfile   : specify the theme source file (*.ths)\n"
	   "              - defaults to stdin");
      exit(1);
    }
  }

  /* Any leftover argument - the theme file? */
  if (optind<argc && argv[optind]) {
    filename = argv[optind];
    
    /* Open it */
    yyin = fopen(filename = argv[1],"r");
    if (!yyin) {
      perror("Error opening input file");
      return 2;
    }

    if (!outfile) {
      char *p;

      /* Make an output file name from the input file.
	 Change the .ths extension to .th */
      
      outfile = malloc(strlen(filename)+5);
      strcpy(outfile,filename);
      p = strstr(outfile,".ths");
      if (p) *p = 0;
      strcat(outfile,".th");
    }
  }

  /* Open the output file */
  if (!testrun && outfile) {
    out = fopen(outfile,"wb");
    if (!out) {
      perror("Error opening output file");
      return 3;
    }
  }
  else {
    out = stdout;
    outfile = "stdout";
  }

  /* Little message thing */
  if (!quiet && !testrun)
    fprintf(stderr,"Compiling %s -> %s\n",filename,outfile);

  /*** Initialization */

  /* Local variable table */
  fsvartab[0] = "x";
  fsvartab[1] = "y";
  fsvartab[2] = "w";
  fsvartab[3] = "h";
  fsvartab_pos = FS_NUMPARAMS;

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

  if (!quiet)
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

  if (!testrun)
    fwrite(themeheap,themeheap_size,1,out);

  return 0;
}

/* Symbol table lookup, optionally putting the symbol's
   value in *value.  The symbol's type is returned. */
int symlookup(const char *sym,unsigned long *value) {
  struct symnode *n = symtab_head;

  while (n) {
    if (!strcmp(sym,n->name)) {
      if (value) *value = n->value;
      return n->type;
    }
    n = n->next;
  }
  if (value) *value = (unsigned long) strdup(sym);
  return UNKNOWNSYM;
}

/* Concatenate two fsnode lists */
struct fsnode *fsnodecat(struct fsnode *a,struct fsnode *b) {
  struct fsnode *p = a;
  if (!p) return b;
  while (p->next) p = p->next;
  p->next = b;
  return a;  
}

/* Allocate an fsnode */
struct fsnode *fsnewnode(unsigned char op) {
  struct fsnode *n;
  n = malloc(sizeof(struct fsnode));
  if (n) {
    memset(n,0,sizeof(struct fsnode));
    n->op = op;
  }
  else
    yyerror("memory allocation error");
  return n;
}

/* Allocate a loadernode for a chunk of data */
struct loadernode *newloader(unsigned char *data,unsigned long len) {
  struct loadernode *n;
  n = malloc(sizeof(struct loadernode));
  if (n) {
    memset(n,0,sizeof(struct loadernode));
    n->data = data;
    n->datalen = len;
    n->next = loaderlist;

    datasz_loader += len;
     
    /* Account for word boundary padding */
    if (len & 3)
       datasz_loader += 4 - (len & 3);
     
    loaderlist = n;
  }
  else
    yyerror("memory allocation error");

  return n;
}


/* The End */
