/* Demonstrate offscreen bitmaps */

#include <picogui.h>

int main(int argc, char **argv) {
  pghandle bit;
  pgcontext gc;

  /* Init PicoGUI */
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Offscreen Bitmap",0);

  /* Create the empty bitmap. It can be used just like any other bitmap */
  bit = pgCreateBitmap(100,100);

  /* Display the bitmap with a bitmap widget */
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_BITMAP,bit,
	      PG_WP_SIDE,PG_S_LEFT,
	      0);

  /* Display it in a button */
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Bitmap Button"),
	      PG_WP_BITMAP,bit,
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  
  /* Any bitmap can be scribbled on with pgRender */
  pgRender(bit,PG_GROP_SETCOLOR,0x0000FF);
  pgRender(bit,PG_GROP_RECT,0,0,100,100);
  pgRender(bit,PG_GROP_SETCOLOR,0xFFFF00);
  pgRender(bit,PG_GROP_LINE,0,0,100,100);
  pgRender(bit,PG_GROP_LINE,100,0,-100,100);

  /* Or, use PGFX */
  gc = pgNewBitmapContext(bit);
  pgSetColor(gc,0xFFFFFF);
  pgFEllipse(gc,10,10,80,80);
  pgSetColor(gc,0x8080FF);
  pgFEllipse(gc,30,30,40,40);

  pgEventLoop();  
  return 0;
}
