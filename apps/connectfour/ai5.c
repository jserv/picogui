/*
 * Filename:  ai5.c
 * Author:    Brandon Smith
 * Date:      April 3, 2002
 * Purpose:   the Fifth level AI
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
#include "ai4.h"
#include "ai5.h"

#define DEBUG
#define FUNCTION_DEBUG

void ai5(struct board *it)
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

  temp = linetrap(it);
#ifdef DEBUG
  fprintf(stderr,"linetrap returned %d\n",temp);
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

  prandmove(it);
}

void prandmove(struct board *it)
{
  int i = 0;

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"prandmove called\n");
#endif

  while(++i < 20)
    if(!(move(it,rand()%3 + it->aipref) < 0))
      return;
  
  randommove(it);
}
