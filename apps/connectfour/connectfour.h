/*
 * Filename:  connectfour.h
 * Author:    Brandon Smith
 * Date:      March 23, 2002
 * Purpose:   A PicoGUI implementation of Connect Four
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


#ifndef __CONNECTFOUR_H__
#define __CONNECTFOUR_H__

#include <picogui.h>

#define COMP -1
#define HUMAN 1

/* Event Handlers */
int NewGame(struct pgEvent *evt);
int piecedrop(struct pgEvent *evt);
int dummy(struct pgEvent *evt);


/* Non-Event stuff */
int redraw();
void drawpiece(int x, int y, int type);
void victoryline(int x, int y, int direction);


typedef struct board
{
  /*
   *  (0,5) (1,5) ... (6,5)
   *  (0,4) (1,4) ... (6,4)
   *  ...
   *  (0,0) (1,0) ... (6,0)
   * 
   * This is a first quadrant (x,y) based grid layout
   */

  int grid[7][6];

  /* Stores the AI level */

  int ailevel;

  /* Stores the random move preference for AI levels 5 and up */
  
  int aipref;

  // temp pointer
  
  struct board *it;

}board;

void catsgame(struct board *it);
void win(struct board *it, int x, int y, int direction);
void lose(struct board *it, int x, int y, int direction);
void endofgame(struct board *it);
int putpiece(int location,int type,struct board *foot);

#endif /* __CONNECTFOUR_H__ */

