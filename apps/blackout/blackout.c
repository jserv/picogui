/* $Id: blackout.c,v 1.2 2001/01/31 04:17:24 micahjd Exp $
 *
 * blackout.c - "Blackout" game to demonstrate game programming and
 *              canvas widget event handling.
 *              I like this game- very simple to write but it still
 *              always manages to get really hard around level 8.
 *              ;-)
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Contributors: 
 * 
 * 
 * 
 */

#include <picogui.h>

/* Position of the board within our canvas */
int bx,by,bs,lightw,lighth;

/* Widgets */
pghandle wCanvas,wToolbar,wStatusLabel;

/* Game board array */
int boardwidth, boardheight, boardsize;
typedef unsigned char light;
light *board;
#define LIGHT(x,y)     board[(x)+(y)*boardwidth]
#define LIGHTGROP(x,y) (1+(x)+(y)*boardwidth)  

/* Colors */
#define ON_COLOR      0xFFFF80
#define OFF_COLOR     0x404040

/* Scorekeeping */
int moves;
int level;

/****************************** Utility functions ***/

/* Set the state of the specified light gropnode */
void setLight(int lx,int ly,light l) {
   pgWriteCmd(wCanvas,PGCANVAS_FINDGROP,1,LIGHTGROP(lx,ly));
   pgWriteCmd(wCanvas,PGCANVAS_SETGROP,1,l ? ON_COLOR : OFF_COLOR);
   pgWriteCmd(wCanvas,PGCANVAS_COLORCONV,1,1);
   pgWriteCmd(wCanvas,PGCANVAS_GROPFLAGS,1,PG_GROPF_PSEUDOINCREMENTAL);
}

/* Toggle the light at the specified coordinates and update the groplist */
void invertLight(int lx,int ly) {
   light l;

   if (lx < 0 || lx >= boardwidth ||
       ly < 0 || ly >= boardheight)
     return;
   l = LIGHT(lx,ly) = !LIGHT(lx,ly);
   setLight(lx,ly,l);
}

/* Toggle the light at the specified coordinates 
 * without updating the groplist. This is for setting up the board. */
void invertLightNoupdate(int lx,int ly) {
   if (lx < 0 || lx >= boardwidth ||
       ly < 0 || ly >= boardheight)
     return;
   LIGHT(lx,ly) = !LIGHT(lx,ly);
}

/* Update the status display with current parameters */
void updateStatus(void) {
   pgReplaceTextFmt(wStatusLabel,"Level %d, Move #%d",level,moves);
   pgUpdate();
}

/* Start a new level */
void startLevel(int newlev) {
   int i,j,x;
   light *p = board;
   
   /* Set up variables */
   moves = 0;
   level = newlev;
   memset(board,0,boardsize);

   /* Make random moves */
   for (x=level;x;x--) {
      i = random() % boardwidth;
      j = random() % boardheight;
      invertLightNoupdate(i,j);
      invertLightNoupdate(i-1,j);
      invertLightNoupdate(i+1,j);
      invertLightNoupdate(i,j-1);
      invertLightNoupdate(i,j+1);
   }
      
   /* Set the gropnode states */
   for (j=0;j<boardheight;j++)
     for (i=0;i<boardwidth;i++)
      setLight(i,j,*(p++));
   pgWriteCmd(wCanvas,PGCANVAS_INCREMENTAL,0);
   
   /* Update the status message and run pgUpdate() */
   updateStatus();
}

/****************************** Event handlers ***/

/* Redraw the game board */
int evtDrawBoard(short event, pghandle from, long param) {
   int x,y,i,j;
   light *p = board;
   if (PG_W<PG_H)

     bs = PG_W-2;
   else
     bs = PG_H-2;
   bx = (PG_W-bs)>>1;
   by = (PG_H-bs)>>1;
   lightw = bs/boardwidth;
   lighth = bs/boardheight;
   /* Get rid of remainder */
   bs = boardwidth*lightw;
   
   /* Clear the groplist */
   pgWriteCmd(from,PGCANVAS_NUKE,0);
   
   /* Black game board background */
   pgWriteCmd(from,PGCANVAS_GROP,5,PG_GROP_RECT,0,0,PG_W,PG_H);
   pgWriteCmd(from,PGCANVAS_SETGROP,1,0x000000);
   pgWriteCmd(from,PGCANVAS_COLORCONV,1,1);

   /* Light gropnodes */
   for (j=boardheight,y=by;j;j--,y+=lighth)
     for (i=boardwidth,x=bx;i;i--,x+=lightw) {
	pgWriteCmd(from,PGCANVAS_GROP,5,PG_GROP_RECT,
		   x+1,y+1,lightw-2,lighth-2);
	pgWriteCmd(from,PGCANVAS_SETGROP,1,
		   (*(p++)) ? ON_COLOR : OFF_COLOR);
	pgWriteCmd(from,PGCANVAS_COLORCONV,1,1);
     }
   
   /* Draw it */
   pgWriteCmd(from,PGCANVAS_REDRAW,0);
   pgUpdate();

   return 0;
}

/* Clickski! */
int evtMouseDown(short event, pghandle from, long param) {
   int lx,ly;
   light *p;
   int i;
   
   /* What light was it in? */
   lx = (PG_PNTR_X - bx) / lightw;
   ly = (PG_PNTR_Y - by) / lighth;
   if (lx < 0 || lx >= boardwidth ||
       ly < 0 || ly >= boardwidth)
     return 0;

   invertLight(lx,ly);
   invertLight(lx-1,ly);
   invertLight(lx+1,ly);
   invertLight(lx,ly-1);
   invertLight(lx,ly+1);

   /* Update the screen */
   pgWriteCmd(from,PGCANVAS_INCREMENTAL,0);
   moves++;
   updateStatus();

   /* A win condition? */
   for (i=boardsize,p=board;i;i--,p++)
     if (*p)
       return 0; /* Nope. */
   
   /* Yep! */
   pgMessageDialogFmt("Blackout!",0,
		      "You completed level %d!\n\n"
		      "Moves: %d",level,moves);
   startLevel(level+1);
   
   return 0;
}

int btnNewGame(short event, pghandle from, long param) {
   if (pgMessageDialog("Give Up","Start over at level 1?",
		       PG_MSGBTN_YES | PG_MSGBTN_NO) == PG_MSGBTN_YES)
     startLevel(1);
}

int btnRestartLevel(short event, pghandle from, long param) {
   if (pgMessageDialogFmt("Scratching Head",
			  PG_MSGBTN_YES | PG_MSGBTN_NO,
			  "Try level %d again?",level) == PG_MSGBTN_YES)
     startLevel(level);
}

/****************************** Main Program ***/

int main(int argc, char **argv) {
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Blackout",0);

   /*** Top-level widgets: toolbar and canvas */
   wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
   wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
   pgBind(PGDEFAULT,PG_WE_BUILD,&evtDrawBoard);
   pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&evtMouseDown);

   /* Toolbar thingies */
   
   pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
   pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("New Game"),0);
   pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnNewGame);

   pgNewWidget(PG_WIDGET_BUTTON,0,0);
   pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Restart Level"),0);
   pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnRestartLevel);

   wStatusLabel = pgNewWidget(PG_WIDGET_LABEL,0,0);
   pgSetWidget(PGDEFAULT,
	       PG_WP_SIDE,PG_S_ALL,
	       PG_WP_TRANSPARENT,0,
	       PG_WP_ALIGN,PG_A_CENTER,
	       PG_WP_FONT,pgNewFont(NULL,0,PG_FSTYLE_BOLD),
	       0);


   /*** Allocate default game board */
   srand(time());
   boardwidth = 5;
   boardheight = 5;
   boardsize = sizeof(light) * boardwidth * boardheight;
   board = (light *) malloc(boardsize);
   startLevel(1);
   
   /*** Event loop */
   pgEventLoop();
   return 0;
}

/* The End */
