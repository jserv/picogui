/* 
 * Filename: battleship.c
 *
 * Copyright (C) Brandon Smith 2000 (lottabs2@yahoo.com)
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
 * Contributers
 *
 *
 *
 *
 */

#include <picogui.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "battleship.h"

int board[4][10][10];
char games = 0;
status aivalues;
int sizex, sizey,ss,xleft,yleft;
int picked_color = 1;
char placement = 0;
pghandle dire;
pghandle canvas;

int main(int argc, char **argv)
{
  
  pghandle toolbar;
  
  
  //memset(board,0,sizeof(board));
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"BATTLESHIP!",0);
  
  toolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  
  canvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);

  srand(time(NULL));
  
  
  
  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("New Game"),0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evt_new_game,NULL);
  
  /*	 pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
	 pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Settings"),0);
	 pgBind(PGDEFAULT,PG_WE_ACTIVATE,&settings,NULL);
	 
  */			 
  
  pgBind(canvas,PG_WE_BUILD,&redraw,NULL);
  pgBind(canvas,PG_WE_PNTR_DOWN,&clickski2,NULL);
  
  dire = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_ALL,PG_WP_TRANSPARENT,0,PG_WP_ALIGN,PG_A_CENTER,PG_WP_FONT,pgNewFont(NULL,0,PG_FSTYLE_BOLD),0);
  quoe(10);
  pgUpdate();
  pgEventLoop();
  
  return 0;
}

int evt_new_game(struct pgEvent *evt)
{
  int i;
  /*if(pgMessageDialogFmt("Alert!",PG_MSGBTN_YES | PG_MSGBTN_NO,"Are you sure you want a new game?")== PG_MSGBTN_NO) return 0; */
  memset(board,0,sizeof(board));
  memset(&aivalues,0,sizeof(status));
  aivalues.play_ships[CRUISER] = -2;
  aivalues.comp_ships[CRUISER] = -2;
  aivalues.status = 1;
  placement = 5;
  quoe(5);
  pgBind(canvas,PG_WE_PNTR_DOWN,&clickski,evt);
  /* Place the comp ships */
  for(i=5;i;i--)
    comp_ship_place(i);
  evt->e.size.w = sizex;
  evt->e.size.h = sizey;
  evt->from = canvas;
  redraw(evt);
  picked_color = 1;
  return 0;
}


int clickski(struct pgEvent *evt)
{
  int guessx, guessy, direction, size;

  guessx = (evt ->e.pntr.x-xleft)/ss;
  guessy = (evt ->e.pntr.y-yleft)/ss;

  evt->e.size.w = sizex;
  evt->e.size.h = sizey;

  if(guessx > 20 || guessy < 0 || guessx == 10) return 0;
  if(guessx > 10) guessx -= 11;

  //draw_miss(guessx,guessy);
  fprintf(stderr,"you guessed x=%d and y=%d\n",guessx,guessy);
  //pgMessageDialogFmt("DEBUG",PG_MSGBTN_OK,"X location %d, Y location %d",guessx,guessy);

  if(placement)
    {
      int i;
      size = placement;
      quoe(size);
      if(size == 1) size = 3;
      direction = pgMessageDialogFmt("Horizontal or Vertical?",PG_MSGBTN_YES | PG_MSGBTN_NO | PG_MSGBTN_CANCEL,"Yes for Vertical and No for Horizontal");

      if(direction == PG_MSGBTN_YES)
	{
	  if(guessy + size > 10)
	    {	
	      pgMessageDialogFmt("ERROR",PG_MSGBTN_OK,"You need to keep your ships on the board");
	      return 0;
	    }
	  for(i=0;i<size;i++)
	    if(board[PLAY_PIECE][guessy+i][guessx])
	      {
		pgMessageDialogFmt("ERROR",PG_MSGBTN_OK,"Ships Shouldn't Share Spots");
		return 0;
	      }
	  for(i=0;i<size;i++)
	    {
	      board[PLAY_PIECE][guessy+i][guessx]= placement;
	      draw_ship(guessx+11,guessy+i,placement);
	    }
	}
      if(direction == PG_MSGBTN_NO)	
	{
	  if(guessx + size > 10)
	    {
	      pgMessageDialogFmt("ERROR",PG_MSGBTN_OK,"You need to keep your ships on the board");
	      return 0;
	    }
	  for(i=0;i<size;i++)
	    if(board[PLAY_PIECE][guessy][guessx+i])
	      {
		pgMessageDialogFmt("ERROR",PG_MSGBTN_OK,"Ships Shouldn't Share Spots");
		return 0;
	      }
	  for(i=0;i<size;i++)
	    {
	      board[PLAY_PIECE][guessy][guessx+i]= placement;
	      draw_ship(guessx+i+11,guessy,placement);
	    }
	}
      if(direction == PG_MSGBTN_CANCEL) placement ++;
      placement--;
      quoe(placement);
      
    }
  else
    {	
      quoe(0);
      if(board[PLAY_GUESS][guessy][guessx])
	{
	  pgMessageDialogFmt("ERROR!",PG_MSGBTN_OK,"You already guessed that spot!");
	  return 0;
	}
      else
	{
	  if(board[COMP_PIECE][guessy][guessx])
	    {
	      //pgMessageDialogFmt("DEBUG",PG_MSGBTN_OK,"-%d--%d-",aivalues.comp_ships[board[COMP_PIECE][guessy][guessx]],board[COMP_PIECE][guessy][guessx]);
	      fprintf(stderr,"YOU GOT A HIT");
	      aivalues.play_points++;
	      aivalues.comp_ships[board[COMP_PIECE][guessy][guessx]]++;
	      if(aivalues.comp_ships[board[COMP_PIECE][guessy][guessx]]+1 == aivalues.comp_ships[board[COMP_PIECE][guessx][guessy]])
		{
		  switch(aivalues.comp_ships[board[COMP_PIECE][guessx][guessy]])
		    {
		    case 1:
		      pgMessageDialogFmt("SHIP SUNK",PG_MSGBTN_OK,"You sunk the Cruiser");
		      break;
		    case 2:
		      pgMessageDialogFmt("SHIP SUNK",PG_MSGBTN_OK,"You sunk the Destroyer");
		      break;
		    case 3:
		      pgMessageDialogFmt("SHIP SUNK",PG_MSGBTN_OK,"You sunk the Submarine");
		      break;
		    case 4:
		      pgMessageDialogFmt("SHIP SUNK",PG_MSGBTN_OK,"You sunk the Battleship");
		      break;
		    case 5:
		      pgMessageDialogFmt("SHIP SUNK",PG_MSGBTN_OK,"You sunk the Carrier");
		      break;
		    }
		}
	      board[PLAY_GUESS][guessy][guessx] = HIT;
	      board[COMP_PIECE][guessy][guessx] +=5;
	    }
	  else
	    {
	      board[PLAY_GUESS][guessy][guessx] = MISS;
	      board[COMP_PIECE][guessy][guessx] = MISS;
	    }
	}
      
      if(aivalues.play_points == 17)
	{
	  pgBind(evt -> from,PG_WE_PNTR_DOWN,&clickski2,NULL);
	  quoe(10);
	  picked_color = 0;
	  //redraw(evt);
	  draw_spot(guessx,guessy,PLAY_GUESS);
	  pgWriteCmd(canvas,PGCANVAS_REDRAW,0);
	  pgMessageDialogFmt("HORRAY!",PG_MSGBTN_OK,"YOU WIN BY %d POINTS",17-aivalues.comp_points);
	  picked_color = 1;
	  return 0;
	}
      draw_spot(guessx,guessy,PLAY_GUESS);
      pgWriteCmd(canvas,PGCANVAS_REDRAW,0);
      aicall();
      if(aivalues.comp_points == 17)
	{
	  //	  redraw(evt);
	  //	  pgUpdate();
	  pgBind(evt->from,PG_WE_PNTR_DOWN,&clickski2,NULL);
	  draw_spot(guessx,guessy,PLAY_GUESS);
	  pgWriteCmd(canvas,PGCANVAS_REDRAW,0);
	  pgMessageDialogFmt("ERROR",PG_MSGBTN_OK ,"YOU NEED TO KEEP YOUR SHIPS");
	  quoe(10);
	  picked_color = 0;
	}
    }
  draw_spot(guessx,guessy,PLAY_GUESS);
  pgWriteCmd(canvas,PGCANVAS_REDRAW,0);
  //pgUpdate();
  picked_color = 1;
  return 0;
}

int clickski2(struct pgEvent *evt)
{
  if(games)
    pgMessageDialogFmt("Alert!",PG_MSGBTN_OK,"Click the New Game button to play again.");
  else
    pgMessageDialogFmt("Alert!",PG_MSGBTN_OK,"Click the New Game button to play.");
  games++;
}

int redraw(struct pgEvent *evt)
{
  //Store the x and y sizes;
  int x;
  int y;

  //incrementers
  int i;
  int j;

  fprintf(stderr,"FULL REDRAW CALLED FOR\n");

  sizex = evt->e.size.w;
  sizey = evt->e.size.h;
  x = evt->e.size.w / 21;
  y = evt->e.size.h / 10;

  if(x<y)
    y=x;
  else
    x=y;
  
  ss = x;
  
  xleft = (sizex - (21*ss)) / 2;
  yleft = (sizey - (10*ss)) / 2;


  pgWriteCmd(evt->from,PGCANVAS_NUKE,0);
  /**Background
     What I do here is I set the background to white, and everything after that is black
     so to save time, I just set it to black after whiting out the background.
  **/

  pgWriteCmd(evt->from,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,0xFFFFFF);
  pgWriteCmd(evt->from,PGCANVAS_GROP,5,PG_GROP_RECT,0,0,sizex,sizey);

  //  pgWriteCmd(evt->from,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,0xFFFFFF);
  //pgWriteCmd(evt->from,PGCANVAS_GROP,5,PG_GROP_RECT,0,0,evt->e.size.w,evt->e.size.h);
  pgWriteCmd(evt->from,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,0x000000);

  /*Board*/
  
  //vertical lines
  for(i=0;i<22;i++)
    {
      pgWriteCmd(evt->from,PGCANVAS_GROP,5,PG_GROP_LINE, xleft+(i*x),yleft,0,y*10);
    }
  
  for(i=0;i<11;i++)
    {
      //Left board
      pgWriteCmd(evt->from,PGCANVAS_GROP,5,PG_GROP_LINE,xleft,yleft+(y*i),x*10,0);
      
      //Right board
      pgWriteCmd(evt->from,PGCANVAS_GROP,5,PG_GROP_LINE,xleft+(x*11),yleft+(y*i),x*10,0);
    }

  
  //Draw the player pieces if they are there
  for(i=0;i<10;i++)
    {
      for(j=0;j<10;j++)
	{
	  draw_spot(i,j,PLAY_GUESS);
	  draw_spot(i,j,PLAY_PIECE);
	}
    }

  //Draw the comp pieces if they are there

  pgWriteCmd(evt->from,PGCANVAS_REDRAW,0);
  // pgUpdate();
  
  return 0;
}

/****************************** non-event stuff */

void comp_ship_place(int size)
{
  int x,y,i;
 it:	
  if(rand() % 2)
    {
      x = rand() % (10 - size);
      y = rand() % 10;
      if(size != 1) for(i=0;i<size+1;i++)
	if(board[COMP_PIECE][x+i][y])	       
	  goto it;
	else if(size == 1) for(i=0;i<4;i++)
	  if(board[COMP_PIECE][x+i][y])
	    if(size != 1)
	      {
		for(i=0;i<size;i++)
		  board[COMP_PIECE][x+i][y] = size;
		return;
	      }
	    else if(size == 1)
	      {
		for(i=0;i<3;i++)
		  board[COMP_PIECE][x+i][y] = size;
		return;
	      }
    }
  else 
    {
      x = rand() % 10;
      y = rand() % (10 - size);
      if(size != 1) for(i=0;i<size+1;i++)
	if(board[COMP_PIECE][x][y+i])
	  goto it;
	else if(size == 1) for(i=0;i<4;i++)
	  if(board[COMP_PIECE][x][y+1])
	    goto it;
      if(size != 1)
	{
	  for(i=0;i<size;i++)
	    board[COMP_PIECE][x][y+i] = size;
	  return;
	}
      else if(size == 1)
	{
	  for(i=0;i<3;i++)
	    board[COMP_PIECE][x][y+i] = size;
	  return;
	}
    }
  goto it;
}
  
void draw_spot (int x, int y, int side)
{
  switch(side)
    {
    case COMP_GUESS:
      //      fprintf(stderr,"draw_spot called with %d and %d for COMP_GUESS\n",x%11,y);
      break;
    case COMP_PIECE:
      //      fprintf(stderr,"draw_spot called with %d and %d for COMP_PIECE\n",x%11,y);
      break;
    case PLAY_GUESS:
      //fprintf(stderr,"draw_spot called with %d and %d for PLAY_GUESS\n",x%11,y);
      if(board[side][y][x%11] == HIT)
	draw_hit(x,y);
      if(board[side][y][x%11] == MISS)
	draw_miss(x,y);
      break;
    case PLAY_PIECE:
      //fprintf(stderr,"draw_spot called with %d and %d for PLAY_PIECE -- %d\n",x,y,board[side][y][x%11]);
      if(board[side][y][x%11] == MISS)
	draw_miss(x+11,y);
      if(board[side][y][x%11] <= 5 && board[side][y][x%11] >= 1)
	draw_ship(x+11,y,board[side][x][y]);
      if(board[side][y][x%11] <= 10 && board[side][y][x%11] >= 6)
	draw_hit_ship(x+11,y);
      break;
    }
}

void draw_miss(int x, int y)
{
  int bit = ss >> 3;

  //This is a simple X

  //forward slash
  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,(x*ss)+xleft+bit,(y*ss)+yleft+bit,ss-(2*bit),ss-(2*bit));
  //back slash
  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,(x*ss)+xleft+bit,((y+1)*ss)+yleft-bit,ss-(2*bit),-ss+(2*bit));
  
  //pgWriteCmd(canvas,PGCANVAS_REDRAW,0);
  
  //pgSubUpdate(canvas);

}

void draw_hit(int x, int y)
{
  int unit =  ss >> 2;
  int center = ss >> 1;
 
  //pgMessageDialogFmt("Alert!",PG_MSGBTN_OK,"ss = %d. ss/2 = %d. ss/4 = %d",ss,halfss,quarterss);
  
  //This is a dumbed down form of an H
  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,(x*ss)+xleft-unit+center,(y*ss)+yleft-unit+center,0,center);
  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,(x*ss)+xleft+unit+center,(y*ss)+yleft-unit+center,0,center);

  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,(x*ss)+xleft-unit+center,(y*ss)+yleft+center,2*unit,0);
  //pgWriteCmd(canvas,PGCANVAS_REDRAW,0);
  //pgSubUpdate(canvas);
}

void draw_ship(int x, int y, int type)
{
  int unit = ss >> 2;
  int center = ss >> 1;

  //This is an empty box
  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,(x*ss)+xleft-unit+center,(y*ss)+yleft-unit+center,0,2*unit);
  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,(x*ss)+xleft-unit+center,(y*ss)+yleft-unit+center,2*unit,0);

  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,(x*ss)+xleft+unit+center,(y*ss)+yleft+unit+center,-2*unit,0);
  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,(x*ss)+xleft+unit+center,(y*ss)+yleft+unit+center,0,-2*unit);
  //pgWriteCmd(canvas,PGCANVAS_REDRAW,0);

  // pgSubUpdate(canvas);
}

void draw_hit_ship(int x, int y)
{
  int unit = ss >> 2;
  int center = ss >> 1;

  //This is a filled box
  pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_RECT,(x*ss)+xleft + (center - unit),(y*ss)+yleft-unit+center,2*unit,2*unit);
  pgWriteCmd(canvas,PGCANVAS_REDRAW,0);
  //pgSubUpdate(canvas);
}

void quoe(int x)
{
  switch(x)
    {
    case 0:
    default:
      pgReplaceTextFmt(dire,"Guess A Location(Board on the left)");
      break;
    case 1:
      pgReplaceTextFmt(dire,"Place Your Cruiser   Size = 3(Board on the Right)");
      break;
    case 2:
      pgReplaceTextFmt(dire,"Place Your Destroyer   Size = 2(Board on the Right)");
      break;
    case 3:
      pgReplaceTextFmt(dire,"Place Your Submarine   Size = 3(Board on the Right)");
      break;
    case 4:
      pgReplaceTextFmt(dire,"Place Your Battleship   Size = 4(Board on the Right)");
      break;
    case 5:
      pgReplaceTextFmt(dire,"Place Your Carrier   Size = 5(Board on the Right)");
      break;
    case 10:
      pgReplaceTextFmt(dire,"Click NEW GAME to start a new game");
    }
  pgUpdate();
}

/****************************** AI STUFF */

void aicall(void)
{
  int i = 1, alpha = 0, numeric = 0;
 begin:
  i = 1;
  switch(aivalues.status)
    {
    case 1:
    default:
      alpha = rand() % 10;
      numeric = rand() % 10;
      if((alpha + numeric) % 2) goto begin;
      if(!board[COMP_GUESS][alpha][numeric])
	{
	  if(board[PLAY_PIECE][alpha][numeric])
	    {
	      aivalues.letter = alpha;
	      aivalues.number = numeric;
	      aivalues.status = 5;
	      aivalues.comp_points++;
	      aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]]++;
	      if(board[PLAY_PIECE][alpha][numeric] == aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]])
		aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]] = SUNK;
	      board[COMP_GUESS][alpha][numeric] = HIT;
	      board[PLAY_PIECE][alpha][numeric] +=5;
	    }
	  else
	    {
	      board[COMP_GUESS][alpha][numeric] = MISS;
	      board[PLAY_PIECE][alpha][numeric] = MISS;
	    }
	}
      else goto begin;
      break;
    case 41:
      i++;
    case 37:
      i++;
    case 33:
      i++;
    case 29:
      i++;
    case 25:
      i++;
    case 21:
      i++;
    case 17:
      i++;
    case 13:
      i++;
    case 9:
      i++;
    case 5:
      alpha = aivalues.letter;
      numeric = aivalues.number;
      alpha +=i;
      
      if(alpha > 9)
	{
	  if(i>1)
	    {
	      aivalues.status = 3;
	      goto begin;
	    }
	  aivalues.status = 4;
	  goto begin;
	}
      if(!board[COMP_GUESS][alpha][numeric])
	{
	  if(board[PLAY_PIECE][alpha][numeric])
	    {
	      aivalues.status +=4;
	      aivalues.comp_points++;
	      aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]]++;
	      if(board[PLAY_PIECE][alpha][numeric] == aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]])
		aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]] = SUNK;
	      board[COMP_GUESS][alpha][numeric] = HIT; 
	      board[PLAY_PIECE][alpha][numeric] += 5;
	    }
	  else
	    {
	      board[COMP_GUESS][alpha][numeric] = MISS;
	      board[PLAY_PIECE][alpha][numeric] = MISS;
	      aivalues.status = 4;
	      if(i>1) aivalues.status = 3;
	    }
	}
      else if(board[COMP_GUESS][alpha][numeric] == HIT)
	{
	  aivalues.status +=4;
	  goto begin;
	}
      else
	{
	  if(i>1)
	    {
	      aivalues.status = 3;
	      goto begin;
	    }
	  aivalues.status = 4;
	  goto begin;
	}
      break;
    case 40:
      i++;
    case 36:
      i++;
    case 32:
      i++;
    case 28:
      i++;
    case 24:
      i++;
    case 20:
      i++;
    case 16:
      i++;
    case 12:
      i++;
    case 8:
      i++;
    case 4:
      alpha = aivalues.letter;
      numeric = aivalues.number;
      numeric +=i;
      
      if(numeric > 9)
	{
	  if(i>1)
	    {
	      aivalues.status =2;
	      goto begin;
	    }
	  aivalues.status = 3;
	  goto begin;
	}
      if(!board[COMP_GUESS][alpha][numeric])
	{
	  if(board[PLAY_PIECE][alpha][numeric])
	    {
	      aivalues.status +=4;
	      aivalues.comp_points++;
	      aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]]++;
	      if(board[PLAY_PIECE][alpha][numeric]==aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]])
		aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]] = SUNK;
	      board[COMP_GUESS][alpha][numeric] = HIT;
	      board[PLAY_PIECE][alpha][numeric] +=5;
	    }
	  else
	    {
	      board[COMP_GUESS][alpha][numeric] = MISS;
	      board[PLAY_PIECE][alpha][numeric] = MISS;
	      aivalues.status = 3;
	      if(i>1) aivalues.status = 2;
	    }
	}
      else if(board[COMP_PIECE][alpha][numeric] == HIT)
	{
	  aivalues.status +=4;
	  goto begin;
	}
      else
	{
	  if(i>1)
	    {
	      aivalues.status = 2;
	      goto begin;
	    }
	  aivalues.status = 3;
	  goto begin;
	}
      break;
    case 39:
      i++;	
    case 35:
      i++;
    case 31:
      i++;
    case 27:
      i++;
    case 23:
      i++;
    case 19:
      i++;
    case 15:
      i++;
    case 11:
      i++;
    case 7:
      i++;
    case 3:
      alpha = aivalues.letter;
      numeric = aivalues.number;
      alpha -=i;
      if(alpha < 0)
	{
	  aivalues.status = 2;
	  goto begin;
	}
      if(!board[COMP_GUESS][alpha][numeric])
	{
	  if(board[PLAY_PIECE][alpha][numeric])
	    {
	      
	      aivalues.status +=4;
	      aivalues.comp_points++;
	      aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]]++;
	      if(board[PLAY_PIECE][alpha][numeric]==aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]])
		aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]] = SUNK;
	      board[COMP_GUESS][alpha][numeric] = HIT;
	      board[PLAY_PIECE][alpha][numeric] +=5;
	    }
	  else
	    {
	      board[COMP_GUESS][alpha][numeric] = MISS;
	      board[PLAY_PIECE][alpha][numeric] = MISS;
	      aivalues.status = 2;
	    }
	}
      else if(board[COMP_GUESS][alpha][numeric] == HIT)
	{
	  aivalues.status +=4;
	  goto begin;
	}
      else
	{
	  aivalues.status = 2;
	  goto begin;
	}
      break;
    case 38:
      i++;
    case 34:
      i++;
    case 30:
      i++;
    case 26:
      i++;
    case 22:
      i++;
    case 18:
      i++;
    case 14:
      i++;
    case 10:
      i++;
    case 6:
      i++;
    case 2:
      alpha = aivalues.letter;
      numeric = aivalues.number;
      numeric -=i;
      if(numeric < 0)
	{
	  aivalues.status = 1;
	  goto begin;
	}
      if(!board[COMP_GUESS][alpha][numeric])
	{
	  if(board[PLAY_PIECE][alpha][numeric])
	    {	
	      aivalues.status +=4;
	      aivalues.comp_points++;
	      aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]]++;
	      if(board[PLAY_PIECE][alpha][numeric] == aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]])
		aivalues.play_ships[board[PLAY_PIECE][alpha][numeric]] = SUNK;
	      board[COMP_GUESS][alpha][numeric] = HIT;
	      board[PLAY_PIECE][alpha][numeric] +=5;
	    }
	  else
	    {
	      board[COMP_GUESS][alpha][numeric] = MISS;
	      board[PLAY_PIECE][alpha][numeric] = MISS;
	      aivalues.status = 1;
	    }
	}
      else if(board[COMP_GUESS][alpha][numeric] == HIT)
	{
	  aivalues.status +=4;
	  goto begin;
	}
      else
	{
	  aivalues.status =1;
	  goto begin;
	}
      break;
    }
  draw_spot(numeric,alpha,PLAY_PIECE);
}
