#include <picogui.h>
#include <sys/shm.h>
#include <time.h>

int main(int argc, char **argv) {
  pghandle bitmap, bitmapwidget, fpswidget;
  struct pgshmbitmap shm;
  unsigned long *bits, *p;
  int x,y;
  int frames, then_frames;
  time_t now,then;

  pgInit(argc,argv);
  
  /* Get a 320x240 memory mapped bitmap to scribble on 
   */
  bitmap = pgCreateBitmap(320,240);
  shm = *pgMakeSHMBitmap(bitmap);
  bits = shmat(shmget(shm.shm_key,shm.shm_length,0),NULL,0);

  /* Make an application featuring this bitmap 
   */
  pgRegisterApp(PG_APP_NORMAL,"Shared Memory Animation Demo",0);
  fpswidget = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_BOTTOM,
	      PG_WP_TRANSPARENT, 0,
	      PG_WP_FONT, pgNewFont(NULL,12,PG_FSTYLE_BOLD),
	      PG_WP_TEXT, pgNewString("Calculating frames per second..."),
	      0);
  pgNewWidget(PG_WIDGET_TEXTBOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_BOTTOM,
	      PG_WP_TEXT, pgNewString("This cheesy 320x240 animation is brought "
				      "to you via shared memory! This application "
				      "writes bitmap data in the video driver's native "
				      "format, directly into pgserver's address space. "
				      "It is then displayed in an ordinary bitmap widget "
				      "that is refreshed every frame.\n\n"),
	      0);
  bitmapwidget = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(bitmapwidget,
	      PG_WP_BITMAP,bitmap,
	      PG_WP_SIDE, PG_S_ALL,
	      0);
  pgUpdate();

  /* Do some animation now, keeping track of the frames per second
   */
  then = time(NULL);
  frames = then_frames = 0;
  while (1) {
    /* Do some (cheesy) animation,
     * This assumes 32bpp color for now */
    p = bits;
    for (y=0;y<240;y++)
      for (x=0;x<320;x++)
	*(p++) = (x + y + frames) & 0xFF;
    frames++;

    /* Update frames per second every few seconds */
    now = time(NULL);
    if (now >= then+4) {
      pgReplaceTextFmt(fpswidget,"%d.%02d frames per second", 
		       (frames-then_frames) / (now-then),
		       (100 * (frames-then_frames) / (now-then)) % 100);
      then_frames = frames;
      then = now;
      pgSubUpdate(fpswidget);
    }

    /* Redraw the bitmap widget and poll for events */
    pgSetWidget(bitmapwidget,PG_WP_BITMAP, bitmap, 0);
    pgSubUpdate(bitmapwidget);
    pgEventPoll();
  }
  return 0;
}
