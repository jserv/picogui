/*
 * Filename:  ai.c
 * Author:    Brandon Smith
 * Date:      March  23, 2002
 * Purpose:   the main AI functions
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

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "connectfour.h"
#include "ai.h"
#include "rules.h"

#define FUNCTION_DEBUG

void aicall(struct board *it)
{
  switch(it -> ailevel)
    {
    case 1:
      ai1(it);
      break;
    case 2:
      ai2(it);
      break;
    case 3:
      ai3(it);
      break;
    case 4:
      ai4(it);
      break;
    case 5:
      ai5(it);
      break;
    case 6:
      ai6(it);
      break;
    case 10:
      ai10(it);
      break;
    }

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"\n");
#endif
}

void randommove(struct board *it)
{
  int i = 0;
  
#ifdef FUNCTION_DEBUG
  fprintf(stderr,"randommove called\n");
#endif
  while(move(it,rand()%7) < 0);
  
}

int move(struct board *it, int location)
{
#ifdef FUNCTION_DEBUG
  fprintf(stderr,"move called - location = %d.\n",location);
#endif
  return putpiece(location,COMP,it);
}
