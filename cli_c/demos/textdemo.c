/* Demonstrate the textbox widget */

#include <picogui.h>

int main(int argc, char **argv) {
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Textbox Test",0);

  pgNewWidget(PG_WIDGET_TEXTBOX,0,0);
  
  pgEventLoop();
  return 0;
}
  
