%{
/* $Id$
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

/* FIXME: Check for Mac OS X using autoconf */
#if (defined(__APPLE__) && defined(__MACH__)) // Mac OS X and Darwin 
#include <sys/types.h>
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

#include "themec.h"

%}

%union {
  unsigned long num;
  unsigned long propid;   /* short would be big enough, but this needs to */
  unsigned long thobjid;  /* be the same length as 'num' for big-endian */
  struct {
    unsigned long loader;
    unsigned long propid;
    unsigned long data;
    struct loadernode *ldnode;
  } propval;
  struct propnode *prop;
  struct objectnode *obj;
  struct fsnode *fsn;
  struct constnode *constn;
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
%token <propval> FINDTHEMEOBJECT
%token <propval> LOADBITMAP
%token <propval> COPY
%token <propval> FONT
%token <propval> ARRAY

%type <num>      constexp
%type <propval>  propertyval
%type <thobjid>  thobj
%type <propid>   property
%type <prop>     statement
%type <prop>     stmt_list
%type <prop>     compound_stmt
%type <obj>      objectdef
%type <obj>      property_stmt
%type <obj>      propdef
%type <obj>      propdef_list
%type <obj>      unit
%type <obj>      unitlist
%type <propval>  fillstyle
%type <fsn>      fsexp
%type <fsn>      fsarglist
%type <fsn>      fsstmt
%type <fsn>      fsstmt_list
%type <fsn>      fsvar_list
%type <fsn>      fsdecl
%type <fsn>      fsdecl_list
%type <fsn>      fsbody
%type <fsn>      fsvar
%type <fsn>      fsprop
%type <constn>   constnode
%type <constn>   constnode_list
%type <fsn>      traversal_const
%type <fsn>      widget_handle

   /* Reserved words */
%token OBJ PROP FILLSTYLE VAR COLORADD COLORSUB COLORDIV COLORMULT
%token CLASS CONTAINER NEXT PREVIOUS APP CHILD WIDGET ARROW

%right '?' ':'
%left OR
%left AND
%left '|'
%left '&'
%left EQUAL NOTEQ
%left LTEQ GTEQ '<' '>'
%left SHIFTL SHIFTR
%left '-' '+'
%left '*' '/'
%nonassoc '!' UMINUS UPLUS
%left CLASS

%start unitlist

%%

unitlist: unit
        | unitlist unit
        ;

   /* This is a list of the structures that can appear
      unenclosed in the file */
unit: objectdef
    | property_stmt
    | ';' {$$=0;}  /* No real purpose but to satisfy people that
		      insist on ending object definitions with a ';' */
    ;

property_stmt: PROP propdef_list { }
             ;

propdef_list: propdef
            | propdef_list ',' propdef
            ;

propdef: UNKNOWNSYM { 
  /* Add a new node to the symbol table */

  struct symnode *n;
  static int next_property = PGTH_P_THEMEAUTO;

  n = malloc(sizeof(struct symnode));
  if (n) {
    n->type = PROPERTY;
    n->name = $1;
    n->value = next_property++;
    n->next = symtab_head;
    symtab_head = n;
  }
  else
    yyerror("memory allocation error");  
}
       | UNKNOWNSYM '=' constexp {
  /* Add a new node to the symbol table */
  struct symnode *n;

  n = malloc(sizeof(struct symnode));
  if (n) {
    n->type = PROPERTY;
    n->name = $1;
    n->value = $3;
    n->next = symtab_head;
    symtab_head = n;
  }
  else
    yyerror("memory allocation error");
}
       ;

objectdef:  OBJ thobj compound_stmt      { 
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
         |  OBJ STRING compound_stmt {
  struct propnode *nameprop;
  unsigned char *buf;
  struct pgrequest *req;
  int len = strlen($2);

  /* Allocate the buffer for name property's request */
  if (buf = malloc(sizeof(struct pgrequest)+len)) {

    /* Reserve space for the request header */
    req = (struct pgrequest *) buf;
    memset(req,0,sizeof(struct pgrequest));
    req->type = htons(PGREQ_MKSTRING);
    req->size = htonl(len);
    
    /* copy string and discard original */
    memcpy(buf+sizeof(struct pgrequest),$2,len);
    free($2);
    
    /* Create the name property */
    nameprop = malloc(sizeof(struct propnode));
    if (nameprop) {
      memset(nameprop,0,sizeof(struct propnode));
      nameprop->loader = PGTH_LOAD_REQUEST;
      nameprop->data   = 0;
      nameprop->ldnode = newloader(buf,(sizeof(struct pgrequest)+len));
      nameprop->propid = PGTH_P_NAME;
      nameprop->next   = $3;
      num_totprop++;
      
      /* Add to the list of objects, including a name property */
      $$ = malloc(sizeof(struct propnode));
      if ($$) {
	memset($$,0,sizeof(struct propnode));
	$$->proplist = nameprop;
	$$->id       = PGTH_O_CUSTOM;
	$$->next     = objectlist;
	objectlist   = $$;
	num_thobj++;
      }
      else
	yyerror("memory allocation error");  
    }
    else
      yyerror("memory allocation error");  
  }
  else
    yyerror("memory allocation error");  
}
         ;

compound_stmt:  statement                { $$ = $1; }
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
     // | UNKNOWNSYM     { $$ = 0; }
     ;

property: PROPERTY
        | constexp    { $$ = $1; }
        | THOBJ       { yyerror("Theme object found in place of property"); }
        // | UNKNOWNSYM  { $$ = 0; }
        ;

propertyval:  constexp          { $$.data = $1; $$.loader = PGTH_LOAD_NONE; $$.ldnode = NULL;}
           |  fillstyle         { $$ = $1; }
           |  COPY '(' THOBJ CLASS PROPERTY ')' {
  /* The Copy() syntax exists for backward compatibility and verbosity */
  $$.data   = ($3 << 16) | $5;
  $$.loader = PGTH_LOAD_COPY;
}     
           |  THOBJ CLASS PROPERTY {
  /* Just like Copy() */
  $$.data   = ($1 << 16) | $3;
  $$.loader = PGTH_LOAD_COPY;
}     
           |  FONT '(' STRING ',' constexp ',' constexp ')' {
  unsigned char *buf;
  struct pgrequest *req;
  struct pgreqd_mkfont *rqd;

  /* Allocate the buffer */
  if (!(buf = malloc(sizeof(struct pgrequest)+sizeof(struct pgreqd_mkfont))))
		yyerror("memory allocation error");
	      
  /* Two structures in the buffer */
  memset(buf,0,sizeof(struct pgrequest) + sizeof(struct pgreqd_mkfont));
  req = (struct pgrequest *) buf;
  rqd = (struct pgreqd_mkfont *) (req+1);

  req->type = htons(PGREQ_MKFONT);
  req->size = htonl(sizeof(struct pgreqd_mkfont));
  if ($3) {
    strcpy(rqd->name,$3);
    free($3);
  }
  rqd->style = htonl($7);
  rqd->size  = htons($5);

  $$.ldnode = newloader(buf,sizeof(struct pgrequest) +
  	    sizeof(struct pgreqd_mkfont));
  $$.loader = PGTH_LOAD_REQUEST;
}	   	   
           |  FINDTHEMEOBJECT '(' STRING ')' {
  $$.ldnode = newloader(strdup($3),strlen($3)+1);
  $$.loader = PGTH_LOAD_FINDTHOBJ;
}
           |  LOADBITMAP '(' STRING ')' {
  FILE *bitf;
  unsigned long size;
  unsigned char *buf;
  struct pgrequest *req;
  
  /* Make a bitmap loader from a file */

  bitf = fopen($3,"r");
  if (!(bitf = fopen($3,"r")))
    yyerror("Error opening bitmap file");
  else {
    fseek(bitf,0,SEEK_END);
    size = ftell(bitf);
    rewind(bitf);

    /* Allocate the buffer */
    if (!(buf = malloc(sizeof(struct pgrequest)+size)))
      yyerror("memory allocation error");
    else {

      /* Reserve space for the request header */
      req = (struct pgrequest *) buf;
      memset(req,0,sizeof(struct pgrequest));
      req->type = htons(PGREQ_MKBITMAP);
      req->size = htonl(size);
      
      /* copy string and discard original */
      fread(buf+sizeof(struct pgrequest),size,1,bitf);
      fclose(bitf);
      
      $$.ldnode = newloader(buf,(sizeof(struct pgrequest)+size));
      $$.loader = PGTH_LOAD_REQUEST;
    }
  }
  free($3);
}     
	   |  STRING {
  unsigned char *buf;
  struct pgrequest *req;
  int len = strlen($1);

  /* Allocate the buffer */
  if (!(buf = malloc(sizeof(struct pgrequest)+len)))
    yyerror("memory allocation error");

  /* Reserve space for the request header */
  req = (struct pgrequest *) buf;
  memset(req,0,sizeof(struct pgrequest));
  req->type = htons(PGREQ_MKSTRING);
  req->size = htonl(len);

  /* copy string and discard original */
  memcpy(buf+sizeof(struct pgrequest),$1,len);
  free($1);

  $$.ldnode = newloader(buf,(sizeof(struct pgrequest)+len));
  $$.loader = PGTH_LOAD_REQUEST;
}
	   |  ARRAY '(' constnode_list ')' {
  unsigned char *buf;
  struct pgrequest *req;
  struct constnode *n,*condemn;
  unsigned long *arr;
  int len;

  /* Count the number of constants in the list */
  n = $3;
  len = 0;
  while (n) {
    len++;
    n = n->next;
  }

  /* Allocate the buffer */
  if (!(buf = malloc(sizeof(struct pgrequest)+ len*4 )))
    yyerror("memory allocation error");

  /* Reserve space for the request header */
  req = (struct pgrequest *) buf;
  memset(req,0,sizeof(struct pgrequest));
  req->type = htons(PGREQ_MKARRAY);
  req->size = htonl(len*4);

  /* Byteswap each constant into the new array, freeing the constant list */
  arr = (unsigned long *) ((unsigned char *) buf + sizeof(struct pgrequest));
  n = $3;
  while (n) {
    *arr = htonl(n->data);
    arr++;
    condemn = n;
    n = n->next;
    free(condemn);
  }

  $$.ldnode = newloader(buf,(sizeof(struct pgrequest)+len*4));
  $$.loader = PGTH_LOAD_REQUEST;
}
           ;

constnode_list: constnode                     { $$ = $1; }
              | constnode_list ',' constnode  { 
  /* Find the end of the list */
  struct constnode *n = $1;
  while (n->next)
    n = n->next;
  /* Concatenate */
  n->next = $3;
  $$ = $1;
}
              ;

constnode: constexp { 
  struct constnode *n = malloc(sizeof(struct constnode));
  if (!n)
    yyerror("memory allocation error");
  n->data = $1;
  n->next = NULL;
  $$ = n;
}
         ;

constexp: constexp '+' constexp { $$ = $1 + $3; }
        | constexp '-' constexp { $$ = $1 - $3; }
        | constexp '&' constexp { $$ = $1 & $3; }
        | constexp '|' constexp { $$ = $1 | $3; }
	| constexp LTEQ constexp { $$ = $1 <= $3; }
	| constexp GTEQ constexp { $$ = $1 >= $3; }
	| constexp '<' constexp { $$ = $1 < $3; }
	| constexp '>' constexp { $$ = $1 > $3; }
	| constexp EQUAL constexp { $$ = $1 == $3; }
	| constexp NOTEQ constexp { $$ = $1 != $3; }
        | constexp SHIFTL constexp { $$ = $1 << $3; }
        | constexp SHIFTR constexp { $$ = $1 >> $3; }
	| constexp OR constexp { $$ = $1 || $3; }
	| constexp AND constexp { $$ = $1 && $3; }
	| '!' constexp { $$ = !$2; }
	| '-' constexp %prec UMINUS { $$ = 0-$2; }
	| '+' constexp %prec UPLUS { $$ = $2; }
        | constexp '*' constexp { $$ = $1 * $3; }
        | constexp '/' constexp {
	  if ($3 == 0)
	    yyerror("Divide by zero");
	  else
	    $$ = $1 / $3; }
        | '(' constexp ')' { $$ = $2; }
	| constexp '?' constexp ':' constexp { 
	  fprintf(stderr,"Warning on %s:%d: question-colon with only constants",
	      filename,lineno);
	  if (yytext[0])
	    fprintf(stderr," at \"%s\"\n",yytext);
	  else
	    fprintf(stderr,"\n");
	  $$ = $1 ? $3 : $5; }
        | NUMBER
	//	| UNKNOWNSYM            { $$ = 0; }
        ;

    /* Fill-style things */

fillstyle: FILLSTYLE  { yyerror("fillstyle requires parameters"); }
         | FILLSTYLE '{' '}' { yyerror("empty fillstyle"); }
         | FILLSTYLE '{' fsbody '}' {
  struct fsnode *p;
  unsigned char *buf,*bp;
  unsigned long bufsize=512;
  struct pgrequest *req;
  long t;

  if (!$3) return;

  /* Allocate the fillstyle buffer */
  if (!(bp = buf = malloc(bufsize)))
    yyerror("memory allocation error");

  /* Reserve space for the request header */
  req = (struct pgrequest *) bp;
  bp += sizeof(struct pgrequest);
  memset(req,0,sizeof(struct pgrequest));
  req->type = htons(PGREQ_MKFILLSTYLE);

  /* Transcribe the fsnodes into real opcodes */

  p = $3;
  while (p) {
    
    /* Check buffer size */
    t = bp-buf;
    if ((bufsize-t)<10) {
      buf = realloc(buf,bufsize <<= 1);
      bp = buf+t;
    }      

    /* Pack short values when possible */
    if (p->op == PGTH_OPCMD_LONGGROP &&
	     p->param < PGTH_OPSIMPLE_GROP)
      p->op = PGTH_OPSIMPLE_GROP | p->param;
    else if (p->op == PGTH_OPCMD_LONGLITERAL &&
	p->param < PGTH_OPSIMPLE_LITERAL)
      p->op = PGTH_OPSIMPLE_LITERAL | p->param;
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
    case PGTH_OPCMD_LOCALCALL:
      *(((unsigned short *)bp)++) = htons(p->param);
      break;

    case PGTH_OPCMD_LONGGET:          /* 1 byte */
    case PGTH_OPCMD_LONGSET:
      *(bp++) = p->param;
      break;
     
    case PGTH_OPCMD_PROPERTY:         /* 2 x 2 byte */
    case PGTH_OPCMD_CALL:
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
      | THOBJ CLASS PROPERTY '(' fsarglist ')' ';' {
  struct fsnode *n;
  $$ = fsnodecat($5,n = fsnewnode(PGTH_OPCMD_CALL));
  n->param = $1;
  n->param2 = $3;
}   
      | PROPERTY '(' fsarglist ')' ';' {
  struct fsnode *n;
  $$ = fsnodecat($3,n = fsnewnode(PGTH_OPCMD_LOCALCALL));
  n->param = $1;
}   
      ;

fsarglist:                     { $$ = NULL; }
         | fsexp               { $$ = $1; }
         | fsarglist ',' fsexp { $$ = fsnodecat($1,$3); }
         ;

fsexp: '(' fsexp ')'    { $$ = $2; }
     | fsexp '+' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_PLUS)); }
     | fsexp '-' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_MINUS)); }
     | fsexp '*' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_MULTIPLY)); }
     | fsexp '/' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_DIVIDE)); }
     | fsexp '|' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_OR)); }
     | fsexp '&' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_AND)); }
     | fsexp EQUAL fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_EQ)); }
     | fsexp '<' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_LT)); }
     | fsexp '>' fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_GT)); }
     | fsexp AND fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_LOGICAL_AND)); }
     | fsexp OR fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_LOGICAL_OR)); }
     | '!' fsexp  { $$ = fsnodecat($2, fsnewnode(PGTH_OPCMD_LOGICAL_NOT)); }
     | fsexp NOTEQ fsexp  { $$ = fsnodecat(fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_EQ)),fsnewnode(PGTH_OPCMD_LOGICAL_NOT)); }
     | fsexp LTEQ fsexp  { $$ = fsnodecat(fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_GT)),fsnewnode(PGTH_OPCMD_LOGICAL_NOT)); }
     | fsexp GTEQ fsexp  { $$ = fsnodecat(fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_LT)),fsnewnode(PGTH_OPCMD_LOGICAL_NOT)); }
     | fsexp SHIFTL fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_SHIFTL)); }
     | fsexp SHIFTR fsexp  { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_SHIFTR)); }
     | '-' NUMBER %prec UMINUS { ($$ = fsnewnode(PGTH_OPCMD_LONGLITERAL))->param = -$2; }
     | '-' fsexp %prec UMINUS { $$ = fsnodecat(fsnodecat(fsnewnode(PGTH_OPCMD_LONGLITERAL),$2),
     		fsnewnode(PGTH_OPCMD_MINUS)); }
     | '+' fsexp %prec UPLUS { $$ = $2; }     
     | NUMBER       { ($$ = fsnewnode(PGTH_OPCMD_LONGLITERAL))->param = $1; }
     | FSVAR          { $$ = fsnewnode(PGTH_OPCMD_LONGGET); $$->param = $1; }
     | fsprop              { $$ = $1; }
     | COLORADD '(' fsarglist ')' { $$ = fsnodecat($3,fsnewnode(PGTH_OPCMD_COLORADD)); }
     | COLORSUB '(' fsarglist ')' { $$ = fsnodecat($3,fsnewnode(PGTH_OPCMD_COLORSUB)); }
     | COLORMULT '(' fsarglist ')' { $$ = fsnodecat($3,fsnewnode(PGTH_OPCMD_COLORMULT)); }
     | COLORDIV '(' fsarglist ')' { $$ = fsnodecat($3,fsnewnode(PGTH_OPCMD_COLORDIV)); }
     | fsexp '?' fsexp ':' fsexp { $$ = fsnodecat(fsnodecat(fsnodecat($5,$3),$1),
					 fsnewnode(PGTH_OPCMD_QUESTIONCOLON)); } 
     | widget_handle CLASS fsexp { $$ = fsnodecat(fsnodecat($1,$3),fsnewnode(PGTH_OPCMD_GETWIDGET)); }
     | widget_handle
     ;

fsprop: THOBJ CLASS PROPERTY { $$ = fsnewnode(PGTH_OPCMD_PROPERTY); $$->param = $1; $$->param2 = $3; }
      | PROPERTY           { $$ = fsnewnode(PGTH_OPCMD_LOCALPROP); $$->param = $1; }
      ;

traversal_const: CONTAINER { ($$ = fsnewnode(PGTH_OPCMD_LONGLITERAL))->param = PG_TRAVERSE_CONTAINER; }
               | NEXT      { ($$ = fsnewnode(PGTH_OPCMD_LONGLITERAL))->param = PG_TRAVERSE_FORWARD; }
               | PREVIOUS  { ($$ = fsnewnode(PGTH_OPCMD_LONGLITERAL))->param = PG_TRAVERSE_BACKWARD; }
               | APP       { ($$ = fsnewnode(PGTH_OPCMD_LONGLITERAL))->param = PG_TRAVERSE_APP; }
               | CHILD     { ($$ = fsnewnode(PGTH_OPCMD_LONGLITERAL))->param = PG_TRAVERSE_CHILDREN; }
               ;

widget_handle: widget_handle ARROW traversal_const '[' fsexp ']' { $$ = fsnodecat(fsnodecat(fsnodecat($5,$3),$1),
										fsnewnode(PGTH_OPCMD_TRAVERSEWGT)); }
             | WIDGET    { $$ = fsnewnode(PGTH_OPCMD_WIDGET); }
             ;

%%

/* Generic error reporting function for parsing errors */
int yyerror(const char *s) {

/*
  if (YYRECOVERING)
    return;
*/

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
