/* $Id$
 * 
 * omnibar-helio.c - omnibar, hacked up a bit to look good on Helio
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

#include <sys/types.h>   /* For making directory listings */
#include <dirent.h>
#include <sys/stat.h>

#include <time.h>        /* For clock */


/* FIXME: Check for Mac OS X using autoconf */
#if (defined(__APPLE__) && defined(__MACH__)) // Mac OS X and Darwin 
#include <sys/types.h>
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

#include <stdio.h>       /* file IO for getting CPU load */

#include <picogui.h>
#include <unistd.h>

pghandle wClock,wBatt;

/********* Event handlers */

/* Applications menu
 * This whole thing's a kludge, I will write
 * something better (using custom menus) soon
 */
int btnAppMenu(struct pgEvent *evt) {
  pghandle *items;
  struct dirent *dent;
  int i,l;
  DIR *d;

  d = opendir("demos");

  /* FIXME : Count the items and allocate the array */
  items = alloca(sizeof(pghandle) * 40);
  
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
    if (!vfork()) {
      execlp(buf,buf,NULL);
      pgMessageDialogFmt("Error",0,"There was an error starting the\nfollowing program:\n%s",buf);
      exit(1);
    }
  }

  /* Free the server memory and destroy our menu */
  pgLeaveContext();

  closedir(d);

  return 0;
}

/* System menu */
int btnSysMenu(struct pgEvent *evt) {
  switch (pgMenuFromString("About PicoGUI...|Scotty, beam me out!")) {
     
   case 1:
   
     /* Quick little about box */
     pgMessageDialog("About PicoGUI",
		     "Welcome to PicoGUI!\n"
		     "This is the preview release\n"
		     "for the VTech Helio. It is\n"
		     "intended to be a demo of\n"
		     "PicoGUI's current state, many\n"
		     "things will change before\n"
		     "the first 'stable' release. For\n"
		     "the latest code and news:\n\n"
		     "  http://pgui.sourceforge.net",0);
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
  time_t now;
  FILE *f;
  char buf[50];
  struct tm *ltime;
  int bat_raw, bat_percent;
   
  /* Get time */
  time(&now);
  ltime = localtime(&now);
  pgReplaceTextFmt(wClock,"%02d:%02d",
		   ltime->tm_hour,ltime->tm_min);
  pgFlushRequests();
  pgSubUpdate(wClock);

  /* Get battery status */
  f = fopen("/proc/r39xxpm","r");
  if (f) {
     /* I don't think the format of /proc/r39xxpm is set in stone, so
      * this might break? Oh well, works for now. */
     fgets(buf,50,f);  /* CPU speed */
     fgets(buf,50,f);  /* Battery voltage */
     fclose(f);
     bat_raw = atoi(buf+9); /* Skip past "Battery: " */
     
     /* Translate to percent, assuming 259 (2.0 volts) is empty
      * and 402 (3.0 volts) is full. */
     bat_percent = ((bat_raw - 259) * 100) / 143;
     if (bat_percent < 0) bat_percent = 0;
     if (bat_percent > 100) bat_percent = 100;
     
     pgSetWidget(wBatt,PG_WP_VALUE,bat_percent,0);
     pgFlushRequests();
     pgSubUpdate(wBatt);  
  }
}

/********* Main program */

int main(int argc, char **argv) {
  pghandle wLoadbox,fntLabel;

  pgInit(argc,argv);
  pgRegisterApp(PG_APP_TOOLBAR,"OmniBar-Helio",0);

  /* A font for our labels */
  fntLabel = pgNewFont(NULL,10,PG_FSTYLE_FIXED);

  /* Top-level widgets */

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("App"),
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnAppMenu,NULL);

  pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Sys"),
	      PG_WP_EXTDEVENTS,PG_EXEV_PNTR_DOWN,
	      0);
  pgBind(PGDEFAULT,PG_WE_PNTR_DOWN,&btnSysMenu,NULL);

  wClock = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_FONT,fntLabel,
	      PG_WP_TRANSPARENT,0,
	      0);

  pgNewWidget(PG_WIDGET_BOX,0,0);
  pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_INSIDE,PGDEFAULT);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Batt:"),
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_TRANSPARENT,1,
	      0);
  wBatt = pgNewWidget(PG_WIDGET_INDICATOR,0,0);
  pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_ALL,0);

  /* Run it. */
  pgSetIdle(5000,&sysIdle);
  pgEventLoop();
   
  return 0;
}
   
/* The End */
