/* 
 * vrcalc.c - Basic calculator for PicoGUI
 *
 * Copyright (C) 2001 Sean Barnes <sean.a.barnes@att.net>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <picogui.h>
#include <stdio.h>
#include <math.h>
#include "vrcalc.h"
#include <stdlib.h>

int main(int argc, char **argv) {
  pghandle box,row1,row2,row3,row4,row5;
  pghandle fResult, fButton;
  int btn_height;

  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,
		"Calculator",0);

  btn_height = pgThemeLookup(PGTH_O_BUTTON,PGTH_P_HEIGHT);
  fResult = pgNewFont(NULL,btn_height + btn_height/2,0);
  fButton = pgNewFont(NULL,btn_height, PG_FSTYLE_BOLD);
   
  //Display results in this widget
  display=pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("0"),
	      PG_WP_TRANSPARENT,0,
	      PG_WP_FONT,fResult,
	      PG_WP_ALIGN, PG_A_RIGHT,
	      0);
  //Container for the buttons
  box=pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TRANSPARENT,1,
	      PG_WP_MARGIN,0,
	      0);
  row1=pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,box);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,5),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_TRANSPARENT,1,
	      0);

  row2=pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,5),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_TRANSPARENT,1,
	      0);

  row3=pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,5),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_TRANSPARENT,1,
	      0);

  row4=pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,5),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_TRANSPARENT,1,
	      0);

  row5=pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,5),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_TRANSPARENT,1,
	      0);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,row1);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_TEXT,pgNewString("2nd"),
	      PG_WP_FONT, fButton,
	      PG_WP_EXTDEVENTS,PG_EXEV_TOGGLE,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnSecond,&display);

  btn_del=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("<-"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnDelete,&display);

  btn_neg=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("+/-"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnNegate,&display);

  btn_pow=pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("^"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnPower,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,row2);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("7"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnSeven,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("8"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnEight,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("9"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnNine,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("/"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnDivide,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,row3);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("4"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnFour,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("5"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnFive,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("6"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnSix,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("*"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnMultiply,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,row4);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("1"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnOne,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("2"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnTwo,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("3"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnThree,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("-"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnMinus,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,row5);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("0"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnZero,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("."),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnDecimal,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("="),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnEquals,&display);

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZE,pgFraction(1,4),
	      PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
	      PG_WP_FONT, fButton,
	      PG_WP_TEXT,pgNewString("+"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnPlus,&display);

  currentStack=(Node*)malloc(sizeof(Node));
  number[0]='0';
  pgReplaceText(display,number);
  pgEventLoop();
  if (currentStack)
    sClear(currentStack);
  return 0;
}


int btnSecond(struct pgEvent *evt) {
  second=second ? 0 : 1; //toggle global flag
  if (second) {
    // show second operations
    pgReplaceText(btn_del,"C");
    pgReplaceText(btn_neg,"(");
    pgReplaceText(btn_pow,")");
  } else {
    // show normal operations
    pgReplaceText(btn_del,"<-");
    pgReplaceText(btn_neg,"+/-");
    pgReplaceText(btn_pow,"^");
  }
  return 0;
}
int btnDelete(struct pgEvent *evt) {
  if (!second) {
    if (isResult)
      {
	/* No delete on a result */
	return 0;
      }

    if (count) {
      if (number[count-1]=='.')
	doNumber('@'); //dummy function do delete the decimal point
      number[count-1]='\0';
      --count;
    }
    if (!count) {
      number[0]='0';
      sign=' ';
    }
    pgReplaceTextFmt(display,"%c%s",sign,number);
  } else {
    int temp;
    count=0;
    for (temp=0;temp<11;++temp)
      number[temp]='\0';
    number[0]='0';
    sign=' ';
    pgReplaceTextFmt(display,"%c%s",sign,number);

    // Clean up all memory and start with a blank slate
    sClear(currentStack);
    currentStack=(Node*)malloc(sizeof(Node));
    currentStack->stack=0;
    currentStack->op_stack=0;
    currentStack->old=0;
    openp=0;
  }
  return 0;
}
int btnNegate(struct pgEvent *evt) {
  if (!second) {
    if (count)
      sign= (sign==' ') ? '-' : ' ';
    pgReplaceTextFmt(display,"%c%s",sign,number);
  } else {
    //FIXME
    Node* newstack;
    newstack=(Node*)malloc(sizeof(Node));
    newstack->stack=0;
    newstack->op_stack=0;
    newstack->old=currentStack;
    currentStack=newstack;
    pgReplaceText(display,"0");
    openp++;
  }
  return 0;
}
int btnPower(struct pgEvent *evt) {
  if (!second)
    return doOperation(POWER);
  else {
    if (openp) {
      double answer;
      Node* temp;
      int s;
      s= (sign=='-') ? -1 : 1;
      currentStack->stack=pushNumber(currentStack->stack,s * atof(number));
      answer=evaluate(currentStack);
      openp--;
      temp=currentStack->old;
      dClear(currentStack->stack);
      free(currentStack);
      currentStack=temp;
      snprintf(number,11,"%f",answer);
    }
  }
}

int doNumber(char n) {
  static int dec=0; //no decimal yet
  if (isResult)
    {
      int i;
      isResult = 0;
      /* Clear the pending number */
      count = 0;
      for (i = 0;i < 11; i++)
	number [i] = '\0';
      number [0]='\0';
      sign = ' ';
    }

  if (n=='@')
    return dec=0; // Used by the delete button to delete decimal point
  if (count==0)
    dec=0;
  if (count!=NDIGITS && (!(dec && n=='.'))) {
    if (n=='.')
      dec=1;
    number[count]=n;
    count++;
    pgReplaceTextFmt(display,"%c%s",sign,number);
  }
  return 0;
}

int btnSeven(struct pgEvent *evt) {
  return doNumber('7');
}
int btnEight(struct pgEvent *evt) {
  return doNumber('8');
}
int btnNine(struct pgEvent *evt) {
  return doNumber('9');
}
int btnDivide(struct pgEvent *evt) {
  if (doOperation(DIVISION)) {
    //do something on division by zero
  }
  return 0;
}
int btnFour(struct pgEvent *evt) {
  return doNumber('4');
}
int btnFive(struct pgEvent *evt) {
  return doNumber('5');
}
int btnSix(struct pgEvent *evt) {
  return doNumber('6');
}
int btnMultiply(struct pgEvent *evt) {
  return doOperation(MULTIPLICATION);
}
int btnOne(struct pgEvent *evt) {
  return doNumber('1');
}
int btnTwo(struct pgEvent *evt) {
  return doNumber('2');
}
int btnThree(struct pgEvent *evt) {
  return doNumber('3');
}
int btnMinus(struct pgEvent *evt) {
  return doOperation(SUBTRACTION);
}
int btnZero(struct pgEvent *evt) {
  if (count!=0 && !isResult)
    doNumber('0');
  return 0;
}
int btnDecimal(struct pgEvent *evt) {
  return doNumber('.');
}
int btnEquals(struct pgEvent *evt) {
  int temp;
  temp= (sign=='-') ? -1 : 1;
  if (!currentStack->op_stack)
    return 0;
  currentStack->stack=pushNumber(currentStack->stack,temp * atof(number));
  while (openp) {
    double answer;
    Node* temp;
    int s;
    s= (sign=='-') ? -1 : 1;
    //    currentStack->stack=pushNumber(currentStack->stack,s * atof(number));
    answer=evaluate(currentStack);
    openp--;
    temp=currentStack->old;
    dClear(currentStack->stack);
    free(currentStack);
    currentStack=temp;
    currentStack->stack=pushNumber(currentStack->stack,answer);
  }
  evaluate(currentStack);

  {
    /* Make the result the current pending number */
    int i, j;
    char * ans = pgGetString (pgGetWidget (display, PG_WP_TEXT));

    sign = ' ';
    for (i = 0, j = 0; i < NDIGITS + 2; i++)
      {
	if (ans [i] == '-')
	  {
	    sign = '-';
	  }
	else
	  {
	    number [j] = ans [i];
	    if (number [j] == '\0')
	      {
		break;
	      }
	    j++;
	  }
      }
    count = j;

    /* Mark it as a result */
    isResult = 1;
  }
  
  // Clean up all memory and start with a blank slate
  sClear(currentStack);
  currentStack=(Node*)malloc(sizeof(Node));
  currentStack->stack=0;
  currentStack->op_stack=0;
  currentStack->old=0;
}
int btnPlus(struct pgEvent *evt) {
  return doOperation(ADDITION);
}

void dClear(dNode* stack) {
  dNode* temp=stack->lower;
  free(stack);
  if (temp)
    dClear(temp);
}
void oClear(oNode* stack) {
  oNode* temp=stack->lower;
  free(stack);
  if (temp)
    oClear(temp);
}
void sClear(Node* stack) {
  Node* temp=stack->old;
  if (stack->stack)
    dClear(stack->stack);
  if (stack->op_stack)
    oClear(stack->op_stack);
  free(stack);
  if (temp)
    sClear(temp);
}

int doOperation(enum op n) {
  int temp=1;
  if (sign=='-')
    temp=-1;
  // first push the number onto the stack
  currentStack->stack=pushNumber(currentStack->stack,temp * atof(number));
  count=0;
  for (temp=0;temp<NDIGITS+1;++temp)
    number[temp]='\0';
  number[0]='0';
  sign=' ';
  
  while (currentStack->op_stack && (order[n] <= order[currentStack->op_stack->value])) {
    //evaluate one level of the stack
    char ans[NDIGITS+2];
    int counter;
    double number1;
    double number2;
    enum op operation;
    double answer;
    
    //get values to work with
    currentStack->op_stack=popOp(currentStack->op_stack,&operation);
    currentStack->stack=popNumber(currentStack->stack,&number2);
    currentStack->stack=popNumber(currentStack->stack,&number1);
    switch (operation) {
    case ADDITION:
      answer=number1+number2; break;
    case SUBTRACTION:
      answer=number1-number2; break;
    case MULTIPLICATION:
      answer=number1*number2; break;
    case DIVISION:
      answer=number1/number2; break;
    case POWER:
      answer=pow(number1,number2); break;
    }
    currentStack->stack=pushNumber(currentStack->stack,answer);

    // this mess of loops removes any trailing zeros (for a nicer display)
    for (counter=0;counter<NDIGITS+2;counter++)
      ans[counter]='\0';
    snprintf(ans,NDIGITS+2,"%.8f",answer);
    for (counter=0;counter<NDIGITS+2;counter++) {
      if (ans[counter]=='.') {
	for (counter=NDIGITS+1;counter>=0;counter--) {
	  if (ans[counter]=='0') {
	    ans[counter]='\0';
	  } else {
	    if (ans[counter]=='\0')
	      continue;
	    if (ans[counter]=='.')
	      ans[counter]='\0';
	    break;
	  }
	}
	break;
      }
    }
    pgReplaceText(display,ans);
  }
  currentStack->op_stack=pushOp(currentStack->op_stack,n);
  
  return 0;
}

dNode* pushNumber(dNode* stack,double n) {
  dNode* temp;
  temp=(dNode*)malloc(sizeof(dNode));
  temp->lower=stack;
  temp->value=n;
  return temp;
}

dNode* popNumber(dNode* stack, double *n) {
  dNode* temp;
  *n=stack->value;
  temp=stack->lower;
  free(stack);
  return temp;
}

oNode* pushOp(oNode* stack, enum op n) {
  oNode* temp;
  temp=(oNode*)malloc(sizeof(oNode));
  temp->lower=stack;
  temp->value=n;
  return temp;
}

oNode* popOp(oNode* stack, enum op *n) {
  oNode* temp;
  *n=stack->value;
  temp=stack->lower;
  free(stack);
  return temp;
}

double evaluate(Node* stack) {
  char ans[NDIGITS+2];
  int counter;
  double answer;
  double number1;
  double number2;
  enum op operation;

  while (stack->op_stack) {    
    //get values to work with
    currentStack->op_stack=popOp(currentStack->op_stack,&operation);
    currentStack->stack=popNumber(currentStack->stack,&number2);
    currentStack->stack=popNumber(currentStack->stack,&number1);
    switch (operation) {
    case ADDITION:
      answer=number1+number2; break;
    case SUBTRACTION:
      answer=number1-number2; break;
    case MULTIPLICATION:
      answer=number1*number2; break;
    case DIVISION:
      //      if (number2==0)
      //return 0;
      answer=number1/number2; break;
    case POWER:
      answer=pow(number1,number2); break;
    }
    currentStack->stack=pushNumber(currentStack->stack,answer);

  }
  // this mess of loops removes any trailing zeros (for a nicer display)
  for (counter=0;counter<NDIGITS+2;counter++)
    ans[counter]='\0';
  snprintf(ans,NDIGITS+2,"%.8f",answer);
  for (counter=0;counter<NDIGITS+2;counter++) {
    if (ans[counter]=='.') {
      for (counter=NDIGITS+1;counter>=0;counter--) {
	if (ans[counter]=='0') {
	  ans[counter]='\0';
	} else {
	  if (ans[counter]=='\0')
	    continue;
	  if (ans[counter]=='.')
	    ans[counter]='\0';
	  break;
	}
      }
      break;
    }
  }
  pgReplaceText(display,ans);
  return answer;
}
