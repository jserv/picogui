#include <picogui.h>

pghandle wCanvas;

int main(int argc, char **argv) {
   pgInit(argc,argv);
   
   pgRegisterApp(PG_APP_NORMAL,"Canvas Test",0);
   wCanvas = pgNewWidget(PG_WIDGET_CANVAS,0,0);

   
   pgEventLoop();
   return 0;
}
