/* Little popup box to set video mode */

#include <picogui.h>

int main(int argc, char **argv) {
   pghandle result,wApply,wPopup,wFullscreen,wRotate;
   unsigned long vidflags;
   
   pgInit(argc,argv);
   /* FIXME: PicoGUI needs a way to automatically size popups */
   wPopup = pgNewPopupAt(PG_POPUP_ATCURSOR,0,100,200);

   /******** Take care of such niceties... */
   
   wApply = pgNewWidget(PG_WIDGET_BUTTON,0,0);
   pgSetWidget(PGDEFAULT,
	       PG_WP_TEXT,pgNewString("Apply"),
	       PG_WP_SIDE,PG_S_BOTTOM,0);

   pgNewWidget(PG_WIDGET_LABEL,0,0);
   pgSetWidget(PGDEFAULT,
	       PG_WP_TEXT,pgNewString("Set Video Mode"),
	       PG_WP_SIDE,PG_S_LEFT,
	       PG_WP_DIRECTION,PG_DIR_VERTICAL,0);
   
   /******** Mode options */
   
   wFullscreen = pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
   pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Fullscreen"),0);
   
   wRotate = pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
   pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Rotation"),0);
   
   
   /******** Run a tiny event loop */
   
   /* Until the popup box sends us a close event */
   while ( (result=pgGetEvent()->from) != wPopup ) {
      /* If we're not ready to apply yet, wait some more... */
      if (result!=wApply)
	continue;
      
      /* Get the widget status, and make some video mode parameters */
      
      vidflags = (pgGetWidget(wRotate,PG_WP_ON) ? PG_VID_ROTATE90 : 0) |
	(pgGetWidget(wFullscreen,PG_WP_ON) ? PG_VID_FULLSCREEN : 0);

      /* Set the video mode */
      pgSetVideoMode(0,0,0,PG_FM_SET,vidflags);
   }

   return 0;
}

