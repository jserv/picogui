/*
 * Textedit widget demo. Create a textedit widget with a 12pt bold font.
 * By default, the widget has a scrollbar and is editable.
 *
 * Chuck Groom, cgroom@bluemug.com, Blue Mug, Inc. July 2002.
 *
 * Contributors:
 * Philippe Ney <philippe.ney@smartdata.ch>
 *   - Added a second textedit widget to show cut, copy, paste between widgets.
 *     (new clipboard in textedit widget)
 *   - Show unicode support
 *
 */


#include <string.h>
#include <picogui.h>


static pghandle edit, edit2;
static pghandle wCut, wCopy, wPaste;


int evtBtn (struct pgEvent *evt)
{
  static union pg_client_trigger trig;

  /* zero the trig structure */
  memset (&trig, 0, sizeof (trig));

  /* set default for cut, copy and paste */
  trig.content.type = PG_TRIGGER_KEY;
  trig.content.u.kbd.mods |= PGMOD_CTRL;

  if (evt->from == wCut) {
    trig.content.u.kbd.key = PGKEY_x;
  }
  else if (evt->from == wCopy) {
    trig.content.u.kbd.key = PGKEY_c;
  }
  else {
    trig.content.u.kbd.key = PGKEY_v;
  }

  /* the event will be send to the widget focused */
  pgInFilterSend(&trig);
}


int evtGetTxt (struct pgEvent *evt)
{
  char * txtp;
  int i;

  txtp = pgGetString (pgGetWidget (edit, PG_WP_TEXT));
  printf ("\nText in textedit widget 1 :\n");
  for (i = 0; i < strlen (txtp); i++) {
    printf ("0x%x(%c) ", txtp [i] & 0xFF, txtp [i]);
  }
  printf ("\n");

  txtp = pgGetString (pgGetWidget (edit2, PG_WP_TEXT));
  printf ("\nText in textedit widget 2 :\n");
  for (i = 0; i < strlen (txtp); i++) {
    printf ("0x%x(%c) ", txtp [i] & 0xFF, txtp [i]);
  }
  printf ("\n");

  return 0;
}


int main(int argc, char **argv) {
    pghandle scroll, vbox, vbox2, wToolbar;

    pgInit(argc,argv);
    pgRegisterApp(PG_APP_NORMAL,"Textedit Demo",0);
    
    /* A toolbar for the buttons */
    wToolbar = pgNewWidget (PG_WIDGET_TOOLBAR, 0, 0);

    /* share the screen for two textedit widgets */
    vbox = pgNewWidget (PG_WIDGET_BOX, 0, 0);
    pgSetWidget (PGDEFAULT,
		 PG_WP_SIDE, PG_S_LEFT,
		 PG_WP_SIZE, pgFraction (1, 2),
		 PG_WP_SIZEMODE, PG_SZMODE_CNTFRACT,
		 0);

    vbox2 = pgNewWidget (PG_WIDGET_BOX, 0, 0);
    pgSetWidget (PGDEFAULT,
		 PG_WP_SIDE, PG_S_ALL,
		 0);

    /* first edit box */
    scroll = pgNewWidget (PG_WIDGET_SCROLL, PG_DERIVE_INSIDE, vbox);
    edit = pgNewWidget (PG_WIDGET_TEXTEDIT, 0, 0); 
    pgSetWidget (scroll, PG_WP_BIND, edit, 0);
    pgSetWidget (edit,
		 PG_WP_FONT, pgNewFont (NULL, 14, PG_FSTYLE_ENCODING_UNICODE),
		 PG_WP_TEXT, pgNewString ("Howdy!"),
		   PG_WP_SELECTION, pgNewString ("Hey!"),
		 0);

    /* second edit box */
    scroll = pgNewWidget (PG_WIDGET_SCROLL, PG_DERIVE_INSIDE, vbox2);
    edit2 = pgNewWidget(PG_WIDGET_TEXTEDIT, 0, 0); 
    pgSetWidget (scroll, PG_WP_BIND, edit2, 0);
    pgSetWidget (edit2,
		 PG_WP_FONT, pgNewFont (NULL, 14, PG_FSTYLE_ENCODING_UNICODE),
		 PG_WP_TEXT, pgNewString ("今日は世界"),
//		 PG_WP_SELECTION, pgNewString ("Hey!"),
		 0);

    /* buttons in the toolbar */    
    wPaste = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
    pgSetWidget(PGDEFAULT,
                PG_WP_SIZE,pgFraction(1,4),
                PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
                PG_WP_TEXT,pgNewString("Paste"),
                0);
    pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtBtn,NULL);

    wCopy = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
    pgSetWidget(PGDEFAULT,
                PG_WP_SIZE,pgFraction(1,4),
                PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
                PG_WP_TEXT,pgNewString("Copy"),
                0);
    pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtBtn,NULL);

    wCut = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
    pgSetWidget(PGDEFAULT,
                PG_WP_SIZE,pgFraction(1,4),
                PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
                PG_WP_TEXT,pgNewString("Cut"),
                0);
    pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtBtn,NULL);

    pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
    pgSetWidget(PGDEFAULT,
                PG_WP_SIZE,pgFraction(1,4),
                PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
                PG_WP_TEXT,pgNewString("Get"),
                0);
    pgBind(PGDEFAULT,PG_WE_ACTIVATE,&evtGetTxt,NULL);
    

    pgEventLoop();

    return 0;
}
