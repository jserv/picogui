/*
 * Everyone's favorite greeting, now available for PicoGUI
 * Japanese version
 */

/* Every PicoGUI program needs this header 
 */
#include <picogui.h>

int main(int argc, char **argv) {

  /* pgInit establishes a connection to the picogui server and processes
   * some command line arguments starting with --pg
   */
  pgInit(argc,argv);

  /* pgRegisterApp creates a new application widget
   */
  pgRegisterApp(PG_APP_NORMAL,"Japanese Hello World",0);

  /* Create a label widget taking up all available space in the application,
   * with 'hello world' text and a big font.
   */
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TEXT,pgNewString("今日は世界"),
	      PG_WP_FONT,pgNewFont(NULL,80,PG_FSTYLE_ENCODING_UNICODE | PG_FSTYLE_FIXED),
	      0);
  
  /* Process events
   */
  pgEventLoop();
  return 0;
}
  
