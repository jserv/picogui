/* $Id$
 *
 * themec.h - definitions used internally in the theme compiler
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

#ifndef _H_THEMEC
#define _H_THEMEC

#include <string.h>
#include <stdio.h>

#include <picogui/types.h>
#include <picogui/constants.h>
#include <picogui/theme.h>
#include <picogui/network.h>

#define MAXERRORS  20

/* Parser globals */
extern int lineno;
extern int errors;
extern char *yytext;
extern char *filename;
extern FILE *yyin;

/* An entry in the symbol table */
struct symnode {
  int type;
  const char *name;
  unsigned long value;
  struct symnode *next;
};

/* Static symbol table */
extern struct symnode symboltab[];

/* Head pointer for symbol table */
extern struct symnode *symtab_head;

/* Because the interpreter itself places strict
   limitations on the stack space, doesn't make
   sense using a dynamic variable table */
#define FS_MAX_LOCALS   64    /* Server's limit is probably much smaller */

/* fillstyle local variable table */
extern char *fsvartab[FS_MAX_LOCALS];
extern int fsvartab_pos;
#define FS_NUMPARAMS  4 /* Number of parameter variables (non-allocated) */

/*** Structures for the in-memory representation ***/

struct loadernode;

struct propnode {
  unsigned long data;
  unsigned long loader;
  unsigned short propid;
  unsigned long *link_from;  /* If non-null, this location in the
				theme heap is set to this property's
				offset from the beginning of the heap */
  struct loadernode *ldnode;
  struct propnode *next;
};

struct objectnode {
  unsigned short id;
  struct propnode *proplist;
  unsigned long num_prop;
  struct objectnode *next;
};

/* Opcode(s) in a fill style */
struct fsnode {
  unsigned char op;
  unsigned long param;
  unsigned long param2;
  struct fsnode *next;
};

/* A single constant in a list of constants */
struct constnode {
  unsigned long data;
  struct constnode *next;
};

/* Linked list of loaders to link */
struct loadernode {
  unsigned char *data;
  unsigned long datalen;
  unsigned long *link_from;
  struct loadernode *next;
};

extern struct loadernode *loaderlist;
extern struct objectnode *objectlist;
extern unsigned long num_tags;
extern unsigned long num_thobj;
extern unsigned long num_totprop;

extern unsigned long datasz_loader;
extern unsigned long datasz_tags;

/*** Structures for the compiled code ***/

/* The theme heap */

extern unsigned long themeheap_size;
extern unsigned char *themeheap;
extern unsigned char *themeheap_p;   /* Current position */

/* Important structures in the theme heap */
extern struct pgtheme_header *themehdr;
extern struct pgtheme_thobj  *thobjarray;

/*** Parser utility functions ***/

int yyerror(const char *s);   /* Error reporting */
int symlookup(const char *sym,unsigned long *value);
struct fsnode *fsnodecat(struct fsnode *a,struct fsnode *b);
struct fsnode *fsnewnode(unsigned char op);
struct loadernode *newloader(unsigned char *data,unsigned long len);

/*** Important functions ***/

void backend(void);

#endif /* _H_THEMEC */
/* The End */
