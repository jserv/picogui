/* The canvas's persistent mode is actually pretty powerful. This is
 * just a very simple demo */

#include <picogui.h>

int main(int argc,char **argv) {
   pgcontext gc;
   
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"PGFX Persistent Canvas",0);
   pgNewWidget(PG_WIDGET_CANVAS,0,0);
   gc = pgNewCanvasContext(PGDEFAULT,PGFX_PERSISTENT);
   
   /* FIXME: once coordinate translation is implemented you will know
    * the size of the canvas (in arbitrary units) beforehand */
   pgGradient(gc,0,0,1000,1000,0,0x000000,0x0000A0);

   /* Some good ol'fashioned drawing */
   pgSetColor(gc,0xFF0000);
   pgText(gc,10,10,pgNewString("Hello,\nWorld!"));
   pgFrame(gc,10,60,50,50);
   pgSetColor(gc,0x0000FF);
   pgGradient(gc,11,61,48,48,25,0xFFFF00,0x0000FF);
   pgRect(gc,20,70,30,30);
   pgSetColor(gc,0xFFFFFF);
   pgTextV(gc,25,95,pgNewString("pG"),0);

   pgEventLoop();
}
