/* $Id: browserwin.c,v 1.1 2002/01/06 12:32:41 micahjd Exp $
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

const char *status_names[] = {
  "Done.",
  "Loading page...",
  "Loading images...",
};

/********************************* GUI Events */

int btnGo(struct pgEvent *evt) {
  struct browserwin *w = (struct browserwin *) evt->extra;
  browserwin_seturl(w,pgGetString(pgGetWidget(w->wURL,PG_WP_TEXT)));
  return 0;
};

/********************************* Callbacks */

void pageLoadCallback(struct url *u) {
  printf("URL callback\n");
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

  w->wForward = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString(">"),
	      0);

  w->wStop = pgNewWidget(PG_WIDGET_BUTTON,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT, pgNewString("Stop"),
	      0);

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

  browserwin_setstatus(w,STATUS_IDLE);

  return w;
}

void browserwin_setstatus(struct browserwin *w, int status) {
  w->status = status;
  pgReplaceText(w->wStatus, status_names[status]);
  pgSetWidget(w->wStop,
	      PG_WP_DISABLED, status == STATUS_IDLE,
	      0);
}

void browserwin_seturl(struct browserwin *w, const char *url) {
  if (w->page)
    url_delete(w->page);
  
  w->page = url_new(url);

  printf("url: %s\n"
	 "protocol: %s\n"
	 "user: %s\n"
	 "password: %s\n"
	 "server: %s\n"
	 "path: %s\n"
	 "filename: %s\n"
	 "anchor: %s\n"
	 "port: %d\n"
	 "\n",
	 w->page->url,
	 w->page->protocol,
	 w->page->user,
	 w->page->password,
	 w->page->server,
	 w->page->path,
	 w->page->filename,
	 w->page->anchor,
	 w->page->port);

  url_download(w->page, pageLoadCallback);
}

/* The End */









