%{
/* $Id: pgtheme.y,v 1.13 2000/10/08 06:13:43 micahjd Exp $
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

#include <malloc.h>
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
    struct loadernode *ldnode;
  } propval;
  struct propnode *prop;
  struct objectnode *obj;
  struct fsnode *fsn;
  char *str;
}

   /* Data types */
%token <num>     NUMBER
%token <propid>  PROPERTY
%token <thobjid> THOBJ
%token <num>     FSVAR
%token <num>     FSFUNC
%token <str>     STRING 
%token <str>     UNKNOWNSYM

%type <num>      constexp
%type <propval>  propertyval
%type <thobjid>  thobj
%type <propid>   property
%type <prop>     statement
%type <prop>     stmt_list
%type <prop>     compount_stmt
%type <obj>      objectdef
%type <propval>  fillstyle
%type <fsn>      fsexp
%type <fsn>      fsarglist
%type <fsn>      fsstmt
%type <fsn>      fsstmt_list
%type <fsn>      fsexp
%type <fsn>      fsvar_list
%type <fsn>      fsdecl
%type <fsn>      fsdecl_list
%type <fsn>      fsbody
%type <fsn>      fsvar

   /* Reserved words */
%token OBJ FILLSTYLE VAR SHIFTR SHIFTL

%nonassoc CONSTPROMOTE

%left '|'
%left '&'
%left SHIFTL SHIFTR
%left '-' '+'
%left '*' '/'
%nonassoc CONSTPAREN

    /* Reduces the stack space used by fillstyles */
%left VAROR
%left VARAND
%left VARSHIFT
%left VARPLUS
%left VARMULT
%nonassoc VARPAREN

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
    $$->ldnode = $3.ldnode;
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

propertyval:  constexp          { $$.data = $1; $$.loader = PGTH_LOAD_NONE; $$.ldnode = NULL;}
           |  fillstyle         { $$ = $1; }
           |  STRING {
  unsigned char *buf;
  struct pgrequest *req;

  /* Allocate the buffer */
  if (!(buf = malloc(sizeof(struct pgrequest)+strlen($1))))
    yyerror("memory allocation error");

  /* Reserve space for the request header */
  req = (struct pgrequest *) buf;
  memset(req,0,sizeof(struct pgrequest));
  req->type = htons(PGREQ_MKSTRING);
  req->size = htonl(strlen($1));

  /* copy string and discard original */
  memcpy(buf+sizeof(struct pgrequest),$1,strlen($1));
  free($1);

  $$.ldnode = newloader(buf,(sizeof(struct pgrequest)+strlen($1)));
  $$.loader = PGTH_LOAD_REQUEST;
}
           ;

constexp: constexp '+' constexp { $$ = $1 + $3; }
        | constexp '-' constexp { $$ = $1 - $3; }
        | constexp '*' constexp { $$ = $1 * $3; }
        | constexp '&' constexp { $$ = $1 & $3; }
        | constexp '|' constexp { $$ = $1 | $3; }
        | constexp SHIFTL constexp { $$ = $1 << $3; }
        | constexp SHIFTR constexp { $$ = $1 >> $3; }
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
  struct fsnode *p = $3;
  unsigned char *buf,*bp;
  unsigned long bufsize=512;
  struct pgrequest *req;
  long t;

  /* Allocate the fillstyle buffer */
  if (!(bp = buf = malloc(bufsize)))
    yyerror("memory allocation error");

  /* Reserve space for the request header */
  req = (struct pgrequest *) bp;
  bp += sizeof(struct pgrequest);
  memset(req,0,sizeof(struct pgrequest));
  req->type = htons(PGREQ_MKFILLSTYLE);

  /* Transcribe the fsnodes into real opcodes */
  while (p) {
    
    /* Check buffer size */
    t = bp-buf;
    if ((bufsize-t)<10) {
      buf = realloc(buf,bufsize <<= 1);
      bp = buf+t;
    }      

    /* Pack short values when possible */
    if (p->op == PGTH_OPCMD_LONGLITERAL &&
	p->param < PGTH_OPSIMPLE_LITERAL)
      p->op = PGTH_OPSIMPLE_LITERAL | p->param;
    else if (p->op == PGTH_OPCMD_LONGGROP &&
	     p->param < PGTH_OPSIMPLE_GROP)
      p->op = PGTH_OPSIMPLE_GROP | p->param;
    else if (p->op == PGTH_OPCMD_LONGGET &&
	     p->param < PGTH_OPSIMPLE_GET)
      p->op = PGTH_OPSIMPLE_GET | p->param;
    else if (p->op == PGTH_OPCMD_LONGSET &&
	     p->param < PGTH_OPSIMPLE_GET)
      p->op = PGTH_OPSIMPLE_SET | p->param;
    
    /* Output opcode byte */
    *(bp++) = p->op;
   
    /* Output parameters */
    switch (p->op) {

    case PGTH_OPCMD_LONGLITERAL:      /* 4 byte */
      *(((unsigned long *)bp)++) = htonl(p->param);
      break;

    case PGTH_OPCMD_LONGGROP:         /* 2 byte */
    case PGTH_OPCMD_LOCALPROP:
      *(((unsigned short *)bp)++) = htons(p->param);
      break;

    case PGTH_OPCMD_LONGGET:          /* 1 byte */
    case PGTH_OPCMD_LONGSET:
      *(bp++) = p->param;
      break;
     
    case PGTH_OPCMD_PROPERTY:         /* 2 x 2 byte */
      *(((unsigned short *)bp)++) = htons(p->param);
      *(((unsigned short *)bp)++) = htons(p->param2);
      break;

    default: 
      break;
    }

    p = p->next;
  }

  /* Finish the header */
  req->size = htonl(bp-buf-sizeof(struct pgrequest));

  $$.ldnode = newloader(buf,bp-buf);
  $$.loader = PGTH_LOAD_REQUEST;

  /* Set the variable table up for the next fillstyle */
  fsvartab_pos = FS_NUMPARAMS;
}
         ;

fsbody: fsdecl_list fsstmt_list  { $$ = fsnodecat($1,$2); }
      | fsstmt_list              { $$ = $1; }
      | fsdecl_list              { $$ = NULL; yyerror("fillstyle has no statements"); }
      ;

fsdecl_list: fsdecl              { $$ = $1; } 
           | fsdecl_list fsdecl  { $$ = fsnodecat($1,$2); } 
           ;

fsdecl: VAR fsvar_list ';'       { $$ = $2; }
      ;

fsvar_list: fsvar                { $$ = $1; }
          | fsvar_list ',' fsvar { $$ = fsnodecat($1,$3); }
          ;

fsvar: UNKNOWNSYM                { 
  fsvartab[fsvartab_pos++] = $1;
  $$ = fsnewnode(PGTH_OPCMD_LONGLITERAL);
}
     | UNKNOWNSYM '=' constexp   {
  fsvartab[fsvartab_pos++] = $1;
  $$ = fsnewnode(PGTH_OPCMD_LONGLITERAL);
  $$->param = $3;
}
     ;

fsstmt_list: fsstmt              { $$ = $1; }
           | fsstmt_list fsstmt  { $$ = fsnodecat($1,$2); }
	   ;

fsstmt: FSVAR '=' fsexp ';'          { 
  struct fsnode *n;
  $$ = fsnodecat($3,n = fsnewnode(PGTH_OPCMD_LONGSET));
  n->param = $1;
}
      | FSFUNC '(' fsarglist ')' ';' { 
  struct fsnode *n;
  $$ = fsnodecat($3,n = fsnewnode(PGTH_OPCMD_LONGGROP));
  n->param = $1;
}
      ;

fsarglist:                     { $$ = NULL; }
         | fsexp               { $$ = $1; }
         | fsarglist ',' fsexp { $$ = fsnodecat($1,$3); }
         ;

fsexp: '(' fsexp ')'    { $$ = $2; } %prec VARPAREN 
     | fsexp '+' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_PLUS)); } %prec VARPLUS  
     | fsexp '-' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_MINUS)); } %prec VARPLUS 
     | fsexp '*' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_MULTIPLY)); } %prec VARMULT  
     | fsexp '/' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_DIVIDE)); } %prec VARMULT  
     | fsexp '|' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_OR)); } %prec VAROR  
     | fsexp '&' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_AND)); } %prec VARAND  
     | fsexp SHIFTL fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_SHIFTL)); } %prec VARSHIFT 
     | fsexp SHIFTR fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_SHIFTR)); } %prec VARSHIFT 
     | constexp            { ($$ = fsnewnode(PGTH_OPCMD_LONGLITERAL))->param = $1; }          %prec CONSTPROMOTE     
     | FSVAR            { $$ = fsnewnode(PGTH_OPCMD_LONGGET); $$->param = $1; }
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
