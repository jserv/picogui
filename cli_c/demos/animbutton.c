/* Something silly you can do with bitmap rendering... */

#include <picogui.h>

pghandle bBitmap,wButton;
pgcontext gc;

void animate(void) {
   static int frame = 0;
   
   /* A little drawing... */
   pgSetColor(gc,0xFFFFFF);
   pgRect(gc,0,0,15,15);
   pgSetColor(gc,0x000000);
   pgFrame(gc,0,0,15,15);
   pgLine(gc,frame,0,14-frame,14);
   pgLine(gc,0,14-frame,14,frame);
   
   if ((++frame)==15) frame = 0;
   
   /* FIXME: This is just a hack, need a better way to 
    * force bitmap redraw. */
   pgSetWidget(wButton,PG_WP_BITMAP,bBitmap,0);
   pgUpdate();
}

int main(int argc, char **argv) {
   pgInit(argc,argv);
   pgRegisterApp(PG_APP_NORMAL,"Animated Button",0);
   bBitmap = pgCreateBitmap(15,15);
   gc = pgNewBitmapContext(bBitmap);
   pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
   wButton = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,PGDEFAULT);
   pgSetWidget(PGDEFAULT,
	       PG_WP_TEXT,pgNewString("Nifty!"),
	       PG_WP_BITMAP,bBitmap,
	       0);
   pgSetIdle(75,&animate);
   pgEventLoop();  
   return 0;
}
