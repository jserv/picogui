%{
/* $Id: pgtheme.l,v 1.2 2000/09/24 06:39:59 micahjd Exp $
 *
 * pgtheme.l - lexer for processing PicoGUI theme source code
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

#include <string.h>
#include <stdio.h>

#include "y.tab.h"
 
int lineno = 1;
int errors = 0;

%}

%%
[ \t]           ;     /* ignore whitespace */
\n              { lineno++; }
0               { yylval.num = 0; return NUMBER; } 
0x[0-9A-Fa-f]+  { yylval.num = strtol(yytext,(char**)NULL,16); return NUMBER; }
0[0-9]+         { yylval.num = strtol(yytext,(char**)NULL,8); return NUMBER; }
[1-9][0-9]*     { yylval.num = strtol(yytext,(char**)NULL,10); return NUMBER; }
[a-zA-Z]        { return PROPERTY; }
\;              { return 0; }
.               { return yytext[0]; }
%%

extern FILE *yyin;

int main(void) {
  do {
    yyparse();
  } while (!feof(yyin));
  return errors;
}

int yyerror(const char *s) {
  fprintf(stderr,"Error on line %d: %s\n",lineno,s);
  errors++;
  return 1;
}

/* The End */
