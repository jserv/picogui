%{
/* $Id: pgtheme.l,v 1.3 2000/09/24 07:28:14 micahjd Exp $
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

%x COMMENT

%%

[a-zA-Z_][a-zA-Z0-9]* { return PROPERTY; }
[-+*/()=;]      { return yytext[0]; }
\"[^"\n]*["\n]  { return STRING; } 

   /* numeric constants */

0               { yylval.num = 0; return NUMBER; } 
0x[0-9A-Fa-f]+  { yylval.num = strtol(yytext,(char**)NULL,16); return NUMBER; }
0[0-9]+         { yylval.num = strtol(yytext,(char**)NULL,8); return NUMBER; }
[1-9][0-9]*     { yylval.num = strtol(yytext,(char**)NULL,10); return NUMBER; }

   /* whitespace and comments */

\n              { lineno++; }
[ \t\r]+        ;     /* ignore whitespace */
"#".*           ;     /* script-style comment */
"//".*          ;     /* c++ comment */
"/*"            { BEGIN COMMENT; }
<COMMENT>\n     { lineno++; }
<COMMENT>"*/"   { BEGIN 0; }
<COMMENT>.      ;

.               { yyerror("invalid character"); }

%%

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
