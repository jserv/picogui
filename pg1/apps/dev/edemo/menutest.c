/*  $Id$
 *
 * menutest.c - based on , PicoGUI's "menutest"
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
 *
 *  Modified by: Daniele Pizzoni - Ascensit s.r.l. - Italy
 *  tsho@ascensit.com - auouo@tin.it
 *
 *  Based on picosm (by ?) and pgdemo.py by Micah Dowty
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

/* The simplest way to make a menu */
int btnMenuFromString(struct pgEvent *evt) {
  pgMessageDialogFmt("Menu results",0,"pgMenuFromString() returned %d",
		     pgMenuFromString("Hello World!|This is a test...|1\n2\n3"));
  return 0;
}

/* A little more work, but the best way to make dynamic menus.
 * This example makes a directory listing into a menu */
int btnMenuFromArray(struct pgEvent *evt) {
  pghandle *items;
  struct dirent *dent;
  int i,num = 0;
  DIR *d;

  d = opendir("/usr");

  /* Count the items and allocate the array */
  while (readdir(d)) num++;
  items = alloca(sizeof(pghandle) * num);
  
  /* Enter a new context before making the handles */
  pgEnterContext();

  /* Make handles */
  rewinddir(d);
  i = 0;
  while ((dent = readdir(d)))
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
  pgNewPopupAt(PG_POPUP_ATEVENT,PG_POPUP_ATEVENT,
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

  pgNewWidget(PG_WIDGET_LABEL,0,0);
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
  pgNewWidget(PG_WIDGET_LABEL,0,0);
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
  return 0;
}

/****** Main program */

int menutest(pghandle widget)
{
 /*  pgInit(argc,argv); */

/*   pgRegisterApp(PG_APP_TOOLBAR,"msgdialog demo",0); */

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,widget);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("String"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      PG_WP_HOTKEY,PGKEY_F1,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnMenuFromString,NULL);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Array"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      PG_WP_HOTKEY,PGKEY_F2,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnMenuFromArray,NULL);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Custom"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      PG_WP_HOTKEY,PGKEY_F3,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnCustomMenu,NULL);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Dialog"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_HOTKEY,PGKEY_F4,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnDialog,NULL);

/*   pgNewWidget(PG_WIDGET_BUTTON,0,0); */
/*   pgSetWidget(PGDEFAULT, */
/* 	      PG_WP_TEXT,pgNewString("X"), */
/* 	      PG_WP_SIDE,PG_S_RIGHT, */
/* 	      0); */
/*   pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnExit,NULL); */

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
/* 	      PG_WP_TEXT,pgNewString("Menu test program"), */
	      PG_WP_SIDE,PG_S_LEFT,
	      0);

  /* Run it */
  pgEventLoop();

  return 0;
}

/* The End */
