/* Menu test */

#include <picogui.h>

#include <sys/types.h>   /* For making directory listings */
#include <dirent.h>

#include <malloc.h>      /* Dynamic memory is used for the array */

/* The simplest way to make a menu */
int btnMenuFromString(short event,pghandle from,long param) {
  pgMessageDialogFmt("Menu results",0,"pgMenuFromString() returned %d",
		     pgMenuFromString("Hello World!|This is a test...|1\n2\n3"));
  return 0;
}

/* A little more work, but the best way to make dynamic menus.
 * This example makes a directory listing into a menu */
int btnMenuFromArray(short event,pghandle from,long param) {
  pghandle *items;
  struct dirent *dent;
  int i,num = 0;
  DIR *d;

  d = opendir("/home");

  /* Count the items and allocate the array */
  while (readdir(d)) num++;
  items = malloc(sizeof(pghandle) * num);
  
  /* Enter a new context before making the handles */
  pgEnterContext();

  /* Make handles */
  rewinddir(d);
  i = 0;
  while (dent = readdir(d))
    items[i++] = pgNewString(dent->d_name);

  /* Run it */
  i = pgMenuFromArray(items,num);

  /* Show the result */
  if (i)
    pgMessageDialogFmt("Menu results",0,
		       "pgMenuFromArray() returned %d\npgGetString(items[%d]) = \"%s\"",
		       i,i-1,pgGetString(items[i-1]));

  /* Free memory for the item array and get rid of the handles. */
  free(items);
  pgLeaveContext();

  closedir(d);

  return 0;
}

/* The most difficult way to make menus, but this allows
   you to put any arbitrary widgets in the menu and
   customize to your heart's content */
int btnCustomMenu(short event,pghandle from,long param) {
  pghandle result,toolbar;

  /* Do our own context management */
  pgEnterContext();

  /* Create a popup menu (size it ourselves) */
  pgNewPopupAt(PG_POPUP_ATCURSOR,0,180,340);

  /* Decorations! */
  pgNewWidget(PG_WIDGET_LABEL,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT,pgNewString("Welcome to PicoGUI"),
		PG_WP_DIRECTION,PG_DIR_VERTICAL,
		PG_WP_SIDE,PG_S_LEFT,
		PG_WP_FONT,pgNewFont(NULL,20,0),
		0);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT,pgNewString("Click something:"),
		0);

  /* Some items */
  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Perl"),0);
  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Python"),0);
  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("C"),0);
  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT,pgNewString("Linux!"),
		PG_WP_BITMAP,pgNewBitmap(pgFromFile("demos/data/tux.pnm")),
		PG_WP_BITMASK,pgNewBitmap(pgFromFile("demos/data/tux_mask.pnm")),
		0);
  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("C++"),0);
  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("PHP"),0);
  pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("FORTRAN"),0);

  /* More decorations */
   toolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_BOTTOM,0);

  pgNewWidget(PG_WIDGET_BITMAP,0,0);
    pgSetWidget(PGDEFAULT,
		/* Load a picture from the 'data' directory */
		PG_WP_BITMAP,pgNewBitmap(pgFromFile("demos/data/dustpuppy.pnm")),
		PG_WP_BITMASK,pgNewBitmap(pgFromFile("demos/data/dustpuppy_mask.pnm")),
		PG_WP_LGOP,PG_LGOP_OR,
		PG_WP_SIDE,PG_S_ALL,
		0);

  /* You don't have to limit yourself to menuitem widgets */
  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,toolbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Dust Puppy!"),
	      PG_WP_SIDE,PG_S_ALL,
	      /* Set the EXEVs so it acts like a menu item */
	      PG_WP_EXTDEVENTS,PG_EXEV_NOCLICK | PG_EXEV_PNTR_UP,
	      0);

  /* Run it */
  result = pgGetWidget(pgGetEvent(NULL,NULL),PG_WP_TEXT);

  /* Instead of mucking about with payloads, 
     just get the menu item's text property */
  if (result)
    pgMessageDialogFmt("Menu results",0,
		       "You selected \"%s\"",
		       pgGetString(result));

  /* Clean-up time */
  pgLeaveContext();

  return 0;
}

/****** Main program */

int main(int argc, char *argv[])
{
  pgInit(argc,argv);

  pgRegisterApp(PG_APP_TOOLBAR,"msgdialog demo",0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("String"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnMenuFromString);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Array"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnMenuFromArray);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Custom"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnCustomMenu);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Menu test program"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);

  /* Run it */
  pgEventLoop();

  return 0;
}

/* The End */
