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
