/* $Id$
 * 
 * pgl-clock.c - This is a simple clock applet for PGL
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <time.h>
#include <string.h>

/* Maximum length of clock format strings */
#define FMTMAX 30

/* Maximum length of clock contents */
#define CLKMAX 50

/* The toolbar */
pghandle pglToolbar;

/* The toolbar's response variable */
char *pglToolbarResponse;

/* Clock settings */
struct clockData {
  /* The clock itself */
  pghandle wClock;

  /* Settings */
  pghandle fClockFont;
  unsigned int flashColon : 1;
  unsigned int enable24hour : 1;
  unsigned int enableSeconds : 1;
  unsigned int enableWeekDay : 1;
  unsigned int enableDay : 1;
  unsigned int enableMonth : 1;
  unsigned int enableYear : 1;

  /* Format strings- these are generated from the above settings
   * by the mungeSettings function
   */
  char fmt1[40];
  char fmt2[40];

} currentClock;

/* Functions */
void loadSettings(void);
void storeSettings(void);
void recieveMessage(struct pgEvent *evt);
int btnDialog(struct pgEvent *btnevt);
void mungeSettings(void);
void updateTime(void);

void loadSettings(void){
  
  pgAppMessage(pglToolbar, pglBuildMessage(PGL_GETPREF, "PGL-Clock", "flashColon", ""));
  recieveMessage(pgGetEvent());
  if(!strcmp(pglToolbarResponse, "Y")){
    currentClock.flashColon = 1;
  }else{
    currentClock.flashColon = 0;
  }
  
  pgAppMessage(pglToolbar, pglBuildMessage(PGL_GETPREF, "PGL-Clock", "enable24Hour", ""));
  recieveMessage(pgGetEvent());
  if(!strcmp(pglToolbarResponse, "Y")){
    currentClock.enable24hour = 1;
  }else{
    currentClock.enable24hour = 0;
  }
  
  pgAppMessage(pglToolbar, pglBuildMessage(PGL_GETPREF, "PGL-Clock", "enableSeconds", ""));
  recieveMessage(pgGetEvent());
  if(!strcmp(pglToolbarResponse, "Y")){
    currentClock.enableSeconds = 1;
  }else{
    currentClock.enableSeconds = 0;
  }
  
  pgAppMessage(pglToolbar, pglBuildMessage(PGL_GETPREF, "PGL-Clock", "enableWeekday", ""));
  recieveMessage(pgGetEvent());
  if(!strcmp(pglToolbarResponse, "Y")){
    currentClock.enableWeekDay = 1;
  }else{
    currentClock.enableWeekDay = 0;
  }

  pgAppMessage(pglToolbar, pglBuildMessage(PGL_GETPREF, "PGL-Clock", "enableDay", ""));
  recieveMessage(pgGetEvent());
  if(!strcmp(pglToolbarResponse, "Y")){
    currentClock.enableDay = 1;
  }else{
    currentClock.enableDay = 0;
  }

  pgAppMessage(pglToolbar, pglBuildMessage(PGL_GETPREF, "PGL-Clock", "enableMonth", ""));
  recieveMessage(pgGetEvent());
  if(!strcmp(pglToolbarResponse, "Y")){
    currentClock.enableMonth = 1;
  }else{
    currentClock.enableMonth = 0;
  }

  pgAppMessage(pglToolbar, pglBuildMessage(PGL_GETPREF, "PGL-Clock", "enableYear", ""));
  recieveMessage(pgGetEvent());
  if(!strcmp(pglToolbarResponse, "Y")){
    currentClock.enableYear = 1;
  }else{
    currentClock.enableYear = 0;
  }
}

void storeSettings(void){
  
  if(currentClock.

/* Options Dialog */
int btnDialog(struct pgEvent *btnevt) {
  struct pgEvent evt;
  pghandle wSampleTB,wOk,wCancel,wSetFont,wOptionBox,wTB;
  pghandle w24hour,wSeconds,wColon,wWeekDay,wDay,wMonth,wYear;
  struct clockData oldData;

  /* Save data in case of cancel */
  oldData = currentClock;

  /* Top-level widgets */

  pgEnterContext();
  pgDialogBox("Clock Preferences");

  wTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);

  wSampleTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);

  wOptionBox = pgNewWidget(PG_WIDGET_BOX,0,0);

  pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_BEFORE,wOptionBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_BIND,wOptionBox,
	      0);

  /* Options */

  w24hour = pgNewWidget(PG_WIDGET_CHECKBOX,PG_DERIVE_INSIDE,wOptionBox);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("24-hour clock"),
	      PG_WP_ON,currentClock.enable24hour,
	      0);
  wSeconds = pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Show seconds"),
	      PG_WP_ON,currentClock.enableSeconds,
	      0);
  wColon = pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Flash colon"),
	      PG_WP_ON,currentClock.flashColon,
	      0);
  wSetFont = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Set Font"),
	      PG_WP_SIDE,PG_S_TOP,
	      0);
  wWeekDay = pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Show weekday"),
	      PG_WP_ON,currentClock.enableWeekDay,
	      0);
  wDay = pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Show day"),
	      PG_WP_ON,currentClock.enableDay,
	      0);
  wMonth = pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Show month"),
	      PG_WP_ON,currentClock.enableMonth,
	      0);
  wYear = pgNewWidget(PG_WIDGET_CHECKBOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Show year"),
	      PG_WP_ON,currentClock.enableYear,
	      /* Give the last one PG_S_ALL to avoid extra margin at the
	       * bottom of wCheckBoxBox
	       */
	      PG_WP_SIDE,PG_S_ALL,
	      0);

  /* Buttons */

  wCancel = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
				       PGTH_P_STRING_CANCEL),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_HOTKEY_CANCEL),
	      PG_WP_BITMAP,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_CANCEL),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_CANCEL_MASK),
	      0);
  wOk = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
				       PGTH_P_STRING_OK),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_HOTKEY_OK),
	      PG_WP_BITMAP,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_OK),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_OK_MASK),
	      0);

  /* Sample clock */

  currentClock.wClock = pgNewWidget(PG_WIDGET_FLATBUTTON,PG_DERIVE_INSIDE,
				    wSampleTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);

  /* Run the dialog */
  for (;;) {
    mungeSettings();                /* New settings take effect */
    evt = *pgGetEvent();            /* Wait for an event */

    /* Set Font */
    if (evt.from == wSetFont) {
      pghandle fNew, fOld;
      fNew = pgFontPicker("Set Clock Font");
      if (fNew) {
	fOld = currentClock.fClockFont;
	/* Send new font handle to the main context */
	pgChangeContext(fNew,-1);
	/* Select new one, delete old one
	 * (unless this was the original font we entered with.
	 * Then, keep it around because we might cancel this dialog) */
	currentClock.fClockFont = fNew;
	if (fOld != oldData.fClockFont)
	  pgDelete(fOld);
      }
    }

    /* Cancel - revert everything */
    else if (evt.from == wCancel) {
      currentClock = oldData;
      break;
    }

    /* Done. Revert widget, delete old font */
    else if (evt.from == wOk) {
      if (oldData.fClockFont != currentClock.fClockFont)
	pgDelete(oldData.fClockFont);
      currentClock.wClock = oldData.wClock;
      break;
    }

    /* Set various flags */
    else if (evt.from == wColon)
      currentClock.flashColon ^= 1;
    else if (evt.from == w24hour)
      currentClock.enable24hour ^= 1;
    else if (evt.from == wSeconds)
      currentClock.enableSeconds ^= 1;
    else if (evt.from == wWeekDay)
      currentClock.enableWeekDay ^= 1;
    else if (evt.from == wDay)
      currentClock.enableDay ^= 1;
    else if (evt.from == wMonth)
      currentClock.enableMonth ^= 1;
    else if (evt.from == wYear)
      currentClock.enableYear ^= 1;
  }
  pgLeaveContext();
  mungeSettings();
  return 0;
}

/* Prepare settings for use in updateTime */
void mungeSettings(void) {
  unsigned long t;

  /* Set clock font */
  pgSetWidget(currentClock.wClock,
	      PG_WP_FONT,currentClock.fClockFont,
	      0);
  
  /* Figure out the necessary update time */
  if (currentClock.flashColon)
    t = 500;
  else if (currentClock.enableSeconds)
    t = 1000;
  else
    t = 60000;
  pgSetIdle(t,&updateTime);

  /* Build a format string */
  currentClock.fmt1[0] = 0;              /* Start out blank */
  if (currentClock.enableWeekDay)
    strcat(currentClock.fmt1,"%a ");
  if (currentClock.enableMonth)
    strcat(currentClock.fmt1,"%b ");
  if (currentClock.enableDay)
    strcat(currentClock.fmt1,"%e ");
  if (currentClock.enable24hour)
    strcat(currentClock.fmt1,"%k");
  else
    strcat(currentClock.fmt1,"%l");
  strcat(currentClock.fmt1,":%M");
  if (currentClock.enableSeconds)        /* Careful with the colons... */
    strcat(currentClock.fmt1,":%S ");
  else
    strcat(currentClock.fmt1," ");
  if (!currentClock.enable24hour)
    strcat(currentClock.fmt1,"%p ");
  if (currentClock.enableYear)
    strcat(currentClock.fmt1,"%Y ");
  currentClock.fmt1[strlen(currentClock.fmt1)-1] = 0;  /* Chop extra space */

  /* Make secondary format string with optional colon flashing */
  strcpy(currentClock.fmt2,currentClock.fmt1);
  if (currentClock.flashColon) {
    /* Convert all colons to blanks */
    char *p = currentClock.fmt2;
    while (p = strchr(p,':'))
      *p = ' ';
  }

  /* Resize the applet if necessary */
  updateTime();
  pgUpdate();
}

/* Update the current clock time */
void updateTime(void) {
   time_t now;
   struct tm *ltime;
   char buf[CLKMAX];
   static int blink = 0;

   time(&now);
   ltime = localtime(&now);
   strftime(buf,CLKMAX,blink ? currentClock.fmt1 : currentClock.fmt2,ltime);
   blink = !blink;
   pgReplaceText(currentClock.wClock,buf);
   pgSubUpdate(currentClock.wClock);
}

int main(int argc,char **argv) {
  pghandle wPGLbar;
  pgInit(argc,argv);

  /* Find the applet container */
  wPGLbar = pgFindWidget("PGL-AppletBar");
  if (!wPGLbar) {
    pgMessageDialog(argv[0],"This applet requires PGL",PG_MSGICON_ERROR);
    return 1;
  }
 
  /* Clock widget. Make it a flatbutton so it normally looks fairly
   * unobtrusive, but it can be clicked. Clicking the clock launches
   * an options dialog box.
   */
  currentClock.wClock = pgNewWidget(PG_WIDGET_FLATBUTTON,
				    PG_DERIVE_INSIDE,wPGLbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,&btnDialog,NULL);

  /* Set up default clock */
  currentClock.fClockFont = pgNewFont(NULL,0,PG_FSTYLE_FIXED);
  mungeSettings();

  pgEventLoop();
  return 0;
}

/* The End */




