/* $Id: browserwin.c,v 1.3 2002/01/07 06:28:08 micahjd Exp $
 *
 * browserwin.c - User interface for a browser window in Atomic Navigator
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <malloc.h>
#include "browserwin.h"
#include "url.h"
#include "protocol.h"

/* Corresponding to URL_STATUS_* constants */
const char *status_names[] = {
  "Idle.",
  "Resolving page name...",
  "Connecting...",
  "Sending data...",
  "Loading page...",
  "Done.",
  "Error loading page!",
  "Stopped.",
  "Using cached copy.",
};

/********************************* GUI Events */

int btnGo(struct pgEvent *evt) {
  struct browserwin *w = (struct browserwin *) evt->extra;

  browserwin_seturl(w,pgGetString(pgGetWidget(w->wURL,PG_WP_TEXT)));
  return 0;
}

int btnStop(struct pgEvent *evt) {
  struct browserwin *w = (struct browserwin *) evt->extra;

  if (w->page) {
    url_delete(w->page);
    w->page = NULL;
  }
  browserwin_showstatus(w,NULL);
}

int btnBack(struct pgEvent *evt) {
  struct browserwin *w = (struct browserwin *) evt->extra;

}

int btnForward(struct pgEvent *evt) {
  struct browserwin *w = (struct browserwin *) evt->extra;

}

/********************************* Callbacks */

void pageStatus(struct url *u) {
  browserwin_showstatus(u->browser, u);

  printf("Status: %d\n" ,u->status);
}

void pageProgress(struct url *u) {
  static int old_progress = -1;
  
  if (u->progress != old_progress) {
    pgSetWidget(u->browser->wProgress,
		PG_WP_VALUE, u->progress,
		0);
    old_progress = u->progress;
  }
}

/********************************* Methods */

struct browserwin *browserwin_new(void) {
  struct browserwin *w;

  /* Allocate blank browserwin */

  w = malloc(sizeof(struct browserwin));
  if (!w)
    return NULL;
  memset(w,0,sizeof(struct browserwin));

  /* Top-level widgets */

  w->wApp = pgRegisterApp(PG_APP_NORMAL,BROWSER_TITLE,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_NAME,pgNewString(BROWSER_NAME),
	      0);
  
  w->wNavTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_TOP,
	      0);

  w->wStatusTB = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_BOTTOM,
	      0);

  w->wMainBox = pgNewWidget(PG_WIDGET_BOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);

  /* Inside wMainBox */

  w->wScroll = pgNewWidget(PG_WIDGET_SCROLL,PG_DERIVE_INSIDE,w->wMainBox);

  w->wView = pgNewWidget(PG_WIDGET_TEXTBOX,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      0);

  pgSetWidget(w->wScroll,
	      PG_WP_BIND,w->wView,
	      0);

  /* Status toolbar */

  w->wProgress = pgNewWidget(PG_WIDGET_INDICATOR,PG_DERIVE_INSIDE,w->wStatusTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIZEMODE, PG_SZMODE_PERCENT,
	      PG_WP_SIZE, 25,
	      PG_WP_SIDE, PG_S_RIGHT,
	      0);

  w->wStatus = pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,  PG_S_ALL,
	      PG_WP_ALIGN, PG_A_LEFT,
	      0);  

  /* Navigation toolbar */

  w->wBack = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,w->wNavTB);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("<"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,btnBack,(void*) w);

  w->wForward = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString(">"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,btnForward,(void*) w);

  w->wStop = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("Stop"),
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,btnStop,(void*) w);

  w->wGo = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("Go"),
	      PG_WP_SIDE, PG_S_RIGHT,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,btnGo,(void*) w);

  w->wURL = pgNewWidget(PG_WIDGET_FIELD,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE, PG_S_ALL,
	      0);
  pgBind(PGDEFAULT,PG_WE_ACTIVATE,btnGo,(void*) w);
  pgFocus(PGDEFAULT);

  /* No URL yet */
  browserwin_showstatus(w, NULL);

  return w;
}

/* Show a URL's status in the browser window */
void browserwin_showstatus(struct browserwin *w, struct url *u) {
  if (u) {
    w->status = u->status;
    pageProgress(u);
  }
  else {
    /* No current URL */

    w->status = URL_STATUS_IDLE;
    pgSetWidget(w->wProgress,
		PG_WP_VALUE, -1,
		0);
  }

  pgReplaceText(w->wStatus, status_names[w->status]);
  pgSetWidget(w->wStop,
	      PG_WP_DISABLED, 
	      w->status == URL_STATUS_IDLE || 
	      w->status == URL_STATUS_DONE || 
	      w->status == URL_STATUS_ERROR,
	      0);
}

void browserwin_seturl(struct browserwin *w, const char *url) {
  if (w->page)
    url_delete(w->page);

  w->page = url_new(w,url);
  if (!w->page)
    return;

  w->page->status_change = pageStatus;
  w->page->progress_change = pageProgress;
  w->page->handler->connect(w->page);
}

void browserwin_errormsg(struct browserwin *w, const char *msg) {
  /* Display the error message on a web page */

  /* FIXME: This is a memory leak! */
  pgSetWidget(w->wView,
	      PG_WP_TEXTFORMAT,pgNewString("HTML"),
	      PG_WP_TEXT,pgNewString("<font size=+5>Error:</font><hr><p>"),
	      PG_WP_TEXTFORMAT,pgNewString("+HTML"),
	      PG_WP_TEXT,pgNewString(msg),
	      0);
}


/* The End */









