/* Simple animation with the canvas in immediate mode */

#include <picogui.h>

#define ANIMATION_SPEED 1
pgcontext gc;

void animate(void) {
   pgSetcolor(gc,rand()%0xFFFFFF);
   pgRect(gc, rand()%500, rand()%500, rand()%50, rand()%50);
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
