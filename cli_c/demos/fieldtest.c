#include <picogui.h>

int fieldActivate(struct pgEvent *evt) {
  pgMessageDialog("You typed:",
		  pgGetString(pgGetWidget(evt->from,PG_WP_TEXT)),0);
  return 0;
}

int btnExit(struct pgEvent *evt) {
  exit(0);
}

int main(int argc, char **argv) {
  pgInit(argc,argv);

  /* Register app, and but a button on it */

  pgRegisterApp(PG_APP_TOOLBAR,"msgdialog demo",0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("X"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnExit,NULL);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Field test: "),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);

  pgNewWidget(PG_WIDGET_FIELD,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&fieldActivate,NULL);
  pgFocus(PGDEFAULT);
   
  /* Run it */
  pgEventLoop();

  return 0;
}
