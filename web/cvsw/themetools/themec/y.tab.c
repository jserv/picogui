#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "pgtheme.y"
/* $Id: y.tab.c,v 1.1 2000/11/06 00:31:37 micahjd Exp $
 *
 * pgtheme.y - yacc grammar for processing PicoGUI theme source code
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

#line 33 "pgtheme.y"
typedef union {
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
} YYSTYPE;
#line 55 "y.tab.c"
#define NUMBER 257
#define PROPERTY 258
#define THOBJ 259
#define STRING 260
#define UNKNOWNSYM 261
#define OBJ 262
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    9,    9,    8,    7,    7,    5,    5,    5,
    5,    6,    6,    3,    3,    3,    4,    4,    4,    2,
    1,    1,    1,    1,    1,    1,    1,
};
short yylen[] = {                                         2,
    1,    2,    1,    1,    3,    1,    3,    4,    1,    2,
    2,    1,    2,    1,    1,    1,    1,    1,    1,    1,
    3,    3,    3,    3,    3,    1,    1,
};
short yydefred[] = {                                      0,
    0,    4,    0,    3,    1,   15,   14,   16,    0,    2,
    0,   17,   18,   19,    9,    0,    0,    6,    5,   10,
   11,   12,    0,    0,    7,   13,   26,   27,    0,    0,
    0,    0,    0,    0,    0,    0,    8,   25,    0,    0,
   23,   24,
};
short yydgoto[] = {                                       3,
   30,   31,    9,   17,   18,   23,   19,    4,    5,
};
short yysindex[] = {                                    -58,
 -222,    0,  -58,    0,    0,    0,    0,    0,  -56,    0,
  -57,    0,    0,    0,    0,  -46,  -54,    0,    0,    0,
    0,    0,  -50,  -40,    0,    0,    0,    0,  -40,  -22,
  -35,  -12,  -40,  -40,  -40,  -40,    0,    0,  -15,  -15,
    0,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  -25,
    0,    0,    0,    0,    0,    0,    0,    0,  -37,  -31,
    0,    0,
};
short yygindex[] = {                                      0,
  -18,    0,    0,    0,    3,    0,    0,    0,    2,
};
#define YYTABLESIZE 221
short yytable[] = {                                      29,
    2,   20,   15,   22,   10,   22,   24,   22,   15,   21,
   32,   21,   15,   21,   39,   40,   41,   42,   22,   35,
   34,   22,   33,   37,   36,   26,   35,   21,   38,   35,
   34,   36,   33,   20,   36,    6,    7,    0,    8,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   16,   21,    0,    0,
    0,    0,    0,    0,   25,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   11,
    0,   12,   13,    1,   14,   11,    0,   12,   13,   11,
   14,   12,   13,    0,   14,    0,   27,    0,    0,    0,
   28,
};
short yycheck[] = {                                      40,
   59,   59,   59,   41,    3,   43,   61,   45,   59,   41,
   29,   43,   59,   45,   33,   34,   35,   36,   16,   42,
   43,   59,   45,   59,   47,   23,   42,   59,   41,   42,
   43,   47,   45,   59,   47,  258,  259,   -1,  261,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  123,  125,   -1,   -1,
   -1,   -1,   -1,   -1,  125,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  256,
   -1,  258,  259,  262,  261,  256,   -1,  258,  259,  256,
  261,  258,  259,   -1,  261,   -1,  257,   -1,   -1,   -1,
  261,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 262
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'('","')'","'*'","'+'",0,"'-'",0,"'/'",0,0,0,0,0,0,0,0,0,0,0,"';'",
0,"'='",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,"NUMBER","PROPERTY","THOBJ","STRING","UNKNOWNSYM","OBJ",
};
char *yyrule[] = {
"$accept : unitlist",
"unitlist : unit",
"unitlist : unitlist unit",
"unit : objectdef",
"unit : ';'",
"objectdef : OBJ thobj compount_stmt",
"compount_stmt : statement",
"compount_stmt : '{' stmt_list '}'",
"statement : property '=' propertyval ';'",
"statement : ';'",
"statement : error ';'",
"statement : error '}'",
"stmt_list : statement",
"stmt_list : stmt_list statement",
"thobj : THOBJ",
"thobj : PROPERTY",
"thobj : UNKNOWNSYM",
"property : PROPERTY",
"property : THOBJ",
"property : UNKNOWNSYM",
"propertyval : constexp",
"constexp : constexp '+' constexp",
"constexp : constexp '-' constexp",
"constexp : constexp '*' constexp",
"constexp : constexp '/' constexp",
"constexp : '(' constexp ')'",
"constexp : NUMBER",
"constexp : UNKNOWNSYM",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#line 161 "pgtheme.y"

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
#line 243 "y.tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 5:
#line 83 "pgtheme.y"
{ 
  /* Add to the list of objects */
  yyval.obj = malloc(sizeof(struct propnode));
  if (yyval.obj) {
    memset(yyval.obj,0,sizeof(struct propnode));
    yyval.obj->proplist = yyvsp[0].prop;
    yyval.obj->id       = yyvsp[-1].thobjid;
    yyval.obj->next     = objectlist;
    objectlist   = yyval.obj;
    num_thobj++;
  }
  else
    yyerror("memory allocation error");  
}
break;
case 6:
#line 99 "pgtheme.y"
{ yyval.prop = yyvsp[0].prop; }
break;
case 7:
#line 100 "pgtheme.y"
{ yyval.prop = yyvsp[-1].prop; }
break;
case 8:
#line 103 "pgtheme.y"
{ 
  yyval.prop = malloc(sizeof(struct propnode));
  if (yyval.prop) {
    memset(yyval.prop,0,sizeof(struct propnode));
    yyval.prop->loader = yyvsp[-1].propval.loader;
    yyval.prop->data   = yyvsp[-1].propval.data;
    yyval.prop->propid = yyvsp[-3].propid;
    num_totprop++;
  }
  else
    yyerror("memory allocation error");
}
break;
case 9:
#line 115 "pgtheme.y"
{ yyval.prop = NULL; }
break;
case 10:
#line 116 "pgtheme.y"
{ yyval.prop = NULL; }
break;
case 11:
#line 117 "pgtheme.y"
{ yyval.prop = NULL; }
break;
case 12:
#line 121 "pgtheme.y"
{ yyval.prop = yyvsp[0].prop; }
break;
case 13:
#line 122 "pgtheme.y"
{ 
  if (yyvsp[0].prop) {
    yyvsp[0].prop->next = yyvsp[-1].prop; 
    yyval.prop = yyvsp[0].prop; 
  }
  else       /* This handles skipping invalid statements
		for error recovery (nice error messages
		instead of a segfault ;-) */
    yyval.prop = yyvsp[-1].prop;
}
break;
case 14:
#line 134 "pgtheme.y"
{ yyval.thobjid = yyvsp[0].thobjid; }
break;
case 15:
#line 135 "pgtheme.y"
{ yyerror("Property found in place of theme object"); }
break;
case 16:
#line 136 "pgtheme.y"
{ yyval.thobjid = 0; }
break;
case 18:
#line 140 "pgtheme.y"
{ yyerror("Theme object found in place of property"); }
break;
case 19:
#line 141 "pgtheme.y"
{ yyval.propid = 0; }
break;
case 20:
#line 144 "pgtheme.y"
{ yyval.propval.data = yyvsp[0].num; yyval.propval.loader = PGTH_LOAD_NONE; }
break;
case 21:
#line 147 "pgtheme.y"
{ yyval.num = yyvsp[-2].num + yyvsp[0].num; }
break;
case 22:
#line 148 "pgtheme.y"
{ yyval.num = yyvsp[-2].num - yyvsp[0].num; }
break;
case 23:
#line 149 "pgtheme.y"
{ yyval.num = yyvsp[-2].num * yyvsp[0].num; }
break;
case 24:
#line 150 "pgtheme.y"
{ 
	  if (yyvsp[0].num == 0)
	    yyerror("Divide by zero");
	  else
	    yyval.num = yyvsp[-2].num / yyvsp[0].num; }
break;
case 25:
#line 155 "pgtheme.y"
{ yyval.num = yyvsp[-1].num; }
break;
case 27:
#line 157 "pgtheme.y"
{ yyval.num = 0; }
break;
#line 505 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
