/*
 * Filename:  ai3.c
 * Author:    Brandon Smith
 * Date:      April 1, 2002
 * Purpose:   The third graduation of the AI levels
 *
 * Copyright (C) 2002 Brandon Smith <lottabs2@yahoo.com> 
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
 *
 */

#include "connectfour.h"
#include "ai.h"
#include "ai1.h"
#include "ai2.h"
#include "ai3.h"

#include "ai5.h"

#define DEBUG
#define FUNCTION_DEBUG

void ai3(struct board *it)
{
  int temp;

  temp = nextmovewin(it);
#ifdef DEBUG
    fprintf(stderr,"nextmovewin returned %d\n",temp);
#endif

  if(temp != -1)
  {
    move(it,temp);
    return;
  }
  temp = nextmovelose(it,-1);
#ifdef DEBUG
  fprintf(stderr,"nextmovelose returned %d\n",temp);
#endif
  if(temp != -1)
  {
    notmove(it,temp);
    return;
  }

  randommove(it);
}

int nextmovelose(struct board *it, int param)
{
  int x,y,total,i;

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"nextmovelose called - param = %d.\n",param);
#endif

  //zero slope
  for(x=0;x<4;x++)
    for(y=1;y<6;y++)
    {
      total = glook(it,x,y) + glook(it,x+1,y) + glook(it,x+2,y) + glook(it,x+3,y);
      if(total == 3 || total == -3)
	for(i=0;i<4;i++)
	  if(glook(it,x+i,y) == 0 && glook(it,x+i,y-1) == 0 && glook(it,x+i,y-2) != 0)
	    if(paramcheck(param,x+i) == -1)
	       return nextmovelose(it,paramadd(param,x+i));
    }
  
  //positive slope
  for(x=0;x<4;x++)
    for(y=0;y<3;y++)
    { 
      total = glook(it,x,y) + glook(it,x+1,y+1) + glook(it,x+2,y+2) + glook(it,x+3,y+3);
      if(total == 3 || total == -3)
	for(i=0;i<4;i++)
	  if(glook(it,x+i,y+i) == 0 && glook(it,x+i,y+i-1) == 0 && glook(it,x+i,y+i-2) != 0)
	    if(paramcheck(param,x+i) == -1)
	      return nextmovelose(it,paramadd(param,x+i));
    }
  
  //negative slope
  for(x=0;x<4;x++)
    for(y=0;y<6;y++)
    {
      total = glook(it,x,y) + glook(it,x+1,y-1) + glook(it,x+2,y-2) + glook(it,x+3,y-3);
      if(total == 3 || total == -3)
	for(i=0;i<4;i++)
	  if(glook(it,x+i,y-i) == 0 && glook(it,x+i,y-i-1) == 0 && glook(it,x+i,y-i-2) != 0)
	    if(paramcheck(param,x+i) == -1)
	      return nextmovelose(it,paramadd(param,x+i));
    }
  return param;
}

/* 
   FIXME 
   This should use bitshifts and masks (3 bits) instead of this crazy base 10 and modulus crap 
   changing it around won't be hard because all of the param stuff is here
*/

void notmove(struct board *it, int param)
{
  int spot;
  int tester = 0;
#ifdef FUNCTION_DEBUG  
  fprintf(stderr,"notmove called - param = %d.\n",param);
#endif

  /* try for a random good spot 100 times */
  while(tester != 20)
  {
    if(it -> ailevel > 4)
      spot = rand() % 5 + it->aipref - 1; 
    else
      spot = rand() % 7;
    if(paramcheck(param,spot) == -1)
    {
      if(move(it,spot) != -1)
	return;
    }
    tester++;
  }
  
  /* make sure that it wasn't a fluke I missed it */
  for(tester = 0 ; tester < 7 ; tester++)
  {
    if(paramcheck(param,spot) == -1)
    {
      if(move(it,spot) != -1)
	return;
    }
  }
  
  if(it -> ailevel < 5)
    randommove(it);
  else
    prandmove(it);
}

int paramcheck(int param, int test)
{
  int temp;

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"paramcheck called - param = %d, test = %d.\n",param,test);
#endif
  

  while(param > 0)
  {
    temp = param % 10;
    if(temp == 8)
      temp = 0;
    if(temp == test)
      return test;
    else
      param = param / 10;
  }
  return -1;
}

int paramadd(int param, int add)
{
#ifdef FUNCTION_DEBUG
  fprintf(stderr,"paramadd called - param = %d, add = %d.\n",param,add);
#endif /* DEBUG_STUFF */
  if(add == 0)
    add = 8;
  if(param == -1)
    return add;
  else
    return param * 10 + add;
}
