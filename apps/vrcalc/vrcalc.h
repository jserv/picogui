/* 
 * vrcalc.h - Basic calculator for PicoGUI
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

#ifndef _VRCALC_H
#define _VRCALC_H

int second=0;
int openp=0;
pghandle btn_del,btn_neg,btn_pow,display;

enum op {ADDITION,SUBTRACTION,
	 MULTIPLICATION,DIVISION,
	 POWER};
const int order[]={1,1,
		   2,2,
		   3};

struct _dNode {
  double value;
  struct _dNode *lower; //pointer to node lower in the stack
};

typedef struct _dNode dNode;

struct _oNode {
  enum op value;
  struct _oNode *lower; //pointer to node lower in the stack
};

typedef struct _oNode oNode;

struct _Node {
  dNode* stack; //stack of numbers
  oNode* op_stack; //stack of operations
  struct _Node *old; //pointer to previous stacks
};

typedef struct _Node Node;

Node* firstStack;
Node* currentStack;

#ifdef POCKETBEE
/* Less precision with uClibm ... */
#define NDIGITS 8
#else
#define NDIGITS 10
#endif
char number[NDIGITS + 1];
int count=0; //how many characters are there? (max: NDIGITS)
char sign=' ';
int isResult = 0; // Flag indicating the current number is a result

double evaluate(Node* stack);
void dClear(dNode* stack);
void oClear(oNode* stack);
void sClear(Node* stack);
int doNumber(char);
int doOperation(enum op);
dNode* pushNumber(dNode*,double);
dNode* popNumber(dNode*,double*);
oNode* pushOp(oNode*,enum op);
oNode* popOp(oNode*,enum op*);
int btnSecond(struct pgEvent *evt);
int btnDelete(struct pgEvent *evt);
int btnNegate(struct pgEvent *evt);
int btnPower(struct pgEvent *evt);
int btnSeven(struct pgEvent *evt);
int btnEight(struct pgEvent *evt);
int btnNine(struct pgEvent *evt);
int btnDivide(struct pgEvent *evt);
int btnFour(struct pgEvent *evt);
int btnFive(struct pgEvent *evt);
int btnSix(struct pgEvent *evt);
int btnMultiply(struct pgEvent *evt);
int btnOne(struct pgEvent *evt);
int btnTwo(struct pgEvent *evt);
int btnThree(struct pgEvent *evt);
int btnMinus(struct pgEvent *evt);
int btnZero(struct pgEvent *evt);
int btnDecimal(struct pgEvent *evt);
int btnEquals(struct pgEvent *evt);
int btnPlus(struct pgEvent *evt);

#endif
