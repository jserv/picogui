/* Simple animation with the canvas in immediate mode */

#include <picogui.h>

#define ANIMATION_SPEED 1
pgcontext gc;

void animate(void) {
   int i;
   /* Send 50 random rectangles, they all get rendered at once. This will
    * temporarily use 100 gropnodes, (1 for the rectangle, 1 for color)
    * but nothing is stored permanently */
   pgSetMapping(gc,0,0,100,100,PG_MAP_SCALE);
   pgSetLgop(gc,PG_LGOP_XOR);
   pgSetColor(gc,rand()%0xFFFFFF);
   pgRect(gc, rand()%100, rand()%100, rand()%100, rand()%100);
   pgContextUpdate(gc);
}

int main(int argc,char **argv) {
   /* Make an app with a canvas widget, store an immediate-mode context
    * for it, and set an idle handler */
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"PGFX Immediate Canvas",0);
   pgNewWidget(PG_WIDGET_CANVAS,0,0);
   gc = pgNewCanvasContext(PGDEFAULT,PGFX_IMMEDIATE);
   pgSetIdle(ANIMATION_SPEED,&animate);
   pgEventLoop();
}
