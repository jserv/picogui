/* $Id: themec.h,v 1.5 2000/09/25 19:41:19 micahjd Exp $
 *
 * themec.h - definitions used internally in the theme compiler
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

#ifndef _H_THEMEC
#define _H_THEMEC

#include <string.h>
#include <stdio.h>

#include <picogui/constants.h>
#include <picogui/theme.h>

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
};

extern struct symnode symboltab[];

/*** Structures for the in-memory representation ***/

struct propnode {
  unsigned long data;
  unsigned long loader;
  unsigned short propid;
  struct propnode *next;
};

struct objectnode {
  unsigned short id;
  struct propnode *proplist;
  struct objectnode *next;
};

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

/*** Important functions ***/

void backend(void);

#endif /* _H_THEMEC */
/* The End */
