/* Demonstrate the textbox widget */

#include <picogui.h>

int main(int argc, char **argv) {
  pghandle wText;
  struct pgmemdata data = pgFromFile(argv[1]);
  
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Textbox Test",0);

  wText = pgNewWidget(PG_WIDGET_TEXTBOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXTFORMAT,pgNewString("HTML"),
	      PG_WP_TEXT,pgNewString(data.pointer),
	      0);
  
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TRANSPARENT,0,
	      PG_WP_TEXT,pgNewString("Extra Space"),
	      0);
  
  pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_BEFORE,wText);
  pgSetWidget(PGDEFAULT,PG_WP_BIND,wText,0);
 
  pgEventLoop();
  return 0;
}
  
