/* little image viewer demo */

#include <picogui.h>

int main(int argc, char *argv[])
{

  pgInit(argc,argv);

  pgRegisterApp(PG_APP_NORMAL,"Image Viewer",
		PG_APPSPEC_HEIGHT,200,
		0);

  pgNewWidget(PG_WIDGET_BITMAP,0,0);
  pgSetWidget(0,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_BITMAP,pgNewBitmap(pgFromFile(argv[1])),
	      0);

  pgEventLoop();
  return 0;
}





/* The End */
