%{
/* $Id: pgtheme.l,v 1.1 2000/09/24 03:14:59 micahjd Exp $
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

#include "y.tab.h"
extern int yylval;
%}

%%
[ \t]           ;     /* ignore whitespace */
0x[0-9A-Fa-f]+  { yylval = strtol(yytext,(char**)NULL,16); return NUMBER; }
0[0-9]+         { yylval = strtol(yytext,(char**)NULL,8); return NUMBER; }
[0-9]+          { yylval = strtol(yytext,(char**)NULL,10); return NUMBER; }
=               { return EQUALS; }
\{              { return BEGINBLOCK; }
\}              { return ENDBLOCK; }
.               { yyerror("Unrecognized token"); } 
%%
