/* Demonstrate the textbox widget */

#include <picogui.h>

/* Put an 'extra space' marker in this box */
void extraSpace(pghandle box) {
  pgcontext gc;
  pghandle canvas;
  int i;

  pgSetWidget(box,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  canvas = pgNewWidget(PG_WIDGET_CANVAS,PG_DERIVE_INSIDE,box);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);

  gc = pgNewCanvasContext(canvas,PGFX_PERSISTENT);

  pgSetMapping(gc,0,0,10,10,PG_MAP_SCALE);
  pgSetColor(gc,0x808080);
  for (i=0;i<10;i++) {
    pgLine(gc,0,i,i,0);
    pgLine(gc,i,10,10,i);
  }
  pgSetColor(gc,0x000000);
  pgText(gc,0,0,pgNewString("Extra Space"));

  pgDeleteContext(gc);
}

int main(int argc, char **argv) {
  pghandle wText, str, wBox;
 
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Textbox Test",0);

  wBox = pgNewWidget(PG_WIDGET_BOX,0,0);

  /* Load text */
  str = pgDataString(pgFromFile(argv[1])),

  wText = pgNewWidget(PG_WIDGET_TEXTBOX, PG_DERIVE_INSIDE,wBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXTFORMAT,pgNewString("HTML"),
	      PG_WP_TEXT,str,
	      PG_WP_SIDE,PG_S_LEFT,
	      0);

  /* The textbox makes its own copy of the text */
  pgDelete(str);
  
  extraSpace(pgNewWidget(PG_WIDGET_BOX,0,0));
  extraSpace(pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_AFTER,wBox));

  pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_BEFORE,wText);
  pgSetWidget(PGDEFAULT,PG_WP_BIND,wText,0);
 
  pgEventLoop();
  return 0;
}
  
