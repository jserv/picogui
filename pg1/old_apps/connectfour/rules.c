/*
 * Filename:  rules.c
 * Author:    Brandon Smith
 * Date:      March 23, 2002
 * Purpose:   the functions that dictate the "rules" of the game
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
#include "rules.h"

int wincheck(struct board *foo)
{
  int x, y, i;

  //zero slope
  for(x=0;x<4;x++)
    for(y=0;y<6;y++)
     if(foo->grid[x][y] == foo->grid[x+1][y] && foo->grid[x][y] == foo->grid[x+2][y] &&
	 foo->grid[x][y] == foo->grid[x+3][y] && foo->grid[x][y] != 0)
	return 100 + x*10 + y;
  
  //no slope
  for(x=0;x<7;x++)
    for(y=0;y<3;y++)
      if(foo->grid[x][y] == foo->grid[x][y+1] && foo->grid[x][y] == foo->grid[x][y+2] &&
	 foo->grid[x][y] == foo->grid[x][y+3] && foo->grid[x][y] != 0)
	return 200 + x*10 + y;
  
  //positive slope
  for(x=0;x<4;x++)
    for(y=0;y<3;y++)
      if(foo->grid[x][y] == foo->grid[x+1][y+1] && foo->grid[x][y] == foo->grid[x+2][y+2] &&
	 foo->grid[x][y] == foo->grid[x+3][y+3] && foo->grid[x][y] != 0)
	return 300 + x*10 + y;
  
  //negative slope
  for(x=0;x<4;x++)
    for(y=3;y<6;y++)
      if(foo->grid[x][y] == foo->grid[x+1][y-1] && foo->grid[x][y] == foo->grid[x+2][y-2] &&
	 foo->grid[x][y] == foo->grid[x+3][y-3] && foo->grid[x][y] != 0)
	return 400 + x*10 +y;
  
  //check for a "cat's" game
  for(x=0,i=0;x<7;x++)
    for(y=0;y<6;y++)
      if(foo -> grid[x][y])
	i++;
  
  if(i == 42)
    return 1;
  
  return 0;
}
