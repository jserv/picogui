#include <picogui.h>

int drawStuff(short event, pghandle from, long param);
void animate(void);

pghandle wCanvas;
int width,height;

/* Called by the PG_WE_BUILD handler when the widget is resized or initially created */

int drawStuff(short event, pghandle from, long param) {

   /* Turn off the animation while we're busy */
   pgSetIdle(0,NULL);
   
   /* Save new width and height for later */
   width  = PG_W;
   height = PG_H;
   
   /* Clear the groplist */
   pgWriteCmd(from,PGCANVAS_NUKE,0);
   
   /* Black background */
   pgWriteCmd(from,PGCANVAS_GROP,5,PG_GROP_RECT,0,0,PG_W,PG_H);
   pgWriteCmd(from,PGCANVAS_SETGROP,1,0x000000);
   pgWriteCmd(from,PGCANVAS_COLORCONV,1,1);

   /* Green trail behind the line */
   pgWriteCmd(from,PGCANVAS_GROP,5,PG_GROP_LINE,0,0,0,0);
   pgWriteCmd(from,PGCANVAS_SETGROP,1,0x008000);
   pgWriteCmd(from,PGCANVAS_COLORCONV,1,1);
   pgWriteCmd(from,PGCANVAS_GROPFLAGS,1,PG_GROPF_INCREMENTAL);
   
   /* Draw line */
   pgWriteCmd(from,PGCANVAS_GROP,5,PG_GROP_LINE,0,0,0,0);
   pgWriteCmd(from,PGCANVAS_SETGROP,1,0xFFFF80);
   pgWriteCmd(from,PGCANVAS_COLORCONV,1,1);
   pgWriteCmd(from,PGCANVAS_GROPFLAGS,1,PG_GROPF_INCREMENTAL);

   /* Draw the background (grops not marked as incremental) */
   pgWriteCmd(from,PGCANVAS_REDRAW,1);
   pgSubUpdate(from);

   /* Start the animation timer */
   pgSetIdle(10,&animate);
   
   return 0;
}

/* Called by the idle handler to animate things */

void animate(void) {
   static int ox1=0,oy1=0,ox2=0,oy2=0;   /* Previous coordinates of line */
   static int x1=10,y1=10,x2=20,y2=20;   /* Current coordinates of line */
   static int dx1=5,dy1=3,dx2=3,dy2=5;   /* Velocity of line */
   static unsigned long frames = 0;      /* Frame counter */

   /* Erase previous line */
   pgWriteCmd(wCanvas,PGCANVAS_FINDGROP,1,1);
   pgWriteCmd(wCanvas,PGCANVAS_MOVEGROP,4,ox1,oy1,ox2-ox1,oy2-oy1);

   /* New line */
   pgWriteCmd(wCanvas,PGCANVAS_FINDGROP,1,2);
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
}

/* main(), sets a few things up */

int main(int argc, char **argv) {
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Canvas Animation Test",0);

   wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);
   pgBind(PGDEFAULT,PG_WE_BUILD,&drawStuff);
   
   pgEventLoop();
   return 0;
}
