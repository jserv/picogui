/* Little popup box to set video mode */

#include <picogui.h>

int main(int argc, char **argv) {
  struct pgEvent *evt;
  pghandle wApply,wClose,sTmp,wToolbar;
  pghandle wXres,wYres,wBpp,wRotate,wPopup;
  struct pgmodeinfo mi;
  char buf[20];
   
  pgInit(argc,argv);
  wPopup = pgDialogBox("Set Video Mode");

  /******** Take care of such niceties... */
   
  wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);
		  
  wApply = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Apply"),
	      0);

  wClose = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Close"),
	      0);

  /******** Mode options */
   
  /* For now just make boxes, we'll fill them in later */
  wXres = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_AFTER,wToolbar);
  pgSetWidget(PGDEFAULT,PG_WP_TRANSPARENT,1,0);
  wYres = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TRANSPARENT,1,0);
  wBpp = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TRANSPARENT,1,0);
   
  wRotate = pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Rotation"),0);
   
  /* Fill in the boxes */
   
  wXres = pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,wXres);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_SIZE,50,
	      PG_WP_SIZEMODE,PG_SZMODE_PERCENT,
	      0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TEXT,pgNewString("Width:"),
	      0);
   
  wYres = pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,wYres);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_SIZE,50,
	      PG_WP_SIZEMODE,PG_SZMODE_PERCENT,
	      0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TEXT,pgNewString("Height:"),
	      0);
   
  wBpp = pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,wBpp);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_SIZE,50,
	      PG_WP_SIZEMODE,PG_SZMODE_PERCENT,
	      0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TEXT,pgNewString("BPP:"),
	      0);
   
  /******** Run a tiny event loop */
   
  /* Big loop setting modes... */
  for (;;) {
      
    /* Get actual mode info, put it in the widgets.
     * Not the best way, but for now pgReplaceTextFmt doesn't work on
     * field widgets because their handle handling is different :)
     * The field copies our string to an internal buffer so we don't need
     * to keep the handle around. Note that a server bug (listed in my
     * todo file) will cause a server segfault if pgReplaceText is used!
     */
    mi = *pgGetVideoMode();
      
    sprintf(buf,"%d",mi.xres);
    sTmp = pgNewString(buf);
    pgSetWidget(wXres,PG_WP_TEXT,sTmp,0);
    pgDelete(sTmp);

    sprintf(buf,"%d",mi.yres);
    sTmp = pgNewString(buf);
    pgSetWidget(wYres,PG_WP_TEXT,sTmp,0);
    pgDelete(sTmp);

    sprintf(buf,"%d",mi.bpp);
    sTmp = pgNewString(buf);
    pgSetWidget(wBpp,PG_WP_TEXT,sTmp,0);
    pgDelete(sTmp);

    pgSetWidget(wRotate,PG_WP_ON,mi.flags & PG_VID_ROTATE90,0);
      
    /* Small event loop waiting for an apply */
    for (;;) {
      evt = pgGetEvent();
	 
      /* Done yet? */
      if (evt->from == wClose)
	return 0;
	 
      /* Any PG_WE_ACTIVATE causes apply, this covers clicking the apply
       * button or pressing enter in a field. Specifically exclude
       * clicking the rotate checkbox */
      if (evt->type == PG_WE_ACTIVATE && evt->from != wRotate) 
	break;
    }
      
    /* Set mode based on widget values */
    pgSetVideoMode(atoi(pgGetString(pgGetWidget(wXres,PG_WP_TEXT))),
		   atoi(pgGetString(pgGetWidget(wYres,PG_WP_TEXT))),
		   atoi(pgGetString(pgGetWidget(wBpp,PG_WP_TEXT))),
		   pgGetWidget(wRotate,PG_WP_ON) ? PG_FM_ON : PG_FM_OFF,
		   PG_VID_ROTATE90);
  }
   
  return 0;
}

