%{
/* $Id$
 *
 * pgtheme.l - lexer for processing PicoGUI theme source code
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

#include "themec.h"
#include "pgtheme.h"

/* This isn't necessary on Linux, but some other Unixes (like Darwin)
 * seem to need this.
 */
#undef yywrap
int yywrap(void) {
 return 1;
}

%}

%x COMMENT

%%

   /* reserved words */

obj(ect)?       { return OBJ; }
prop(erty)?     { return PROP; }
fillstyle       { return FILLSTYLE; }
var             { return VAR; }
ColorAdd        { return COLORADD; }
ColorSub        { return COLORSUB; }
ColorDiv        { return COLORDIV; }
ColorMult       { return COLORMULT; }
FindThemeObject { return FINDTHEMEOBJECT; }
LoadBitmap      { return LOADBITMAP; }
Copy            { return COPY; }
Font            { return FONT; }
Array           { return ARRAY; }
widget          { return WIDGET; }
"NULL"          { yylval.num = 0; return STRING; }
container       { return CONTAINER; }
next            { return NEXT; }
previous        { return PREVIOUS; }
app             { return APP; }
child           { return CHILD; }

   /* symbols, operators, and strings */

"<<"            { return SHIFTL; }
">>"            { return SHIFTR; }
"::"            { return CLASS; }
"=="            { return EQUAL; }
"<="            { return LTEQ; }
">="            { return GTEQ; }
"!="            { return NOTEQ; }
"&&"            { return AND; }
"||"            { return OR; }
"->"            { return ARROW; }

[-+*/()=;{},\&\|:?<>!\[\]]        { return yytext[0]; }

[a-zA-Z_][a-zA-Z0-9_\.]* { 
  int i;

  /* First try looking in the fillstyle variable table */
  for (i=0;i<fsvartab_pos;i++)
    if (!strcmp(yytext,fsvartab[i])) {
      yylval.num = i;
      return FSVAR;
    }

  /* Next the constant symbol table */
  return symlookup(yytext,&yylval.num);
}

\"([^\"\n\\]*(\\.)*)+[\"\n] { 
  char *a,*b;

  if (!(yylval.str = malloc(yyleng-1)))
    yyerror("allocation error");
  
  a = yytext+1;
  b = yylval.str;
  while ((*a) && (*a)!='\"') {
    if ((*a)=='\\') {
      a++;
      /* This handles all the single-character escapes, but
	 not octal or hex codes. */
      switch (*a) {
      case 'n': *b = '\n'; break;
      case 't': *b = '\t'; break;
      case 'v': *b = '\v'; break;
      case 'b': *b = '\b'; break;
      case 'r': *b = '\r'; break;
      case 'f': *b = '\f'; break;
      case 'a': *b = '\a'; break;
      default:  *b = *a;
      }
    }
    else
      *b = *a;
    a++; b++;
  }
  if (!*a) 
    yyerror("unterminated string");
  *b = 0;

  return STRING;
}

   /* numeric constants */

0               { yylval.num = 0; return NUMBER; } 
0x[0-9A-Fa-f]+  { yylval.num = strtol(yytext,(char**)NULL,16); return NUMBER; }
0[0-9]+         { yylval.num = strtol(yytext,(char**)NULL,8); return NUMBER; }
[1-9][0-9]*     { yylval.num = strtol(yytext,(char**)NULL,10); return NUMBER; }
\'\\?[^\'\n]\'  {
   if (yytext[1]=='\\')
     switch (yytext[2]) {
      case 'n': yylval.num = '\n'; break;
      case 't': yylval.num = '\t'; break;
      case 'v': yylval.num = '\v'; break;
      case 'b': yylval.num = '\b'; break;
      case 'r': yylval.num = '\r'; break;
      case 'f': yylval.num = '\f'; break;
      case 'a': yylval.num = '\a'; break;
      default:  yylval.num = yytext[2];
     }
   else
     yylval.num = yytext[1];
   return NUMBER;
}
	
   /* whitespace and comments */

\n              { lineno++; }
[ \t\r]+        ;     /* ignore whitespace */
"#".*           ;     /* script-style comment */
"//".*          ;     /* C++ comment */
"/*"            { BEGIN COMMENT; } /* C comment */
<COMMENT>\n     { lineno++; }
<COMMENT>"*/"   { BEGIN 0; }
<COMMENT>.      ;

.               { yyerror("invalid character"); }

