/* $Id$
 *
 * browserwin.c - User interface for a browser window in Atomic Navigator
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
#include <malloc.h>
#include <string.h>
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

/* Internal function to set the URL */
void browserwin_seturl(struct browserwin *w, const char *url);

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
  return 0;
}

int btnBack(struct pgEvent *evt) {
  struct browserwin *w = (struct browserwin *) evt->extra;

  return 0;
}

int btnForward(struct pgEvent *evt) {
  struct browserwin *w = (struct browserwin *) evt->extra;

  return 0;
}

int evtMessage(struct pgEvent *evt) {
  struct browserwin *w = (struct browserwin *) evt->extra;
  char *command, *param;
  
  /* Separate message into command and parameter by the first space */
  command = strdup(evt->e.data.pointer);
  param = strchr(command,' ');
  if (param) {
    *param = 0;
    param++;
  }
  else
    param = "";

  if (!strcmp(command, "URL")) {
    browserwin_seturl(w,param);
    pgEnterContext();
    pgSetWidget(w->wURL,
		PG_WP_TEXT,pgNewString(param),
		0);
    pgLeaveContext();
  }

  free(command);
  return 0;
}

/********************************* Callbacks */

void pageStatus(struct url *u) {
  browserwin_showstatus(u->browser, u);

  /* Done loading a page? */
  if (u->status == URL_STATUS_DONE) {
    pgEnterContext();
    pgSetWidget(u->browser->wView,
		PG_WP_TEXTFORMAT, pgNewString("html"),
		PG_WP_INSERTMODE, PG_INSERT_OVERWRITE,
		PG_WP_TEXT,pgDataString(u->data),
		0);
    pgLeaveContext();
  }

  pgUpdate();
}

void pageProgress(struct url *u) {
  static int old_progress = -1;
  
  if (u->progress != old_progress) {
    pgSetWidget(u->browser->wProgress,
		PG_WP_VALUE, u->progress,
		0);
    old_progress = u->progress;
    pgSubUpdate(u->browser->wProgress);
    pgFlushRequests();
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
  pgBind(PGDEFAULT,PG_WE_APPMSG,evtMessage,(void*) w);
  
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

  pgEnterContext();
  pgSetWidget(w->wView,
	      PG_WP_TEXTFORMAT,pgNewString("html"),
	      PG_WP_INSERTMODE, PG_INSERT_OVERWRITE,
	      PG_WP_TEXT,pgNewString("<font size=+5>Error:</font><hr><p>"),
	      PG_WP_INSERTMODE, PG_INSERT_APPEND,
	      PG_WP_TEXT,pgNewString(msg),
	      0);
  pgLeaveContext();
}

/* By passing messages using pgAppMessage, we will only receive them when we're
 * in pgEventLoop. This is important for many things, including URL loading.
 * We don't want to be loading a new URL while we're still parsing the first
 * one, for example. This gets pgserver to do the queueing for us :)
 */
void browserwin_command(pghandle w, const char *command, const char *param) {
  char *buf = malloc( strlen(param) + strlen(command) + 5 );
  sprintf(buf,"%s %s",command,param);
  pgAppMessage(w, pgFromTempMemory(buf, strlen(buf)));
}

/* The End */









