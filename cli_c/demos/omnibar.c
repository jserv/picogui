/* $Id: omnibar.c,v 1.3 2000/11/18 06:50:16 micahjd Exp $
 * 
 * omnibar.c - hopefully this will grow into a general interface
 *             for starting and manipulating applications, but
 *             for now it's pretty simple.
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
 * Contributors:
 * 
 * 
 * 
 */

#include <picogui.h>

#include <sys/types.h>   /* For making directory listings */
#include <dirent.h>
#include <sys/stat.h>

#include <malloc.h>      /* Dynamic memory is used for the array */

pghandle wClock;

/********* Event handlers */

/* Applications menu
 * This whole thing's a kludge, I will write
 * something better (using custom menus) soon
 */
int btnAppMenu(short event,pghandle from,long param) {
  pghandle *items;
  struct dirent *dent;
  int i,l;
  DIR *d;

  d = opendir("demos");

  /* FIXME : Count the items and allocate the array */
  items = malloc(sizeof(pghandle) * 40);
  
  /* Enter a new context before making the handles */
  pgEnterContext();

  /* Make handles */
  rewinddir(d);
  i = 0;
  while (dent = readdir(d)) {
    /* Skip all but applications */
    l = strlen(dent->d_name);
    if (l<4) continue;
    if (strcmp(dent->d_name+l-4,".app")) continue;

    /* Strip extension */
    dent->d_name[l-4] = 0;

    /* Add item */
    items[i++] = pgNewString(dent->d_name);
  }

  /* Run it */
  i = pgMenuFromArray(items,i);

  /* Result? */
  if (i) {
    char buf[80];  /* FIXME: Buffer overflow 'sploit waiting to happen! */
    strcpy(buf,"demos/");
    strcat(buf,pgGetString(items[i-1]));
    strcat(buf,".app");
    if (!fork()) {
      execv(buf,NULL);
      pgMessageDialogFmt("Error",0,"There was an error starting the\nfollowing program:\n%s",buf);
      exit(1);
    }
  }

  /* Free memory for the item array and get rid of the handles. */
  free(items);
  pgLeaveContext();

  closedir(d);

  return 0;
}

/* System menu */
int btnSysMenu(short event,pghandle from,long param) {
  switch (pgMenuFromString("About PicoGUI...|Scotty, beam me out!")) {
     
   case 1:
   
     /* Quick little about box */
     pgMessageDialog("About PicoGUI",
		     "Welcome to PicoGUI!\n"
		     "This is a preview release (or a development\n"
		     "version if you're a developer :)\n"
		     "What you see may or may not represent the\n"
		     "future of PicoGUI, it is just a test.\n"
		     "For more information and the latest code, visit:\n"
		     "     http://pgui.sourceforge.net\n"
		     "\n"
		     "-- Micah",0);
     break;

   case 2:
     /* Beam us out! */
     if (pgMessageDialog("Beam out...","Make it so?",
			 PG_MSGBTN_YES | PG_MSGBTN_NO)==PG_MSGBTN_YES)
       exit(0);
  }
}

/* Called to update the clock and system load indicators */
void sysIdle(void) {
   static int n = 0;
   pgReplaceTextFmt(wClock,"[%d]",++n);
}

/********* Main program */

int main(int argc, char **argv) {
  pgInit(argc,argv);
  pgRegisterApp(PG_APP_TOOLBAR,"OmniBar",
		PG_APPSPEC_SIDE,PG_S_BOTTOM,0);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Applications"),
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnAppMenu);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("System"),
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnSysMenu);

  wClock = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_FONT,pgNewFont(NULL,10,PG_FSTYLE_FIXED),
	      PG_WP_TRANSPARENT,0,
	      0);

  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Welcome to PicoGUI (preview release)"),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_FONT,pgNewFont("Times",10,PG_FSTYLE_BOLD),
	      0);

  /* Run it. */
  pgSetIdle(1000,&sysIdle);
  pgEventLoop();
   
  return 0;
}
   
/* The End */
