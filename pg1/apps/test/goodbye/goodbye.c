#include <picogui.h>

int btnQuit(struct pgEvent *evt) {
  if (pgMessageDialog("Goodbye cruel world!",
		      "Are you sure you want to quit?",
		      PG_MSGBTN_YES | PG_MSGBTN_NO) == PG_MSGBTN_YES)
    pgExitEventLoop();
  
  return 1;
}

int main(int argc, char **argv) {
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Goodbye",0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("Quit"),
	      PG_WP_SIDE, PG_S_TOP,
	      0);
  pgBind(PGDEFAULT, PG_WE_ACTIVATE, &btnQuit, NULL);

  pgEventLoop();
  return 0;
}
