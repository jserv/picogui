/* Menu test */

#include <picogui.h>

int btnMenu(short event,pghandle from,long param) {
  pgEnterContext();

  pgNewPopupAt(PG_POPUP_ATCURSOR,0,50,150);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Perl"),
	      PG_WP_SIDE,PG_S_TOP,0);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Python"),
	      PG_WP_SIDE,PG_S_TOP,0);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("C"),
	      PG_WP_SIDE,PG_S_TOP,0);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("C++"),
	      PG_WP_SIDE,PG_S_TOP,0);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("PHP"),
	      PG_WP_SIDE,PG_S_TOP,0);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("FORTRAN"),
	      PG_WP_SIDE,PG_S_TOP,0);

  pgGetEvent(NULL,NULL);
  pgLeaveContext();
  pgUpdate();

  return 0;
}

int btnClose(short event,pghandle from,long param) {
  exit(0);
}

int main(int argc, char *argv[])
{
  pgInit(argc,argv);

  pgRegisterApp(PG_APP_TOOLBAR,"msgdialog demo",0);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Run"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnMenu);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Menu test program"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("X"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnClose);

  /* Run it */
  pgEventLoop();

  return 0;
}

/* The End */
