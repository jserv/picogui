/* Simple animation with the canvas in immediate mode */

#include <picogui.h>

#define ANIMATION_SPEED 1
pgcontext gc;

void animate(void) {
   int i;
   /* Draw a random line in a random color */
   pgSetMapping(gc,0,0,100,100,PG_MAP_SCALE);
   pgSetClip(gc,20,20,60,60);
   pgSetLgop(gc,PG_LGOP_XOR);
   pgSetColor(gc,rand()%0xFFFFFF);
   pgLine(gc, rand()%100, rand()%100, rand()%100, rand()%100);
   pgContextUpdate(gc);
}

int main(int argc,char **argv) {
   /* Make an app with a canvas widget, store an immediate-mode context
    * for it, and set an idle handler */
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"PGFX Immediate Canvas",0);
   pgNewWidget(PG_WIDGET_CANVAS,0,0);
   gc = pgNewCanvasContext(PGDEFAULT,PGFX_IMMEDIATE);
   pgUpdate();
   while (1) {
     animate();
     pgEventPoll();
   }
}
