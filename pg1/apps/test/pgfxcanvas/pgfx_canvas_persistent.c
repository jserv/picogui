/* The canvas's persistent mode is actually pretty powerful. This is
 * just a very simple demo */

#include <picogui.h>

int main(int argc,char **argv) {
   pgcontext gc;
   int poly[]={1,1, 20,1, 20,20, 1,20,}; 
   /* Initialize PicoGUI, create an application with a canvas widget,
    * and get a PGFX context for the canvas */
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"PGFX Persistent Canvas",0);
   pgNewWidget(PG_WIDGET_CANVAS,0,0);
   gc = pgNewCanvasContext(PGDEFAULT,PGFX_PERSISTENT);

   /* Set coordinate mapping.
    * This sets up a transformation to scale 0,0,100,100 to whatever size the
    * canvas widget is. Now we have a 100x100 unit virtual area to draw
    * in, and it is scaled to the canvas widget's size
    */
   pgSetMapping(gc,0,0,100,100,PG_MAP_SCALE);
   
   /* Some good ol'fashioned drawing */

   pgSetColor(gc,0x000000);        /* An 'X' across the canvas */
   pgMoveTo(gc,1,1);
   pgLineTo(gc,99,99);             /* test moveto and lineto */
   pgLineTo(gc,99,1);
   pgLineTo(gc,1,99);
   pgLineTo(gc,1,1);
   pgSetColor(gc,0x000080);        /* Blue text */
   pgText(gc,50,10,pgNewString("Hello,\nWorld!"));   
   pgSetColor(gc,0xFFFFFF);        /* Gradient thingy */
   pgGradient(gc,30,30,40,40,25,0x8080FF,0xF0F080);
   pgFrame(gc,30,30,40,40);
   pgSetColor(gc,0xFFFF00);        /* Ellipses */
   pgFEllipse(gc,35,35,30,30);
   pgFPolygon(gc,pgNewArray(poly,16)); 
   pgSetColor(gc,0x000000);
   pgEllipse(gc,35,35,30,30);
   pgSetColor(gc,0xFFFFFF);        /* XOR a stripe down the middle */
   pgSetLgop(gc,PG_LGOP_XOR);
   pgRect(gc,0,45,100,10);
   pgSetLgop(gc,PG_LGOP_STIPPLE);      /* Dotted lines */
   pgSetColor(gc,0x000000);        /* Use offset to make a gap 2 pixels wide */
   pgWriteCmd(PGDEFAULT,PGCANVAS_GROP,5,PG_GROP_SETOFFSET,0,3,0,0);
   pgLine(gc,30,85,70,85);
   pgWriteCmd(PGDEFAULT,PGCANVAS_GROP,5,PG_GROP_SETOFFSET,0,0,0,0);
   pgLine(gc,30,85,70,85);
   
   pgEventLoop();
}
