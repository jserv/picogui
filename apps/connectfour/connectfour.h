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
#include "ai.h"

#define COMP -1;
#define HUMAN 1;

/* Event Handlers */
int NewGame(struct pgEvent *evt);
int piecedrop(struct pgEvent *evt);

/* Non-Event stuff */
int redraw();
int putpiece(int,int,struct pgEvent *evt);
void drawpiece(int x, int y, int type);
void win(int x, int y, int direction);

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

  /* Store the pointer for the AI's structure stuff */
  /* aidata *info;
   */

}board;

#endif /* __CONNECTFOUR_H__ */

