/* Demonstrate the textbox widget */

#include <picogui.h>

pghandle wText;

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

int evtField(struct pgEvent *evt) {
  pgSetWidget(wText,
	      PG_WP_TEXT,pgGetWidget(evt->from,PG_WP_TEXT),
	      0);
  pgSetWidget(evt->from,
	      PG_WP_TEXT,pgNewString(""),
	      0);
  return 0;
}

int evtMoof(struct pgEvent *evt) {
  pgSetWidget(wText,
	      PG_WP_TEXT,pgNewString("<font color=blue>M<b>oo</b>f!</font><br>"),
	      0);
  return 0;
}

int main(int argc, char **argv) {
  pghandle str, wBox , wTB;
 
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Textbox Test",0);

  /* Top-level containers */

  wTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_BOTTOM,
	      0);

  wBox = pgNewWidget(PG_WIDGET_BOX,0,0);

  /* Stuff inside the box */

  wText = pgNewWidget(PG_WIDGET_TEXTBOX, PG_DERIVE_INSIDE,wBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXTFORMAT,pgNewString("+HTML"),
	      PG_WP_SIDE,PG_S_LEFT,
              PG_WP_AUTOSCROLL,1,
	      0);

  extraSpace(pgNewWidget(PG_WIDGET_BOX,0,0));

  /* Space between box and toolbar */

  extraSpace(pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_AFTER,wBox));

  /* Toolbar */

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TEXT,pgNewString("Moof!"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtMoof,NULL);

  pgNewWidget(PG_WIDGET_FIELD,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtField,NULL);
  pgFocus(PGDEFAULT);

  /* Load text from file */
  if (argv[1]) {
    str = pgDataString(pgFromFile(argv[1]));
    pgSetWidget(wText,PG_WP_TEXT,str,0);

    /* The textbox makes its own copy of the text */
    pgDelete(str);
  }  


  pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_BEFORE,wText);
  pgSetWidget(PGDEFAULT,PG_WP_BIND,wText,0);
 
  pgEventLoop();
  return 0;
}
  
