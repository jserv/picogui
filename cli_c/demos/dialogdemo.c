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
		  "There is a theory which states that if ever anyone\n"
		  "discovers exactly what the Universe is for and why it is\n"
		  "here, it will instantly disappear and be replaced by\n"
		  "something even more bizarre and inexplicable.  There is\n"
		  "another theory which states that this has already\n"
		  "happened.\n"
		  "\n"
		  "-- Douglas Adams, \"The Hitchhiker's Guide to the Galaxy\"",
		  0);
}

int btnConfirm(struct pgEvent *evt) {
  int i;

  i = pgMessageDialog("Really???",
		      "You just clicked a button.\n"
		      "Are you sure you want to initiate nuclear fusion?",
		      PG_MSGBTN_YES | PG_MSGBTN_NO);
  
  if (i == PG_MSGBTN_YES)
    pgMessageDialogFmt("Boom!",0,
		       "You have only %d hours until critical mass\n",
		       time(NULL));
}

/******************* Custom dialog box */

int btnTarget(struct pgEvent *evt) {
  pgcontext gc;
  pghandle wClose,wTB;
  int i,x,w;

  pgEnterContext();
  pgDialogBox("Custom Dialog Box");
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
  pghandle wApp;
  pgInit(argc,argv);
  wApp = pgRegisterApp(PG_APP_NORMAL,"Standard Dialogs Demo",0);

  /* Custom dialog box */
  pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,wApp);
  pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgDialogBox:"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_SIZEMODE,PG_SZMODE_PERCENT,
	      PG_WP_SIZE,50,
	      PG_WP_ALIGN,PG_A_RIGHT,
	      0);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Target"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnTarget,NULL);

  /* Message Dialog */
  pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,wApp);
  pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgMessageDialog:"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_SIZEMODE,PG_SZMODE_PERCENT,
	      PG_WP_SIZE,50,
	      PG_WP_ALIGN,PG_A_RIGHT,
	      0);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Quote"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnQuote,NULL);
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Confirm"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnConfirm,NULL);

  /* Date picker */
  pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,wApp);
  pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgDatePicker:"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_SIZEMODE,PG_SZMODE_PERCENT,
	      PG_WP_SIZE,50,
	      PG_WP_ALIGN,PG_A_RIGHT,
	      0);
  wDate = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Select a Date"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnDate,NULL);


  pgEventLoop();
  return 0;
}
