#include <picogui.h>

pghandle myfield;

int fieldActivate(short event,pghandle from,long param) {
  pgMessageDialog("You typed:",
		  pgGetString(pgGetWidget(myfield,PG_WP_TEXT)),0);
  return 0;
}

int btnExit(short event,pghandle from,long param) {
  exit(0);
}

int main(int argc, char *argv[])
{
  pgInit(argc,argv);

  /* Register app, and but a button on it */

  pgRegisterApp(PG_APP_TOOLBAR,"msgdialog demo",0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("X"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnExit);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Field test: "),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);

  myfield = pgNewWidget(PG_WIDGET_FIELD,0,0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&fieldActivate);

   
  /* Run it */
  pgEventLoop();

  return 0;
}
