/*
 * Demonstation of showing bitmap animation until a button is pressed
 *
 * -- Micah Dowty <micahjd@users.sourceforge.net>
 */

#include <picogui.h>

#define ANIMTITLE    "Boing!"
#define NUMFRAMES    13
#define FRAMENAME    "data/ball%02d.jpeg"
#define FRAMENAMELEN 40

int main(int argc,char **argv) {
  pghandle frames[NUMFRAMES];
  char frname[FRAMENAMELEN];
  int i, d;
  pghandle wBitmap, wOk;

  pgInit(argc,argv);

  /* This is so we can clean up afterwards. This isn't necessary for this
   * simple demo, but if this code is used as part of a larger app this
   * is very important.
   */
  pgEnterContext();

  /* Load the frames into memory */
  for (i=0;i<NUMFRAMES;i++) {
    snprintf(frname,FRAMENAMELEN,FRAMENAME,i);
    frames[i] = pgNewBitmap(pgFromFile(frname));
  }

  /* Create a dialog box with an ok button and bitmap widget */
  pgDialogBox(ANIMTITLE);
  wOk = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_TEXT,pgGetServerRes(PGRES_STRING_OK),
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_OK),
	      0);
  wBitmap = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TRANSPARENT,0,
	      0);

  /* Animation loop */
  i = 0;
  d = 1;
  for (;;) {
    
    /* change the frame, update the screen */
    pgSetWidget(wBitmap,
		PG_WP_BITMAP,frames[i],
		0);
    pgUpdate();
    
    /* Check whether the ok button was clicked */
    if (pgCheckEvent() && pgGetEvent()->from==wOk)
      break;

    /* Small delay between frames */
    usleep(1);

    /* Next frame, ping-pong style */
    if (i == NUMFRAMES-1)
      d = -1;
    if (!i)
      d = 1;
    i += d;
  }

  /* Clean up */
  pgLeaveContext();

  return 0;
}
