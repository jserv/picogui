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

#include "battleship.h"

int board[4][10][10];
status aivalues;
int sizex, sizey,ss;
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

	
	
	pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
	pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("New Game"),0);
	pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evt_new_game,NULL);
	
/*	 pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_AFTER,PGDEFAULT);
	         pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Settings"),0);
		         pgBind(PGDEFAULT,PG_WE_ACTIVATE,&settings,NULL);

*/			 
	
	pgBind(canvas,PG_WE_BUILD,&redraw,NULL);

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
	if(pgMessageDialogFmt("Alert!",PG_MSGBTN_YES | PG_MSGBTN_NO,"Are you sure you want a new game?")== PG_MSGBTN_NO) return 0; 
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
	return 0;
}

int settings(struct pgEvent *evt)
{
	pgMessageDialogFmt("Settings",PG_MSGBTN_YES | PG_MSGBTN_NO | PG_MSGBTN_CANCEL , "GET NEW SETTINGS");
	return 0;
}

int clickski(struct pgEvent *evt)
{
	int guessx, guessy, direction, size;
 //	pgMessageDialogFmt("CLICK REGISTERED",PG_MSGBTN_YES,"CLICK REGISTERED AT %d, %d", evt->e.pntr.x,evt->e.pntr.y);
	guessx = evt ->e.pntr.x/ss;
	guessy = evt ->e.pntr.y/ss;
	if(guessx > 20 || guessy > 9 || guessx == 10) return 0;
	if(placement)
	{
		int i;
		size = placement;
		quoe(size);
		if(size == 1) size = 3;
		guessx--;
		guessy--;
		direction = pgMessageDialogFmt("Horizontal or Vertical?",PG_MSGBTN_YES | PG_MSGBTN_NO | PG_MSGBTN_CANCEL,"Yes for Vertical and No for Horizontal");
		if(direction == PG_MSGBTN_YES)
		{
			for(i=0;i<size;i++)
				if(board[PLAY_PIECE][guessy+i][guessx])
				{
					pgMessageDialogFmt("ERROR",PG_MSGBTN_YES,"Ships Shouldn't Share Spots");
					return 0;
				}
			if(guessy + size > 9) 
			{
				pgMessageDialogFmt("ERROR",PG_MSGBTN_YES,"You need to keep your ships on the board");
				return 0;
			}
			for(i=0;i<size;i++)
				board[PLAY_PIECE][guessy+i][guessx]= placement;
		}
		if(direction == PG_MSGBTN_NO)	
		{
			guessx++;
			guessy++;
			guessx -=11;
			for(i=0;i<size;i++)
                                if(board[PLAY_PIECE][guessy][guessx+i])
                                {
                                        pgMessageDialogFmt("ERROR",PG_MSGBTN_YES,"Ships Shouldn't Share Spots");
                                        return 0;
                                }
			if(guessx + size > 9)
			{
				 pgMessageDialogFmt("ERROR",PG_MSGBTN_YES,"You need to keep your ships on the board");
    				 return 0;
			}
                        for(i=0;i<size;i++)
                                board[PLAY_PIECE][guessy][guessx+i]= placement;
		}
		placement--;
		quoe(placement);
		
	}
	else
	{	
		quoe(0);
		if(guessx > 10) guessx -= 11;
	
		
		evt->e.size.w = sizex;
		evt->e.size.h = sizey;
		if(board[PLAY_GUESS][guessy][guessx])
		{
			pgMessageDialogFmt("ERROR!",PG_MSGBTN_YES,"You already guessed that spot!");
			return 0;
		}
		else
		{
			if(board[COMP_PIECE][guessy][guessx])
			{
				aivalues.play_points++;
				board[PLAY_GUESS][guessy][guessx] = HIT;
			}
			else
				board[PLAY_GUESS][guessy][guessx] = MISS;
		}
		
		redraw(evt);
		if(aivalues.play_points == 17)
		{
			pgBind(evt -> from,PG_WE_PNTR_DOWN,&clickski2,NULL);
			pgMessageDialogFmt("HORRAY!",PG_MSGBTN_OK,"YOU WIN BY %d POINTS",17-aivalues.comp_points);
		}
		aicall();
		if(aivalues.comp_points == 17)
		{
			redraw(evt);
			pgUpdate();
			pgBind(evt->from,PG_WE_PNTR_DOWN,&clickski2,NULL);
			pgMessageDialogFmt("BWAHAHAHA!",PG_MSGBTN_OK ,"YOU LOSE BY %d POINTS",17-aivalues.play_points);
		}
	}
	evt->e.size.w = sizex;
	evt->e.size.h = sizey;
	redraw(evt);
	pgUpdate();
	return 0;
}

int clickski2(struct pgEvent *evt)
{
	pgMessageDialogFmt("Alert!",PG_MSGBTN_OK,"Click the New Game button to play again.");
}

int redraw(struct pgEvent *evt)
{
	int x;
	int y;
	int i,j;
	sizex = evt->e.size.w;
	sizey = evt->e.size.h;
	x = evt->e.size.w / 21;
	y = evt->e.size.h / 10;
	
	if(x<y)y=x;
	else x=y;

	ss = x;
	
	//pgMessageDialogFmt("INFO",PG_MSGBTN_YES,"x=%d,y=%d",evt->e.size.w,evt->e.size.h);
	pgWriteCmd(evt->from,PGCANVAS_NUKE,0);
	
	/*Background*/
	pgWriteCmd(evt->from,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,BACKGROUND_COLOR);
	pgWriteCmd(evt->from,PGCANVAS_GROP,5,PG_GROP_RECT,0,0,evt->e.size.w,evt->e.size.h);

	/*Board*/
	for(i=0;i<21;i++)
	{
	if(i==10) i++;
	for(j=0;j<10;j++)
	{
		
		pgWriteCmd(evt->from,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,color_picker(j,i));
		pgWriteCmd(evt->from,PGCANVAS_GROP,5,
					  PG_GROP_RECT,i*x+1,j*y+1,x-1,y-1);
	}
	}
	pgWriteCmd(evt->from,PGCANVAS_REDRAW,0);
	pgUpdate();
	
	return 0;
}

/****************************** non-event stuff */

void comp_ship_place(int size)
{
	int x,y,i;
it:	
	if(rand() % 2 && size + x < 10)
	{
		x = rand() % (10 - size);
		y = rand() % 10;
		for(i=0;i<size;i++)
		       if(board[COMP_PIECE][x+i][y])	       
			       goto it;
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
		for(i=0;i<size;i++)
			if(board[COMP_PIECE][x][y+i])
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

int color_picker(int h, int w)
{
//	pgMessageDialogFmt("Location",PG_MSGBTN_YES,"h=%d,w=%d",h,w);
	if(w < 10)
	{
		switch(board[PLAY_GUESS][h][w])
		{
		case HIT:
			return HIT_COLOR;
			break;
		case MISS:
			return MISS_COLOR;
			break;
		default:
			return OCEAN_COLOR;
			break;
		}
	}
	else
	{
		w-=11;
		switch(board[PLAY_PIECE][h][w])
		{
		case CARRIER:
                        return CARRIER_COLOR;
                        break;
                case BATTLESHIP:
                        return BATTLESHIP_COLOR;
                        break;
                case SUBMARINE:
                        return SUBMARINE_COLOR;
                        break;
                case CRUISER:
                        return CRUISER_COLOR;
                        break;
                case DESTROYER:
                        return DESTROYER_COLOR;
                        break;
                case HIT_CARRIER:
                        return HIT_CARRIER_COLOR;
                        break;
                case HIT_BATTLESHIP:
                        return HIT_BATTLESHIP_COLOR;
                        break;
                case HIT_SUBMARINE:
                        return HIT_SUBMARINE_COLOR;
                        break;
                case HIT_CRUISER:
                        return HIT_CRUISER_COLOR;
                        break;
                case HIT_DESTROYER:
                        return HIT_DESTROYER_COLOR;
                        break;
                case MISS:
                        return MISS_COLOR;  
                        break;
                default:
                        return OCEAN_COLOR; 
                        break;
		}
	}
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
}
