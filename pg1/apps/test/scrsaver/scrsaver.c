/* 
 * Simple example screen saver for PicoGUI
 * 
 * Micah Dowty <micahjd@users.sourceforge.net>
 */

#include <picogui.h>

#include <stdlib.h>          /* For random numbers */
#include <time.h>

#define INACTIVETIME 10000   /* In milliseconds */
#define POLLTIME     2       /* In seconds */
#define FRAMEDELAY   2000    /* In milliseconds */

pgcontext gc;                /* Display context for animation */
struct pgmodeinfo mi;        /* Video mode information */

pghandle mytext;
int textw, texth;

/************************ Screensaver functions */

void startup(void) {
  pghandle myfont;

  /* Clear screen */
  pgSetColor(gc,0);
  pgRect(gc,0,0,mi.xres,mi.yres);
  pgContextUpdate(gc);

  /* Set the font */
  myfont = pgNewFont(NULL,40,PG_FSTYLE_BOLD);
  pgSetFont(gc,myfont);

  /* Allocate the string once */
  mytext = pgNewString("Hello, World!");
  pgSizeText(&textw,&texth,myfont,mytext);

  srand(time(NULL));
}

void animate(void) {
  /* Draw the string at a random place */
  pgSetColor(gc,0);
  pgRect(gc,0,0,mi.xres,mi.yres);
  pgSetColor(gc,0xFFFF00);
  pgText(gc,
	 rand() % (mi.xres-textw), 
	 rand() % (mi.yres-texth),
	 mytext);

  pgContextUpdate(gc);
}

/************************ Screensaver engine */

int main(int argc, char **argv) {
  pghandle inFilter;
   
  pgInit(argc,argv);

  /* Keep running the screensaver */
  while (1) {
					 
    /* Wait out the inactivity period */
    while (pgGetInactivity() < INACTIVETIME)
      sleep(POLLTIME);
					 
    /* Grab the I/O devices */
    pgRegisterOwner(PG_OWN_DISPLAY);
    inFilter = pgNewInFilter(0,-1L,-1L);
    gc = pgNewBitmapContext(0);
    mi = *pgGetVideoMode();
					 
    /* Run the screensaver until user input */
    pgEnterContext();
    startup();
    pgSetIdle(FRAMEDELAY,&animate);
    pgGetEvent();
    pgLeaveContext();
					 
    /* Give up the I/O devices */
    pgDeleteContext(gc);
    pgDelete(inFilter);
    pgUnregisterOwner(PG_OWN_DISPLAY);					 
  }
}

/* The End */





