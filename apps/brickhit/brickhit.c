/* $Id: brickhit.c,v 1.2 2001/01/31 04:18:06 micahjd Exp $
 *
 * brickhit.c - PicoGUI remake of a game I made when I was really little. The
 *              original BrickHit was written in True Basic using 16-color
 *              graphics. I wrote the video libraries and started work on a
 *              sequel, BrickHit II, written in C and using Mode 0x13.
 *              I never finished that.
 *              This uses the same idea but adds a few new twists to the game
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

/**** Game constants */
#define BRICKS_LONG 10
#define BRICKS_DEEP 4

/**** Some calculated constants */

#define NUM_OBJECTS (BRICKS_LONG * BRICKS_DEEP + 3)

/**** Widgets */

pghandle wToolbar,wCanvas;
int canvasw,canvash;

/**** A game object - brick, ball, or paddle */

struct gameobj {
   int x,y,w,h;  /* Position and size */
   int vx,vy;    /* Velocity in pixels per frame */
   int fg_grop,bg_grop;
   
   /* Called when it collides with another object, 'projectile' */
   void (*on_collide)(struct gameobj *self, struct gameobj *projectile);
};

struct gameobj objects[NUM_OBJECTS];
struct gameobj *ball    = &objects[0];
struct gameobj *paddle1 = &objects[1];
struct gameobj *paddle2 = &objects[2];

/**** Functions */
void moveAndCheck(void);
int evtDrawPlayfield(short event, pghandle from, long param);
int evtMouseDown(short event, pghandle from, long param);
void idleHandler(void);
   
/****************************** Game functions ***/

/* Move all applicable objects and check for collisions */
void moveAndCheck(void) {
   struct gameobj *o,*p;
   int i,j;
   
   for (i=NUM_OBJECTS,o=objects;i;i--,o++)
      if (o->vx || o->vy) {
	 
	 /* Hide it at its old position */
	 pgWriteCmd(wCanvas,PGCANVAS_FINDGROP,1,o->bg_grop);
	 pgWriteCmd(wCanvas,PGCANVAS_MOVEGROP,4,o->x,o->y,o->w,o->h);
	 pgWriteCmd(wCanvas,PGCANVAS_GROPFLAGS,1,PG_GROPF_PSEUDOINCREMENTAL);
	 
	 o->x += o->vx;
	 o->y += o->vy;

         /* Check collisions with other objects */
	 for (j=NUM_OBJECTS,p=objects;j;j--,p++) {
	    if (o==p) continue;
	    
	    if ( (p->x < (o->x+o->w-1)) &&
	         ((p->x+p->w-1) > o->x) )
	      o->x += (o->vx = -o->vx) << 1;

	    if ( (p->y < (o->y+o->h-1)) &&
	         ((p->y+p->h-1) > o->y) )
	      o->y += (o->vy = -o->vy) << 1;
	 }
	 
	 /* Check collisions with playfield edge */
	 if ( (o->x < 0) ||
	     ((o->x+o->w-1) > canvasw) )
	   o->x += (o->vx = -o->vx) << 1;

//	 if ( (o->y < 0) ||
//	     ((o->y+o->h-1) > canvash) )
//	   o->y += (o->vy = -o->vy) << 1;
	 
	 pgWriteCmd(wCanvas,PGCANVAS_FINDGROP,1,o->fg_grop);
	 pgWriteCmd(wCanvas,PGCANVAS_MOVEGROP,4,o->x,o->y,o->w,o->h);
	 pgWriteCmd(wCanvas,PGCANVAS_GROPFLAGS,1,PG_GROPF_PSEUDOINCREMENTAL);
      }
}
      

/****************************** Event Handlers ***/

int evtDrawPlayfield(short event, pghandle from, long param) {

   /* Turn off idle handler (pause game) and clear */
   pgSetIdle(0,NULL);
   pgWriteCmd(wCanvas,PGCANVAS_NUKE,0);
   
   /* Stop here if we're rolled up */
   if (!(PG_W || PG_H))
     return 0;
   
   /* Background */
   pgWriteCmd(from,PGCANVAS_GROP,5,PG_GROP_RECT,0,0,PG_W,PG_H);
   pgWriteCmd(from,PGCANVAS_SETGROP,1,0x000000);
   pgWriteCmd(from,PGCANVAS_COLORCONV,1,1);
   
   /* Start up idle handler */
   pgSetIdle(10,&idleHandler);
   return 0;
}

int evtMouseDown(short event, pghandle from, long param) {
}

void idleHandler(void) {
   /* Animate */
   moveAndCheck();
   pgWriteCmd(wCanvas,PGCANVAS_INCREMENTAL,0);
   pgSubUpdate(wCanvas);
}

/****************************** Main Program ***/

int main(int argc, char **argv) {
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"BrickHit III",0);

   /*** Top-level widgets: toolbar and canvas */
   wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
   wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
   pgBind(PGDEFAULT,PG_WE_BUILD,&evtDrawPlayfield);
   pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&evtMouseDown);

   /*** Event loop */
   pgEventLoop();
   return 0;
}

/* The End */
