/* $Id: fillstyle.c,v 1.19 2002/01/16 19:47:26 lonetech Exp $
 * 
 * fillstyle.c - Interpreter for fillstyle code
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/common.h>

#include <pgserver/svrtheme.h>
#include <pgserver/pgnet.h>   /* ntohs and friends... */
#include <picogui/theme.h>

#include <stdio.h>  /* for NULL */

/* Stack for the interpreter */
#define FSSTACKSIZE  32
unsigned long fsstack[FSSTACKSIZE];
int fsstkpos;  /* position in the stack */

/* Macros to get the next short/long from the fillstyle buffer */

#define NEXTSHORT      ((unsigned short)( (unsigned short)(p[0])<<8 |\
                        (unsigned short)(p[1]) ))
#define NEXTLONG       ((unsigned long)( (unsigned long)(p[0])<<24 |\
                        (unsigned long)(p[1])<<16 | (unsigned long)\
                        (p[2])<<8 | (unsigned short)(p[3]) ))

/* Little utility functions */

g_error fsgrop(struct gropctxt *ctx,int grop);
g_error fsget(int reg);
g_error fsset(int reg);
g_error fspopargs(void);

/* Arguments for binary operators */
unsigned long fsa,fsb;

g_error check_fillstyle(const unsigned char *fs, unsigned long fssize)
 {
  const unsigned char *p=fs, *plimit=fs+fssize;
  unsigned char op, reg;
  unsigned short grop;

  /* Initialize stack; 4 positions loaded for x y w h */
  fsstkpos = 4;
  while(p<plimit)
   {
    op = *p++;
#ifdef DEBUG_THEME
    printf("check_fillstyle --- Op: 0x%02X stack in: %d ", op, fsstkpos);
#endif
    if (op & PGTH_OPSIMPLE_GROP) {
      /* 1-byte grop */
      grop=op&(PGTH_OPSIMPLE_GROP-1);
      /* TODO: ought to check that it's a supported grop too */
      fsstkpos-=PG_GROPPARAMS(grop);
      if(!PG_GROP_IS_UNPOSITIONED(grop))
	fsstkpos-=4;
    } else if(op & PGTH_OPSIMPLE_LITERAL) {
      fsstkpos++;
    } else if(op & PGTH_OPSIMPLE_CMDCODE) {
      switch (op) {
	case PGTH_OPCMD_LONGLITERAL:
	case PGTH_OPCMD_PROPERTY:
	  /* grabs 4 bytes and pushes 1 value */
	  p+=2;
	  /* fall through */
	case PGTH_OPCMD_LOCALPROP:
	  /* reads 2 bytes, pushes 1 value */
	  p+=2;
	  fsstkpos++;
	  break;
	case PGTH_OPCMD_LONGGROP:
	  /* executes a grop which may be >=PGTH_OPSIMPLE_GROP */
	  grop=NEXTSHORT;
	  p+=2;
	  fsstkpos-=PG_GROPPARAMS(grop);
	  if(!PG_GROP_IS_UNPOSITIONED(grop))
	    fsstkpos-=4;
	  break;
	case PGTH_OPCMD_LONGGET:
	  /* loads a stack value */
	  if(p==plimit)
	    return mkerror(PG_ERRT_BADPARAM,91);  /* Truncated opcode */
	  reg=*(p++);
	  if(reg>=FSSTACKSIZE)
	    return mkerror(PG_ERRT_BADPARAM,90);  /* Var out of range */
	  fsstkpos++;
	  break;
	case PGTH_OPCMD_LONGSET:
	  /* changes a stack value */
	  if(p==plimit)
	    return mkerror(PG_ERRT_BADPARAM,91);  /* Truncated opcode */
	  reg=*(p++);
	  if(reg>=FSSTACKSIZE)
	    return mkerror(PG_ERRT_BADPARAM,90);  /* Var out of range */
	  fsstkpos--;
	  break;
	case PGTH_OPCMD_QUESTIONCOLON:
	  fsstkpos--;
	  /* fall through; 3 arguments, 1 result */
	case PGTH_OPCMD_PLUS:
	case PGTH_OPCMD_MINUS:
	case PGTH_OPCMD_MULTIPLY:
	case PGTH_OPCMD_SHIFTL:
	case PGTH_OPCMD_SHIFTR:
	case PGTH_OPCMD_OR:
	case PGTH_OPCMD_AND:
	case PGTH_OPCMD_EQ:
	case PGTH_OPCMD_LT:
	case PGTH_OPCMD_GT:
	case PGTH_OPCMD_LOGICAL_OR:
	case PGTH_OPCMD_LOGICAL_AND:
	case PGTH_OPCMD_LOGICAL_NOT:
	case PGTH_OPCMD_DIVIDE:
	case PGTH_OPCMD_COLORADD:
	case PGTH_OPCMD_COLORSUB:
	case PGTH_OPCMD_COLORDIV:
	case PGTH_OPCMD_COLORMULT:
	  /* 2 arguments, 1 result */
	  if(!--fsstkpos)
	    return mkerror(PG_ERRT_BADPARAM,88);  /* Stack underflow */
	  break;
	default:
	  return mkerror(PG_ERRT_BADPARAM,67);  /* Bad bytecode */
      }
    } else if(op & PGTH_OPSIMPLE_GET) {
	if((op & (PGTH_OPSIMPLE_GET-1)) >= FSSTACKSIZE)
	  return mkerror(PG_ERRT_BADPARAM,90);  /* Var out of range */
	fsstkpos++;
    } else {	/* 1-byte set */
      if(op >= FSSTACKSIZE)
	return mkerror(PG_ERRT_BADPARAM,90);  /* Var out of range */
      fsstkpos--;
    }
#ifdef DEBUG_THEME
    printf("out: %d\n", fsstkpos);
#endif
    if(fsstkpos<0)
      return mkerror(PG_ERRT_BADPARAM,88);  /* Stack underflow */
    if (fsstkpos>=FSSTACKSIZE)
      return mkerror(PG_ERRT_BADPARAM,89);  /* Stack overflow */
   }
  if(p>plimit)
    return mkerror(PG_ERRT_BADPARAM,91);  /* Truncated opcode */
  return success;
 }

/* Fillstyle interpreter- generates/refreshes a gropnode list */
g_error exec_fillstyle(struct gropctxt *ctx,unsigned short state,
		       unsigned short property) {
  g_error e;
  unsigned long fssize;  /* Fillstyle size */
  unsigned char *fs;  /* Pointer to the actual fillstyle data */
  unsigned char *p,*plimit;
  unsigned char op;
  int r,g,b;          /* For color arithmetic */

  /* Look up the fillstyle */
  e = rdhandle((void**)&fs,PG_TYPE_FILLSTYLE,-1,theme_lookup(state,property));
  errorcheck;

  if (!fs) {
    
    /* When our best just isn't good enough... */
    if (property == PGTH_P_BACKDROP)
      return success;

    /* The default fillstyle, if no theme is loaded or no 
       theme has defined the property*/

    addgrop(ctx,PG_GROP_SETCOLOR);
    ctx->current->param[0] = VID(color_pgtohwr) (0x000000);
    
    switch (state) {

    case PGTH_O_BUTTON_ON:      /* 2 borders */
      addgropsz(ctx,PG_GROP_FRAME,ctx->x,ctx->y,ctx->w,ctx->h);
      ctx->x += 1; ctx->y += 1; ctx->w -= 2; ctx->h -= 2;
    default:                    /* 1 border */
      addgropsz(ctx,PG_GROP_FRAME,ctx->x,ctx->y,ctx->w,ctx->h);
      ctx->x += 1; ctx->y += 1; ctx->w -= 2; ctx->h -= 2;
    case PGTH_O_LABEL_SCROLL:   /* No border */
      addgrop(ctx,PG_GROP_SETCOLOR);
      ctx->current->param[0] = VID(color_pgtohwr) (theme_lookup(state,PGTH_P_BGCOLOR));
      addgropsz(ctx,PG_GROP_RECT,ctx->x,ctx->y,ctx->w,ctx->h);      
    }
    return success;
  }

  /* Reset stack. preload x,y,w,h as local variables */
  if (ctx) {
    fsstack[0] = ctx->x;
    fsstack[1] = ctx->y;
    fsstack[2] = ctx->w;
    fsstack[3] = ctx->h;
  }
  fsstkpos = 4;

  /* Process the opcodes */
  fssize = *(((unsigned long *)fs)++);
  p = fs;
  plimit = fs+fssize;
  while (p<plimit) {
    op = *(p++);
    
    /* These must occur in MSB to LSB order! (see constants.h) */
    if (op & PGTH_OPSIMPLE_GROP) {
      /* 1-byte gropnode */
      e = fsgrop(ctx,op & (PGTH_OPSIMPLE_GROP-1));
      errorcheck;
    }
    else if (op & PGTH_OPSIMPLE_LITERAL) {
      /* 1-byte literal */
      fsstack[fsstkpos++] = op & (PGTH_OPSIMPLE_LITERAL-1);
    }
    else if (op & PGTH_OPSIMPLE_CMDCODE) {
      /* Command code */
      switch (op) {

      case PGTH_OPCMD_LONGLITERAL:
	if ((plimit-p)<4)
	  return mkerror(PG_ERRT_BADPARAM,91);  /* Truncated opcode */
	 fsstack[fsstkpos++] = NEXTLONG;
	 p += 4;
	 break;

      case PGTH_OPCMD_LONGGROP:
	if ((plimit-p)<2)
	  return mkerror(PG_ERRT_BADPARAM,91);  /* Truncated opcode */
	e = fsgrop(ctx,NEXTSHORT);
	p += 2;
	errorcheck;
	break;

      case PGTH_OPCMD_LONGGET:
	if (plimit<=p)
	  return mkerror(PG_ERRT_BADPARAM,91);  /* Truncated opcode */
	e = fsget(*(p++));
	errorcheck;
	break;

      case PGTH_OPCMD_LONGSET:
	if (plimit<=p)
	  return mkerror(PG_ERRT_BADPARAM,91);  /* Truncated opcode */
	e = fsset(*(p++));
	errorcheck;
	break;

      case PGTH_OPCMD_PROPERTY:
	if ((plimit-p)<4)
	  return mkerror(PG_ERRT_BADPARAM,91);  /* Truncated opcode */
	fsa = NEXTSHORT;
	p += 2;
	fsb = NEXTSHORT;
	p += 2;
	fsstack[fsstkpos++] = theme_lookup(fsa,fsb);
	break;

      case PGTH_OPCMD_LOCALPROP:
	if ((plimit-p)<2)
	  return mkerror(PG_ERRT_BADPARAM,91);  /* Truncated opcode */
	 fsa = NEXTSHORT;
	 p += 2;
#ifdef DEBUG_THEME
	 printf("Local theme lookup, property %ld\n",fsa);
#endif
	 fsstack[fsstkpos++] = theme_lookup(state,fsa);
	break;

      case PGTH_OPCMD_PLUS:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa + fsb;
	break;

      case PGTH_OPCMD_MINUS:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa -  fsb;
	break;

      case PGTH_OPCMD_MULTIPLY:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa * fsb;
	break;

      case PGTH_OPCMD_SHIFTL:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa << fsb; 
	break;
	
      case PGTH_OPCMD_SHIFTR:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa >> fsb;
	break;

      case PGTH_OPCMD_OR:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa | fsb;
	break;

      case PGTH_OPCMD_AND:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa & fsb;
	break;

      case PGTH_OPCMD_EQ:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa == fsb;
	break;

      case PGTH_OPCMD_LT:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa < fsb;
	break;

      case PGTH_OPCMD_GT:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa > fsb;
	break;

      case PGTH_OPCMD_LOGICAL_OR:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa || fsb;
	break;

      case PGTH_OPCMD_LOGICAL_AND:
	e = fspopargs();
	errorcheck;
	fsstack[fsstkpos++] = fsa && fsb;
	break;

      case PGTH_OPCMD_LOGICAL_NOT:
	fsstack[fsstkpos-1] = !fsstack[fsstkpos-1];
	break;

      case PGTH_OPCMD_DIVIDE:   
	e = fspopargs();
	errorcheck;
	if (fsb)
	  fsstack[fsstkpos++] = fsa / fsb; 
	else
	  fsstack[fsstkpos++] = 0xFFFFFFFF;   /* limit of fsa/fsb as fsb approaches 0 */
	break;

      case PGTH_OPCMD_COLORADD:
	e = fspopargs();
	errorcheck;
	r = getred(fsa);
	g = getgreen(fsa);
	b = getblue(fsa);
	r += getred(fsb);
	g += getgreen(fsb);
	b += getblue(fsb);
	if (r>255) r = 255;
	if (g>255) g = 255;
	if (b>255) b = 255;
	fsstack[fsstkpos++] = mkcolor(r,g,b);
	break;

      case PGTH_OPCMD_COLORSUB:
	e = fspopargs();
	errorcheck;
	r = getred(fsa);
	g = getgreen(fsa);
	b = getblue(fsa);
	r -= getred(fsb);
	g -= getgreen(fsb);
	b -= getblue(fsb);
	if (r<0) r = 0;
	if (g<0) g = 0;
	if (b<0) b = 0;
	fsstack[fsstkpos++] = mkcolor(r,g,b);
	break;

      case PGTH_OPCMD_COLORDIV:
	e = fspopargs();
	errorcheck;
	r = getred(fsa);
	g = getgreen(fsa);
	b = getblue(fsa);
	r = getred(fsb) ? (r/getred(fsb)) : 0xFF;     /* Avoid divide by zero */
	g = getgreen(fsb) ? (g/getgreen(fsb)) : 0xFF; 
	b = getred(fsb) ? (b/getblue(fsb)) : 0xFF;
     	fsstack[fsstkpos++] = mkcolor(r,g,b);
	break;

      case PGTH_OPCMD_COLORMULT:
	e = fspopargs();
	errorcheck;
	r = getred(fsa);
	g = getgreen(fsa);
	b = getblue(fsa);
	r *= getred(fsb);
	g *= getgreen(fsb);
	b *= getblue(fsb);
	if (r>255) r = 255;
	if (g>255) g = 255;
	if (b>255) b = 255;
	fsstack[fsstkpos++] = mkcolor(r,g,b);
	break;

      case PGTH_OPCMD_QUESTIONCOLON:
	if (fsstkpos<3)
	  return mkerror(PG_ERRT_BADPARAM,88);  /* Stack underflow */
	fsstkpos -= 2;
	fsstack[fsstkpos-1] = fsstack[fsstkpos+1] ? 
	  fsstack[fsstkpos] : fsstack[fsstkpos-1];
	break;

      }
    }
    else if (op & PGTH_OPSIMPLE_GET) {
      /* 1-byte get */
      e = fsget(op & (PGTH_OPSIMPLE_GET-1));
      errorcheck;
    }
    else {
      /* 1-byte set */
      e = fsset(op);
      errorcheck;
    }

#ifdef DEBUG_THEME
    /* trace */
    printf("FILLSTYLE --- Op: 0x%02X Stk:",op);
    for (fsa=0;fsa<fsstkpos;fsa++)
      printf(" %ld",fsstack[fsa]);
    printf("\n"); 
#endif
    
    /* check for stack over/underflow */
    if (fsstkpos<0)
      return mkerror(PG_ERRT_BADPARAM,88);  /* Stack underflow */
    if (fsstkpos>=FSSTACKSIZE)
      return mkerror(PG_ERRT_BADPARAM,89);  /* Stack overflow */
  }
  
  /* Return x,y,w,h */
  if (ctx) {
    ctx->x = fsstack[0];
    ctx->y = fsstack[1];
    ctx->w = fsstack[2];
    ctx->h = fsstack[3];
  }    

  return success;
}

g_error fsgrop(struct gropctxt *ctx,int grop) {
  g_error e;
  int params = PG_GROPPARAMS(grop);
  int i,j;
  
  if (PG_GROP_IS_UNPOSITIONED(grop)) {
     
     if (fsstkpos<params)
       return mkerror(PG_ERRT_BADPARAM,88);  /* Stack underflow */
     
     e = addgrop(ctx,grop);
  }
  else {
  
     if (fsstkpos<params+4)
       return mkerror(PG_ERRT_BADPARAM,88);  /* Stack underflow */
     
     e = addgropsz(ctx,grop,fsstack[fsstkpos-params-4],fsstack[fsstkpos-params-3],
		   fsstack[fsstkpos-params-2],fsstack[fsstkpos-params-1]);
  }
  errorcheck;

  for (i=fsstkpos-params,j=0;j<params;i++,j++)
     ctx->current->param[j] = fsstack[i]; 
     
  /* Special case: handle color conversion here (simplifies
   * the themes themselves) */
  if (grop == PG_GROP_SETCOLOR)
     ctx->current->param[0] = VID(color_pgtohwr) (ctx->current->param[0]);
   
  fsstkpos -= params;
  if (!PG_GROP_IS_UNPOSITIONED(grop))
     fsstkpos -= 4;

  return success;
}

g_error fsget(int reg) {
  if (reg>=FSSTACKSIZE)
    return mkerror(PG_ERRT_BADPARAM,90);  /* Var out of range */
  fsstack[fsstkpos++] = fsstack[reg];
  return success;
}

g_error fsset(int reg) {
  if (reg>=FSSTACKSIZE)
    return mkerror(PG_ERRT_BADPARAM,90);  /* Var out of range */
  fsstack[reg] = fsstack[--fsstkpos]; 
  return success;
}

g_error fspopargs(void) {
  if (fsstkpos<2)
    return mkerror(PG_ERRT_BADPARAM,88);  /* Stack underflow */
  fsb = fsstack[--fsstkpos];
  fsa = fsstack[--fsstkpos];
  return success;
}

/* The End */
