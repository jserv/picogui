/* $Id$
 *
 * canvastst.c - based on  PicoGUI's "canvastst.c"
 *
 * Modified by: Daniele Pizzoni - Ascensit s.r.l. - Italy
 * tsho@ascensit.com - auouo@tin.it
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
 */

#include <picogui.h>
#include <special.h>

int drawStuff(struct pgEvent *evt);
void animate(void);

pghandle wCanvas;
int width,height;

/* if we want to limit the number of iterations */
long nbOfIterations;

/* Called by the PG_WE_BUILD handler when
 * the widget is resized or initially created */

int drawStuff(struct pgEvent *evt) {

   /* Turn off the animation while we're busy */
/*    pgSetIdle(0,NULL); */

   /* If we're rolled up, don't bother starting the animation again
    * or even rebuilding the groplist. Save memory and CPU. */
   if (!evt->e.size.w || !evt->e.size.h)
      return 0;
   
   /* Save new width and height for later */
   width  = evt->e.size.w;
   height = evt->e.size.h;
   
   /* Clear the groplist */
   pgWriteCmd(evt->from,PGCANVAS_NUKE,0);
   
   /* Black background */
   pgWriteCmd(evt->from,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,0x000000);
   pgWriteCmd(evt->from,PGCANVAS_GROP,5,PG_GROP_RECT,0,0,
	      evt->e.size.w,evt->e.size.h);

   pgWriteCmd(evt->from,PGCANVAS_DEFAULTFLAGS,1,PG_GROPF_INCREMENTAL);

   /* Green trail behind the line */
   pgWriteCmd(evt->from,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,0x008000);
   pgWriteCmd(evt->from,PGCANVAS_GROP,5,PG_GROP_LINE,0,0,0,0);

   /* Draw line */
   pgWriteCmd(evt->from,PGCANVAS_GROP,2,PG_GROP_SETCOLOR,0xFFFF80);
   pgWriteCmd(evt->from,PGCANVAS_GROP,5,PG_GROP_LINE,0,0,0,0);

   /* Draw the background (grops not marked as incremental) */
   pgWriteCmd(evt->from,PGCANVAS_REDRAW,1);
   pgSubUpdate(evt->from);

   /* Start the animation timer */
/*    pgSetIdle(1,&animate); */
   idleHandlerAdd(animate);

   return 0;
}

/* Called by the idle handler to animate things */

void animate(void) {
   static int ox1=0,oy1=0,ox2=0,oy2=0;   /* Previous coordinates of line */
   static int x1=10,y1=10,x2=20,y2=20;   /* Current coordinates of line */
   static int dx1=5,dy1=3,dx2=3,dy2=5;   /* Velocity of line */
   static unsigned long frames = 0;      /* Frame counter */

   /* Erase previous line */
   pgWriteCmd(wCanvas,PGCANVAS_FINDGROP,1,3);
   pgWriteCmd(wCanvas,PGCANVAS_MOVEGROP,4,ox1,oy1,ox2-ox1,oy2-oy1);

   /* New line */
   pgWriteCmd(wCanvas,PGCANVAS_FINDGROP,1,5);
   pgWriteCmd(wCanvas,PGCANVAS_MOVEGROP,4,x1,y1,x2-x1,y2-y1);
   
   /* Save old position */
   ox1 = x1; ox2 = x2; oy1 = y1; oy2 = y2;
   
   /* Update position */
   if (x1 < 0 || x1 >= width)  {
      dx1 = -dx1;
      if (x1<0)
	x1 = 0;
      else
	x1 = width-1;
   }
   if (y1 < 0 || y1 >= height) {
      dy1 = -dy1;
      if (y1<0)
	y1 = 0;
      else
	y1 = height-1;
   }
   if (x2 < 0 || x2 >= width) {
      dx2 = -dx2;
      if (x2<0)
	x2 = 0;
      else
	x2 = width - 1;
   }
   if (y2 < 0 || y2 >= height) {
      dy2 = -dy2;
      if (y2<0)
	y2 = 0;
      else
	y2 = height - 1;
   }
   x1 += dx1; y1 += dy1; x2 += dx2; y2 += dy2;

   /* Every so often do a full redraw to clear the canvas */
   if (!(frames % 500))
     pgWriteCmd(wCanvas,PGCANVAS_REDRAW,0);
   else
     /* Incremental update */
     pgWriteCmd(wCanvas,PGCANVAS_INCREMENTAL,0);

   /* Yay! */
   frames++;

   pgSubUpdate(wCanvas);

   /* if the counter is now 0, we can stop the program */
   if(( nbOfIterations >= 0 ) && ( frames >= nbOfIterations )) {
     exit(0);
   }
}

/* main(), sets a few things up */
int canvastst(pghandle widget) {
   /* init the number of iteration we want */
/*    if( argc == 2 ) { */
/*      nbOfIterations = atoi( argv[1] ) ; */
/*    } else { */
/*      nbOfIterations = -1; */
/*    } */

/*    pgInit(argc,argv); */
/*    pgRegisterApp(PG_APP_NORMAL,"Canvas Animation Test",0); */

/*    wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0); */
/*    pgBind(PGDEFAULT,PG_WE_BUILD,&drawStuff,NULL); */
   
/*    pgEventLoop(); */

   nbOfIterations = -1;
   wCanvas = widget;
   pgBind(wCanvas, PG_WE_BUILD, &drawStuff, NULL);

   return 0;
}
