#include <picogui.h>

int drawStuff(short event, pghandle from, long param) {
   
   pgWriteCmd(from,PGCANVAS_EXECFILL,6,
	      PGTH_O_BUTTON,PGTH_P_BGFILL,40,10,20,20);
   pgWriteCmd(from,PGCANVAS_GROP,5,PG_GROP_FRAME,10,10,20,20);
   pgWriteCmd(from,PGCANVAS_GROP,5,PG_GROP_LINE,0,0,PG_W,PG_H);
   pgWriteCmd(from,PGCANVAS_GROP,5,PG_GROP_LINE,PG_W,0,-PG_W,PG_H);
   
   pgWriteCmd(from,PGCANVAS_REDRAW,1);
   pgSubUpdate(from);

   return 0;
}

int main(int argc, char **argv) {
   pgInit(argc,argv);
   
   pgRegisterApp(PG_APP_NORMAL,"Canvas Test",0);
   pgNewWidget(PG_WIDGET_CANVAS,0,0);
   pgBind(PGDEFAULT,PG_WE_BUILD,&drawStuff);
   
   pgEventLoop();
   return 0;
}
