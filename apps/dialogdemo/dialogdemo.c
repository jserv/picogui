/* Demo for all the builtin dialog boxes 
 *
 * This program is released into the public domain. Feel free to
 * incorporate it into other PicoGUI programs or use it as a reference.
 *
 * -- Micah Dowty <micahjd@users.sourceforge.net>
 */

#include <picogui.h>
#include <time.h>     /* time() is used in our little joke */

/* For storing the date */
pghandle wDate;
int year,month,day;

pghandle wInputBtn,wFontBtn,wFileOpenBtn,wFileSaveBtn;

/******************* Bomb */

int btnBomb(struct pgEvent *evt) {
  pgNewWidget(0xFEEDBEEF,PG_DERIVE_AFTER,wInputBtn);
}

/******************* Color */

int btnColor(struct pgEvent *evt) {
  static pgcolor c = 0x224466;

  if (pgColorPicker(&c,"Select a Color"))
    pgMessageDialogFmt("Color",0,"You selected:\n0x%06X",c);
}

/******************* File save dialog */

int btnFileSave(struct pgEvent *evt) {
  const char *str;

  str = pgFilePicker(NULL,NULL,NULL,PG_FILESAVE,"Save a File");
  if (str)
    pgReplaceText(wFileSaveBtn,str);
}

/******************* File open dialog */

int btnFileOpen(struct pgEvent *evt) {
  const char *str;

  str = pgFilePicker(NULL,NULL,NULL,PG_FILEOPEN,"Open a File");
  if (str)
    pgReplaceText(wFileOpenBtn,str);
}

/******************* Font picker */

int btnFont(struct pgEvent *evt) {
  pghandle fnt;

  fnt = pgFontPicker("Pick a new font");

  if (fnt) {
    /* Delete the old font, and set the new font */
    
    pgDelete(pgGetWidget(wFontBtn,PG_WP_FONT));
    pgSetWidget(wFontBtn,PG_WP_FONT,fnt,0);
  }
}

/******************* String input */

int btnInput(struct pgEvent *evt) {
  pghandle str;

  str = pgInputDialog("Input Test",
		      "What... is the air-speed velocity of\n"
                      "an unladen swallow?",
                      pgGetWidget(wInputBtn,PG_WP_TEXT));

  if (str) {
    /* Normally we would use pgReplaceText, but we already have a
     * handle so we can save a step. */
    
    pgDelete(pgGetWidget(wInputBtn,PG_WP_TEXT));
    pgSetWidget(wInputBtn,PG_WP_TEXT,str,0);
  }
}

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
  pghandle wBox;
  
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_NORMAL,"Standard Dialogs",0);

  /* Scrollable box */
  wBox = pgNewWidget(PG_WIDGET_SCROLLBOX,0,0);

  /* Message Dialog */
  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wBox);
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

  /* Custom dialog box */
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgDialogBox: Target"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnTarget,NULL);

  /* Date picker */
  wDate = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgDatePicker: Select a Date"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnDate,NULL);

  /* Text input */
  wInputBtn = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgInputDialog: Enter a string"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnInput,NULL);

  /* Font picker */
  wFontBtn = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgFontPicker: Select a font"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnFont,NULL);

  /* File open */
  wFileOpenBtn = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgFilePicker: Open a file"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnFileOpen,NULL);

  /* File save */
  wFileSaveBtn = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgFilePicker: Save a file"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnFileSave,NULL);

  /* Color Picker */
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("pgColorPicker: Select a Color"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnColor,NULL);

  /* Bomb */
  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Bomb: Cause error dialog"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnBomb,NULL);

  pgEventLoop();
  return 0;
}
