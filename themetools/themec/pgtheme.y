%{
/* $Id: pgtheme.y,v 1.3 2000/09/25 00:15:26 micahjd Exp $
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
    unsigned long data;
    unsigned short loader;
  } propval;
}

%token <num>     NUMBER
%token <propid>  PROPERTY
%token <thobjid> THOBJ
%token STRING

%left '-' '+'
%left '*' '/'

%type <num>      constexp
%type <propval>  propertyval
%type <propval>  statement

%%
statementlist: statement
             | statementlist statement
             ;

statement:  PROPERTY '=' propertyval ';' { printf("=%d\n",$3); }
         ;

propertyval:  constexp
           |  statement
           ;

constexp: constexp '+' constexp { $$ = $1 + $3; }
        | constexp '-' constexp { $$ = $1 - $3; }
        | constexp '*' constexp { $$ = $1 * $3; }
        | constexp '/' constexp { 
	  if ($3 == 0)
	    yyerror("Divide by zero");
	  else
	    $$ = $1 / $3; }
        | '(' constexp ')' { $$ = $2; }
        | NUMBER
        ;
