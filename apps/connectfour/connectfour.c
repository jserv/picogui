/*
 * Filename:  connectfour.c
 * Author:    Brandon Smith
 * Date:      March 23, 2002
 * Purpose:   A PicoGUI implementation of the game connectfour
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
 * This code borrows heavily from blackout written by Micah Dowty
 *
 */

#include <picogui.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "connectfour.h"
#include "ai.h"
#include "rules.h"

pghandle info;
pghandle canvas;

int main(int argc, char **argv)
{ 
  pghandle bar;
 
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Connect Four",0);
   
  bar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  canvas  = pgNewWidget(PG_WIDGET_CANVAS,0,0);
  
  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,bar);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("New Game"),
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,0);
  
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&NewGame,NULL);
  
  info = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_ALL,PG_WP_TRANSPARENT,0,
	      PG_WP_ALIGN,PG_A_CENTER,PG_WP_FONT,
	      pgNewFont(NULL,0,PG_FSTYLE_BOLD),0);
  
  pgReplaceTextFmt(info,"Click New Game to pick a level.");
  
  pgEventLoop();
}



/**
 * Event Handlers
 */

/* New Game and AI setter */
int NewGame(struct pgEvent *evt)
{
  switch(pgMenuFromString("Skill Level One|Skill Level Two|Skill Level Three|Skill Level Four"))
    {
    case 1:
      pgReplaceTextFmt(info,"Skill level One");
      break;
    case 2:
      pgReplaceTextFmt(info,"Skill level Two");
      break;
    case 3:
      pgReplaceTextFmt(info,"Skill level Three");
      break;
    case 4:
      pgReplaceTextFmt(info,"Skill Level Four");
      break;
    }
  if(!(evt->extra == NULL)) free(evt -> extra);
  evt -> extra = malloc(sizeof(board));
  memset(evt -> extra,0,sizeof(board)); 
  pgBind(canvas,PG_WE_PNTR_DOWN,&piecedrop,evt->extra);
  redraw(); 
  return 0;
}

/* redraw the canvas */
int redraw()
{
  pgWriteCmd(canvas,PGCANVAS_NUKE,0);
  pgWriteCmd(canvas,PGCANVAS_DEFAULTFLAGS,1,0);

  pgWriteCmd(canvas,PGCANVAS_GROP,6,PG_GROP_SETMAPPING,0,0,72,62,PG_MAP_SQUARESCALE);
  pgWriteCmd(canvas,PGCANVAS_GROPFLAGS,1,PG_GROPF_UNIVERSAL);
  
  /* If I wanted to map the input I get to the thing I have, this is the command I would use*/
  pgWriteCmd(canvas,PGCANVAS_INPUTMAPPING,5,0,0,72,62,PG_MAP_SQUARESCALE); 
  
  {
    int i,j;

    for(i=0;i<8;i++)
      pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,i*10+1,1,0,60);
    for(i=0;i<7;i++)
      pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_LINE,1,i*10+1,70,0);
  }

  //pgWriteCmd(canvas,PGCANVAS_DEFAULTFLAGS,1,PG_GROPF_PSEUDOINCREMENTAL);
  return 0;
}

int piecedrop(struct pgEvent *evt)
{
 
  /* store the location of the click */
  int loc;
  fprintf(stderr,"Clickski at x = %d y = %d\n",evt -> e.pntr.x, evt -> e.pntr.y);
  
  //make sure it is properly formatted
  if(evt->e.pntr.x < 0 || evt->e.pntr.x > 70)
    return 0;
  
  loc = (evt->e.pntr.x-1) / 10;
  
  fprintf(stderr,"%d\n",loc);

  /*if(evt->e.pntr.btn == 1)
    drawpiece(loc,(evt->e.pntr.y-1) / 10,-1);
  else
    drawpiece(loc,(evt->e.pntr.y-1) / 10,1);
  */
  return putpiece(loc,1,evt);
}


/****
     non-event stuff
*/
				       
int putpiece(int location, int type, struct pgEvent *evt)
{
  board *foo = evt->extra;
  int i = 0;

  //find the next available spot on the board to fill
  if(foo->grid[location][5] == 0)
     while(foo->grid[location][i]!=0) i++;
  else
    return -1;
  
  fprintf(stderr,"%d\n",i);
  foo->grid[location][i] = 1;
  drawpiece(location,i,type);
  return i;
}

void drawpiece(int x, int y, int type)
{
  //actually draw the dern thing.
  if(type<0)
    pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_FELLIPSE,2+(x*10),2+((5-y)*10),8,8);
  else
    pgWriteCmd(canvas,PGCANVAS_GROP,5,PG_GROP_ELLIPSE,2+(x*10),2+((5-y)*10),8,8);

  pgWriteCmd(canvas,PGCANVAS_GROPFLAGS,1,PG_GROPF_PSEUDOINCREMENTAL);
  pgWriteCmd(canvas,PGCANVAS_INCREMENTAL,0);
}

void win(int x, int y, int direction)
{
  
}
