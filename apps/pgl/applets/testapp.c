/* Simulate the PGL applet container */

#include <picogui.h>

int main(int argc, char **argv) {
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_TOOLBAR,"PGL Applet Test",0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_PUBLICBOX,1,
	      PG_WP_NAME,pgNewString("PGL-AppletBar"),
	      0);
  pgEventLoop();
}

