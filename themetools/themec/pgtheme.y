%{
/* $Id: pgtheme.y,v 1.8 2000/10/07 05:49:30 micahjd Exp $
 *
 * pgtheme.y - yacc grammar for processing PicoGUI theme source code
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

%}

%union {
  unsigned long num;
  unsigned short propid;
  unsigned short thobjid;
  struct {
    unsigned long loader;
    unsigned long propid;
    unsigned long data;
  } propval;
  struct propnode *prop;
  struct objectnode *obj;
}

   /* Data types */
%token <num>     NUMBER
%token <propid>  PROPERTY
%token <thobjid> THOBJ
%token STRING 

%type <num>      constexp
%type <propval>  propertyval
%type <thobjid>  thobj
%type <propid>   property
%type <prop>     statement
%type <prop>     stmt_list
%type <prop>     compount_stmt
%type <obj>      objectdef
%type <propval>  fillstyle

   /* Reserved words */
%token UNKNOWNSYM OBJ FILLSTYLE VAR FSVAR FSFUNC

%left VARPLUS
%left VARMULT
%nonassoc VARPAREN
%left '-' '+'
%left '*' '/'
%nonassoc CONSTPAREN

%start unitlist

%%

unitlist: unit
        | unitlist unit
        ;

   /* This is a list of the structures that can appear
      unenclosed in the file */
unit: objectdef
    | ';'        /* No real purpose but to satisfy people that
		    insist on ending object definitions with a ';' */
    ;

objectdef:  OBJ thobj compount_stmt      { 
  /* Add to the list of objects */
  $$ = malloc(sizeof(struct propnode));
  if ($$) {
    memset($$,0,sizeof(struct propnode));
    $$->proplist = $3;
    $$->id       = $2;
    $$->next     = objectlist;
    objectlist   = $$;
    num_thobj++;
  }
  else
    yyerror("memory allocation error");  
}
         ;

compount_stmt:  statement                { $$ = $1; }
             |  '{' stmt_list '}'        { $$ = $2; }
             ;

statement:  property '=' propertyval ';' { 
  $$ = malloc(sizeof(struct propnode));
  if ($$) {
    memset($$,0,sizeof(struct propnode));
    $$->loader = $3.loader;
    $$->data   = $3.data;
    $$->propid = $1;
    num_totprop++;
  }
  else
    yyerror("memory allocation error");
}
         |  ';'         { $$ = NULL; }
	 |  error ';'   { $$ = NULL; }
	 |  error '}'   { $$ = NULL; }
         ;

    /* stmt_list makes a linked list of statements */ 
stmt_list: statement               { $$ = $1; }
         | stmt_list statement     { 
  if ($2) {
    $2->next = $1; 
    $$ = $2; 
  }
  else       /* This handles skipping invalid statements
		for error recovery (nice error messages
		instead of a segfault ;-) */
    $$ = $1;
}
         ;

thobj: THOBJ          { $$ = $1; }
     | PROPERTY       { yyerror("Property found in place of theme object"); } 
     | UNKNOWNSYM     { $$ = 0; }
     ;

property: PROPERTY
        | THOBJ       { yyerror("Theme object found in place of property"); }
        | UNKNOWNSYM  { $$ = 0; }
        ;

propertyval:  constexp          { $$.data = $1; $$.loader = PGTH_LOAD_NONE; }
           |  fillstyle         { $$ = $1; }
           ;

constexp: constexp '+' constexp { $$ = $1 + $3; }
        | constexp '-' constexp { $$ = $1 - $3; }
        | constexp '*' constexp { $$ = $1 * $3; }
        | constexp '/' constexp {
	  if ($3 == 0)
	    yyerror("Divide by zero");
	  else
	    $$ = $1 / $3; }
        | '(' constexp ')' { $$ = $2; }  %prec CONSTPAREN
        | NUMBER
	//	| UNKNOWNSYM            { $$ = 0; }
        ;

    /* Fill-style things */

fillstyle: FILLSTYLE  { yyerror("fillstyle requires parameters"); }
         | FILLSTYLE '{' '}' { yyerror("empty fillstyle"); }
         | FILLSTYLE '{' fsbody '}' {
  $$.data = 0;
  $$.loader = 0;
}
         ;

fsbody: fsdecl_list fsstmt_list
      | fsstmt_list
      | fsdecl_list   { yyerror("fillstyle has no statements"); }
      ;

fsdecl_list: fsdecl
           | fsdecl_list fsdecl
           ;

fsdecl: VAR fsvar_list ';'
      ;

fsvar_list: UNKNOWNSYM
          | fsvar_list ',' UNKNOWNSYM
          ;

fsstmt_list: fsstmt
           | fsstmt_list fsstmt
	   ;

fsstmt: FSVAR '=' fsexp ';'
      | FSFUNC '(' fsarglist ')' ';'
      ;

fsarglist:
         | fsarglist ',' fsexp
         ;

fsexp: '(' fsexp ')'    %prec VARPAREN
     | fsexp '+' fsexp  %prec VARPLUS
     | fsexp '*' fsexp  %prec VARMULT
     | NUMBER {printf("DEBUG(%d)\n",$1);}
     ;

%%

/* Generic error reporting function for parsing errors */
int yyerror(const char *s) {
  if (YYRECOVERING)
    return;

  if (errors >= MAXERRORS) {
    fprintf(stderr,"Too many errors!\n");
    exit(1);
  }

  fprintf(stderr,"Error on %s:%d: %s",filename,lineno,s);
  if (yytext[0])
    fprintf(stderr," at \"%s\"\n",yytext);
  else
    fprintf(stderr,"\n");

  errors++;
  return 1;
}

/* The End */
