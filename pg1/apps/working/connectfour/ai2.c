/*
 * Filename:  ai2.c
 * Author:    Brandon Smith
 * Date:      April 1, 2002
 * Purpose:   the "second" increment of the connect four AI
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

#include "ai2.h"
#include "ai1.h"
#include "ai.h"

#define FUNCTION_DEBUG

void ai2(struct board *it)
{
  int temp;
  temp = nextmovewin(it);
  if(temp != -1)
    move(it,temp);
  else
    randommove(it);
}

int nextmovewin(struct board *it)
{ 
  int i;
  int x;
  int y;
  int total;
  int toreturn;

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"nextmovewin called\n");
#endif

  //zero slope
  for(x=0;x<4;x++)
    for(y=0;y<6;y++)
    {
      total = glook(it,x,y) + glook(it,x+1,y) + glook(it,x+2,y) + glook(it,x+3,y);
      if(total == -3)
	for(i=0;i<4;i++)
	  if(glook(it,x+i,y) == 0 && glook(it,x+i,y-1) != 0)
	    return x+i;
    }
  
  //no slope
  for(x=0;x<7;x++)
    for(y=0;y<3;y++)
    {
      total = glook(it,x,y) + glook(it,x,y+1) + glook(it,x,y+2) + glook(it,x,y+3);
      if(total == -3)
	return x;
    }
  
  //positive slope
  for(x=0;x<4;x++)
    for(y=0;y<3;y++)
    { 
      total = glook(it,x,y) + glook(it,x+1,y+1) + glook(it,x+2,y+2) + glook(it,x+3,y+3);
      if(total == -3)
	for(i=0;i<4;i++)
	  if(glook(it,x+i,y+i) == 0 && glook(it,x+i,y+i-1) != 0)
	    return x+i;
    }
  
  //negative slope
  for(x=0;x<4;x++)
    for(y=3;y<6;y++)
    {
      total = glook(it,x,y) + glook(it,x+1,y-1) + glook(it,x+2,y-2) + glook(it,x+3,y-3);
      if(total == -3)
	for(i=0;i<4;i++)
	  if(glook(it,x+i,y-i) == 0 && glook(it,x+i,y-i-1) != 0)
	    return x+i;
     }
  
  //zero slope
  for(x=0;x<4;x++)
    for(y=0;y<6;y++)
    {
      total = glook(it,x,y) + glook(it,x+1,y) + glook(it,x+2,y) + glook(it,x+3,y);
      if(total == 3)
	for(i=0;i<4;i++)
	  if(glook(it,x+i,y) == 0 && glook(it,x+i,y-1) != 0)
	    return x+i;
    }

  //no slope
  for(x=0;x<7;x++)
    for(y=0;y<3;y++)
    {
      total = glook(it,x,y) + glook(it,x,y+1) + glook(it,x,y+2) + glook(it,x,y+3);
      if(total == 3)
	return x;
    }
  
  //positive slope
  for(x=0;x<4;x++)
    for(y=0;y<3;y++)
    { 
      total = glook(it,x,y) + glook(it,x+1,y+1) + glook(it,x+2,y+2) + glook(it,x+3,y+3);
      if(total == 3)
	for(i=0;i<4;i++)
	  if(glook(it,x+i,y+i) == 0 && glook(it,x+i,y+i-1) != 0)
	    return x+i;
    }
  
  //negative slope
  for(x=0;x<4;x++)
    for(y=3;y<6;y++)
    {
      total = glook(it,x,y) + glook(it,x+1,y-1) + glook(it,x+2,y-2) + glook(it,x+3,y-3);
      if(total == 3)
	for(i=0;i<4;i++)
	  if(glook(it,x+i,y-i) == 0 && glook(it,x+i,y-i-1) != 0)
	    return x+i;
    }
  
  return -1;
}


/* A wrapper through which the board can be "looked at", so the bottom is seen */
int glook(struct board *it,int x, int y)
{
  if(y == -1)
    return 10;
  if(y < 0 || y > 5 || x < 0 || x > 6)
    return 10;
  return it->grid[x][y];
}	

/*
  void boardout(struct board *it)
  {
  int i;
  int j;
  fprintf(stderr,"\n");
  for(j=0;j<6;j++)
  {
  for(i=0;i<7;i++)
  fprintf(stderr," %d",it->grid[i][5-j]);
  fprintf(stderr,"\n");
  }
  }
*/
