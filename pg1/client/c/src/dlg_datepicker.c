/* $Id$
 *
 * dlg_datepicker.c - Implementation of the pgDatePicker() function. Display
 *                    a date on a calendar, and allow the user to select a
 *                    new date.
 *
 * The dialog displays a Gregorian calendar, with English day and month names.
 * This needs to be internationalized somehow.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors: 
 * 
 * 
 * 
 * 
 */

#include <stdlib.h>
#include <time.h>
#include "clientlib.h"

#include <time.h>     /* Calendars need time */

const char *months[] = {
  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};
const char *weekdays[] = {
  "Su","Mo","Tu","We","Th","Fr","Sa"
};

/* Draw a calendar for the datepicker.
 *
 * The calendar is drawn inside the 'canvas' canvas widget for the
 * specified year and month. The title for the calendar is put into the
 * 'title' label widget. The fHeading font is used for headings, and the
 * day in 'day' is labeled with the fDay font.
 *
 * For reference when the calendar is clicked, the position
 * of the first day and the number of days are returned in firstday and
 * numdays;
 */
void datepicker_drawmonth(pghandle canvas, pghandle title, 
			  int year,int month,int *day,
			  pghandle fHeading,
			  int *firstday, int *numdays) {

  char tmpstr[3];
  struct tm then;
  time_t just_then;
  int i, first_weekday, x,y;
  pgcontext gc;
  pgcolor fg;

  /* clear our month context */
  pgLeaveContext();
  pgEnterContext();

  /* Set up the canvas. This makes a 7x7 grid, each square at least
   * large enough to hold "000", and sets the background and foreground
   * equal to that of a box widget. 
   */
  pgWriteCmd(canvas,PGCANVAS_NUKE,0);
  gc = pgNewCanvasContext(canvas,PGFX_PERSISTENT);
  pgEnterContext();
  pgSizeText(&x,&y,PGDEFAULT,pgNewString("000"));
  pgLeaveContext();
  pgWriteCmd(canvas,PGCANVAS_GRIDSIZE,2,x,y);
  pgSetMapping(gc,0,0,7,7,PG_MAP_SCALE);
  pgSetColor(gc,pgThemeLookup(PGTH_O_BOX,PGTH_P_BGCOLOR));
  pgRect(gc,0,0,7,7);
  pgSetColor(gc,fg = pgThemeLookup(PGTH_O_BOX,PGTH_P_FGCOLOR));

  /* Draw the headings */
  pgSetFont(gc,fHeading);
  for (i=0;i<7;i++)
    pgText(gc,i,0,pgNewString(weekdays[i]));
  pgSetFont(gc,PGDEFAULT);

  /* Find the weekday of the 1st of the month */
  memset(&then,0,sizeof(then));
  then.tm_year = year-1900;
  then.tm_mon = month-1;
  then.tm_mday = 1;
  just_then = mktime(&then);
  first_weekday = localtime(&just_then)->tm_wday;

  /* Find the number of days in the month */
  memset(&then,0,sizeof(then));
  then.tm_year = year-1900;
  then.tm_mon = month;
  just_then = mktime(&then);
  *numdays = localtime(&just_then)->tm_mday;

  /* Make sure we're not about to draw something dumb, like February 31 */
  if (*day > *numdays)
    *day = *numdays;

  /* Setup for calendar drawing. If possible, leave a blank row on top */
  x = first_weekday;
  y = 1 + (first_weekday + *numdays <= 35);
  *firstday = y*7+x;

  /* Actually draw the month */
  for (i=1;i<=*numdays;i++) {
    sprintf(tmpstr,"%d",i);
    
    /* Hilight the selected day */
    if (*day==i) {
      pgSetFont(gc,fHeading);
      pgSetColor(gc,0xFF0000);
      pgText(gc,x,y,pgNewString(tmpstr));
      pgSetFont(gc,PGDEFAULT);
      pgSetColor(gc,fg);
    }
    else
      pgText(gc,x,y,pgNewString(tmpstr));
      
    x++;
    if (x>=7) {
      x=0;
      y++;
    }
  }

  pgContextUpdate(gc);
  pgDeleteContext(gc);

  /* Calendar title */
  pgReplaceTextFmt(title, "%s %d, %d",months[month-1],*day,year);
}

int pgDatePicker(int *year, int *month, int *day, const char *title) {
  pghandle wDateBar,wMonthBar1, wMonthBar2, wButtonBar, wCalendar;
  pghandle wTitle,wCancel,wOk,wCongress,wProgress;
  unsigned long id;
  int i;
  struct pgEvent evt;
  struct tm *now;
  time_t just_now;
  pghandle wMonths[12];       /* Month selector widgets, indexed by payload */
  pghandle fHeading;
  int firstday,numdays;
  int retval = 0;
  int saved_year  = *year;
  int saved_month = *month;
  int saved_day   = *day;     /* Here I come to save the day! */

  pgEnterContext();

  /* Allocate fonts */
  fHeading = pgNewFont(NULL,0,PG_FSTYLE_BOLD | PG_FSTYLE_DEFAULT);

  /********** Container Widgets */

  pgDialogBox(title);
  
  wDateBar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_TOP,
	      0);

  wMonthBar1 = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_TOP,
	      PG_WP_TRANSPARENT,1,
	      0);
  wMonthBar2 = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_TOP,
	      PG_WP_TRANSPARENT,1,
	      0);

  wButtonBar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);

  pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);
  wCalendar = pgNewWidget(PG_WIDGET_CANVAS,PG_DERIVE_INSIDE,PGDEFAULT);

  /********** Year */

  wCongress = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wDateBar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_TEXT,pgNewString("<"),
	      0);

  wProgress = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_TEXT,pgNewString(">"),
	      0);

  wTitle = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);

  /********** Buttons */

  wOk = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wButtonBar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Ok"),
	      PG_WP_HOTKEY,PGKEY_RETURN,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_OK),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_OK_MASK),
	      0);

  wCancel = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString("Cancel"),
	      PG_WP_HOTKEY,PGKEY_ESCAPE,
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_CANCEL),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_CANCEL_MASK),
	      0);

  /********** Months */
 
  for (i=0;i<12;i++) {
    wMonths[i] = pgNewWidget(PG_WIDGET_FLATBUTTON,PG_DERIVE_INSIDE,
			     i<6 ? wMonthBar1 : wMonthBar2);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT,pgNewString(months[i]),
		PG_WP_SIDE,PG_S_RIGHT,
		PG_WP_SIZEMODE,PG_SZMODE_CNTFRACT,
		PG_WP_SIZE,pgFraction(1,6),
		0);
    pgSetPayload(PGDEFAULT,i+1);
  }

  /********** Event loop */
  
  /* Set to today's date if none was provided */
  just_now = time(NULL);
  now = localtime(&just_now);
  if (!*year)
    *year = now->tm_year + 1900;
  if (!*month)
    *month = now->tm_mon + 1;
  if (!*day)
    *day = now->tm_mday;


  /* Draw the months in a separate context */
  pgEnterContext();
  datepicker_drawmonth(wCalendar,wTitle, *year,*month,day,fHeading,
		       &firstday,&numdays);

  for (;;) {
    evt = *pgGetEvent();
    id = pgGetPayload(evt.from);

    /* Cancel button? */
    if (evt.from==wCancel) {
      *year = saved_year;
      *month = saved_month;
      *day = saved_day;
      break;
    }

    /* Ok button? */
    if (evt.from==wOk) {
      retval = 1;
      break;
    }

    /* A month button? */
    else if (id && id<13 && evt.from==wMonths[id-1])
      *month = id;

    /* Year buttons? */
    else if (evt.from==wCongress)
      (*year)--;
    else if (evt.from==wProgress)
      (*year)++;
    
    /* Clicked a day? */
    else if (evt.type==PG_WE_PNTR_DOWN && evt.from==wCalendar) {
      i = evt.e.pntr.x + evt.e.pntr.y*7 - firstday + 1;
      if (i>0 && i <= numdays)
	   *day = i;
    }

    /* Something else we don't care about? */
    else
      continue;

    /* Bounds checking on year. We're using the unix time functions,
     * so the year has to be within the range of the time_t.
     * 
     * So, we can't handle pre-1970. If time_t is 32 bits, we can't get to
     * the year 2038 without trouble. If time_t is 64 bits, we don't have to
     * worry about this.
     */
    if (*year < 1970)
      *year = 1970;
    if (sizeof(time_t)<=32 && *year > 2037)
      *year = 2037;

    /* Change month or year */
    datepicker_drawmonth(wCalendar,wTitle, 
			 *year,*month,day,
			 fHeading,&firstday,&numdays);
  }

  /* 2 contexts, one for the month and one for the dialog */
  pgLeaveContext();
  pgLeaveContext();

  return retval;
}

/* The End */


























