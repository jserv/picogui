/*
 *
 * listtest.c - Test program for list widget
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2001 RidgeRun Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * Todd Fischer
 * 
 * 
 */
#include <picogui.h>

#include <sys/types.h>   /* For making directory listings */
#include <dirent.h>

/* FIXME: Check for Mac OS X using autoconf */
#if (defined(__APPLE__) && defined(__MACH__)) // Mac OS X and Darwin 
#include <sys/types.h>
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

//
// Globals
//
static pghandle listdemo;
static pghandle scroll;
static pghandle boundingbox;

/* List that doesn't require scroll bars */

int btnShortList(struct pgEvent *evt) {
}

/* Scrollable list */

int btnKeyPressed(struct pgEvent *evt) {
   printf(__FUNCTION__"\n");

   if ( evt->from == boundingbox && evt->e.kbd.key == PGKEY_KP_ENTER ) {
      printf(__FUNCTION__": Selected position = %d\n", pgGetWidget(boundingbox, PG_WP_SELECTED));
      printf(__FUNCTION__": Selected handle = %d\n", pgGetWidget(boundingbox, PG_WP_SELECTED_HANDLE));
   }
   
   return 0;
}

int btnScrollableList(struct pgEvent *evt) {

#define MAX_ROWS 50
   pghandle row[MAX_ROWS];
   char str[128];
   int i;
   pgcontext canvasContext;
   pghandle sHandle;
   int w, h;
   pghandle foo;

   scroll = pgNewWidget (PG_WIDGET_SCROLL, PG_DERIVE_INSIDE, listdemo);
   boundingbox = pgNewWidget (PG_WIDGET_LIST, 0, 0);
   pgSetWidget (PGDEFAULT, PG_WP_SIDE, PG_S_ALL, 0);
   pgSetWidget (scroll, PG_WP_BIND, boundingbox, 0);
   
   for ( i=0; i < MAX_ROWS; i++ ) {
      row[i] = pgCreateWidget(PG_WIDGET_BOX, 0);
      pgSetWidget(row[i], PG_WP_TRANSPARENT, 1, 0);
      sprintf(str, "This is row %d", i);
      pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, row[i]);
      pgSetWidget(PGDEFAULT, PG_WP_TRANSPARENT, 0, PG_WP_TEXT, pgNewString(str), 0);   
      pgListInsertAt(boundingbox, row[i], i);
   }

   pgFocus(boundingbox);
   pgSetWidget(boundingbox, PG_WP_SELECTED, 0, 0);
   pgUpdate();
   pgBind(boundingbox, PG_WE_KBD_KEYUP, btnKeyPressed, NULL);
   
}

int junk (void) {

   pghandle *items;
  struct dirent *dent;
  int i,num = 0;
  DIR *d;

  d = opendir("/home/tfischer");

  /* Count the items and allocate the array */
  while (readdir(d)) num++;
  items = alloca(sizeof(pghandle) * num);
  
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
  pgLeaveContext();

  closedir(d);

  return 0;
}

/* The most difficult way to make menus, but this allows
   you to put any arbitrary widgets in the menu and
   customize to your heart's content */
int btnCustomMenu(struct pgEvent *evt) {
  pghandle result,toolbar;

  /* Do our own context management */
  pgEnterContext();

  /* Create a popup menu at the cursor with automatic sizing */
  pgNewPopupAt(PG_POPUP_ATCURSOR,PG_POPUP_ATCURSOR,
	       PGDEFAULT,PGDEFAULT);

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
		PG_WP_BITMAP,pgNewBitmap(pgFromFile("data/tux.pnm")),
		PG_WP_BITMASK,pgNewBitmap(pgFromFile("data/tux_mask.pnm")),
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
		PG_WP_BITMAP,pgNewBitmap(pgFromFile("data/dustpuppy.pnm")),
		PG_WP_BITMASK,pgNewBitmap(pgFromFile("data/dustpuppy_mask.pnm")),
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
  result = pgGetWidget(pgGetEvent()->from,PG_WP_TEXT);

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

/* Not really a menu, but still showing off popup boxen :) */
int btnDialog(struct pgEvent *evt) {
  int result;
  pghandle wToolbar;
  pgcontext gc;
	
  /* So we can easily clean up this mess later */
  pgEnterContext();

  /* Dialog box with title */
  pgDialogBox("Nifty Custom Dialog Box of Doom!");

  /* Make some widgets... */

  wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_BOTTOM,0);

  /* Bitmap image */
  pgNewWidget(PG_WIDGET_BITMAP,0,0);
  pgSetWidget(PGDEFAULT,
	      /* Load a picture from the 'data' directory */
	      PG_WP_BITMAP,pgNewBitmap(pgFromFile("data/dustpuppy.pnm")),
	      PG_WP_BITMASK,pgNewBitmap(pgFromFile("data/dustpuppy_mask.pnm")),
	      PG_WP_LGOP,PG_LGOP_OR,
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
	
  /* Line drawing */
  pgNewWidget(PG_WIDGET_CANVAS,0,0);                   /* Create a canvas */
  pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_RIGHT,0);
  gc = pgNewCanvasContext(PGDEFAULT,PGFX_PERSISTENT);  /* Get a PGFX context */
  pgMoveTo(gc,50,0);                                   /* Draw a star */
  pgLineTo(gc,75,100);
  pgLineTo(gc,0,35);
  pgLineTo(gc,100,35);
  pgLineTo(gc,25,100);
  pgLineTo(gc,50,0);	
  pgSetLgop(gc,PG_LGOP_STIPPLE);
  pgSetFont(gc,pgNewFont(NULL,20,PG_FSTYLE_UNDERLINE));
  pgText(gc,10,110,pgNewString("canvas\nsizing\ntoo!"));
  pgDeleteContext(gc);                                 /* Clean up */

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Hi, Everybody!\n\n"
				     "PicoGUI finally supports real dialog\n"
				     "boxes with automatic sizing of all\n"
				     "containers! Now all it needs is auto\n"
				     "word wrapping for this pesky 'text'\n"
				     "stuff..."),
	      0);
				  
  pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("I'm Impressed!"),0);

  pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("I'm scared..."),0);

  pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_TEXT,pgNewString("Dust Puppy is cute :)"),0);
  

  /* Buttons that can exit the dialog have a return code stored in the payload */

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Ok"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);
  pgSetPayload(PGDEFAULT,1);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Apply"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Cancel"),
	      PG_WP_SIDE,PG_S_LEFT,
	      0);
  pgSetPayload(PGDEFAULT,2);

  /* Wait until a button with an associated payload (in this case a
   * return value) is selected */
  while (!(result = pgGetPayload(pgGetEvent()->from)));

  pgMessageDialogFmt("Dialog Closed",0,"Result code #%d",result);

  /* Free all this memory we used */
  pgLeaveContext();

  return 0;
}

int btnExit(struct pgEvent *evt) {
  exit(0);
}

/****** Main program */

int main(int argc, char *argv[])
{
  pgInit(argc,argv);

  listdemo=pgRegisterApp(PG_APP_TOOLBAR,"list demo",0);


  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("ShortList"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnShortList,NULL);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("ScrollingList"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnScrollableList,NULL);

  /* Run it */
  pgEventLoop();

  return 0;
}

/* The End */
