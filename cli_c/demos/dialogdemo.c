/* Demo for all the builtin dialog boxes 
 *
 * This program is released into the public domain. Feel free to
 * incorporate it into other PicoGUI programs or use it as a reference.
 *
 * -- Micah Dowty <micahjd@users.sourceforge.net>
 */

#include <picogui.h>
#include <time.h>     /* time() is used in our little joke */

pghandle wDate;
int year,month,day;

/******************* Date picker */

int btnDate(struct pgEvent *evt) {
  if (pgDatePicker(&year,&month,&day,"Pick a Date"))
    pgReplaceTextFmt(wDate,"%d/%d/%d",month,day,year);
}

/******************* Message Dialog */

int btnQuote(struct pgEvent *evt) {
  pgMessageDialog("Quote",
		  "\"Andrew, I don't care if your\n"
		  "sister put an ice cube down\n"
		  "your underware she's a girl\n"
		  "and girls will do that.\n"
		  "That doesn't mean that you\n"
		  "can hit her with the dog.\"\n\n" 
		  "-Lt. Col Henry Blake,\n  MASH 4077",
		  0);
}

int btnConfirm(struct pgEvent *evt) {
  int i;

  i = pgMessageDialog("Really???",
		      "You just clicked a button.\n"
		      "Are you sure you want\n"
		      "to initiate nuclear fusion?",
		      PG_MSGBTN_YES | PG_MSGBTN_NO);
  
  if (i == PG_MSGBTN_YES)
    pgMessageDialogFmt("Boom!",0,
		       "You have only %d\n"
		       "hours until critical mass",
		       time(NULL));
}

/******************* Custom dialog box */

int btnTarget(struct pgEvent *evt) {
  pgcontext gc;
  pghandle wClose,wTB;
  int i,x,w;

  pgEnterContext();
  pgDialogBox("Target");
  wTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);

  /* Just draw something fun here */
  pgNewWidget(PG_WIDGET_CANVAS,0,0);
  gc = pgNewCanvasContext(PGDEFAULT,PGFX_PERSISTENT);
  pgSetMapping(gc,0,0,100,100,PG_MAP_SCALE);
  for (i=0;i<=4;i++) {
    x = i*10;
    w = 100-(x<<1);
    pgSetColor(gc,i&1 ? 0xFFFFFF : 0xFF0000);
    pgFEllipse(gc,x,x,w,w);
    pgSetColor(gc,0x000000);
    pgEllipse(gc,x,x,w,w);
  }
  pgContextUpdate(gc);
  pgDeleteContext(gc);

  /* A close button */
  wClose = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Close"),
	      PG_WP_HOTKEY,PGKEY_RETURN,
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);

  /* Wait for a click on the canvas, then clean up */
  while (pgGetEvent()->from != wClose);
  pgLeaveContext();
}

/******************* Main program */

int main(int argc,char **argv) {
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Standard Dialogs",0);

  /* Custom dialog box */
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgDialogBox: Target"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnTarget,NULL);

  /* Message Dialog */
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgMessageDialog: Quote"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnQuote,NULL);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgMessageDialog: Confirm"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnConfirm,NULL);

  /* Date picker */
  wDate = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgDatePicker: Select a Date"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnDate,NULL);


  pgEventLoop();
  return 0;
}
