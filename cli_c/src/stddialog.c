/* $Id: stddialog.c,v 1.1 2001/07/28 10:42:12 micahjd Exp $
 *
 * stddialog.c - Various preconstructed dialog boxes the application
 *               may use. These are implemented 100% client-side using
 *               the normal C client API.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#include "clientlib.h"

/* Just a little helper to make it easy to do dialog boxes correctly */
pghandle pgDialogBox(const char *title) {
  pgNewPopup(0,0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString(title),
	      PG_WP_TRANSPARENT,0,
	      PG_WP_STATE,PGTH_O_LABEL_DLGTITLE,
	      0);
}

/* Like pgMessageDialog, but uses printf-style formatting */
int pgMessageDialogFmt(const char *title,unsigned long flags,const char *fmt, ...) {
  char *p;
  int ret;
  va_list ap;

  va_start(ap,fmt);
  if (!(p = _pg_dynformat(fmt,ap)))
    return;
  ret = pgMessageDialog(title,p,flags);
  free(p);
  va_end(ap);
  return ret;
}

/* Little internal helper function for the message dialog */
void dlgbtn(pghandle tb,unsigned long payload,int textproperty,int key) {

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,tb);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,textproperty),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,key),
	      0);
  pgSetPayload(PGDEFAULT,payload);
}

/* Create a message box, wait until it is
 * answered, then return the answer.
 */
int pgMessageDialog(const char *title,const char *text,unsigned long flags) {
  unsigned long ret;
  pghandle wToolbar;

  /* New context for us! */
  pgEnterContext();

  /* Default flags if none are supplied */
  if (!flags)
    flags = PG_MSGBTN_OK;

  /* Create the important widgets */
  pgDialogBox(title);
  wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TEXT,pgNewString(text),
	      0);

  /* Buttons */
  if (flags & PG_MSGBTN_CANCEL)
    dlgbtn(wToolbar,PG_MSGBTN_CANCEL,PGTH_P_STRING_CANCEL,PGTH_P_HOTKEY_CANCEL);
  if (flags & PG_MSGBTN_OK)
    dlgbtn(wToolbar,PG_MSGBTN_OK,PGTH_P_STRING_OK,PGTH_P_HOTKEY_OK);
  if (flags & PG_MSGBTN_NO)
    dlgbtn(wToolbar,PG_MSGBTN_NO,PGTH_P_STRING_NO,PGTH_P_HOTKEY_NO);
  if (flags & PG_MSGBTN_YES)
    dlgbtn(wToolbar,PG_MSGBTN_YES,PGTH_P_STRING_YES,PGTH_P_HOTKEY_YES);

  /* Run it (ignoring zero-payload events) */
  while (!(ret = pgGetPayload(pgGetEvent()->from)));

  /* Go away now */
  pgLeaveContext();

  return ret;
}

/* There are many ways to create a menu in PicoGUI
 * (at the lowest level, using pgNewPopupAt and the menuitem widget)
 *
 * This creates a static popup menu from a "|"-separated list of
 * menu items, and returns the number (starting with 1) of the chosen
 * item, or 0 for cancel.
 */
int pgMenuFromString(char *items) {
  char *p;
  pghandle str;
  int ret;
  int i;

  if (!items || !*items) return 0;

  /* Create the menu popup in its own context */
  pgEnterContext();
  pgNewPopupAt(PG_POPUP_ATCURSOR,PG_POPUP_ATCURSOR,0,0);
  
  i=0;
  do {
    /* Do a little fancy stuff to make the string handle.
     * This is like pgNewString but we get to specify the 
     * length instead of having strlen() do it for us.
     */
    if (!(p = strchr(items,'|'))) p = items + strlen(items);
    _pg_add_request(PGREQ_MKSTRING,(void *) items,p-items);
    items = p+1;
    pgFlushRequests();
    str = _pg_return.e.retdata;

    /* Create each menu item */
    pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT,str,
		0);
    pgSetPayload(PGDEFAULT,++i);

  } while (*p);

  /* Run the menu */
  ret = pgGetPayload(pgGetEvent()->from);
  pgLeaveContext();
  return ret;
}

/* This creates a menu from an array of string handles. 
 * Same return values as pgMenuFromString above.
 *
 * Important note: pgMenuFromArray expects that a new
 *                 context will be entered before the
 *                 string handles are created
 */
int pgMenuFromArray(pghandle *items,int numitems) {
  int i;

  pgNewPopupAt(PG_POPUP_ATCURSOR,PG_POPUP_ATCURSOR,0,0);

  for (i=0;i<numitems;i++) {
    pgNewWidget(PG_WIDGET_MENUITEM,0,0);
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT,items[i],
		0);
    pgSetPayload(PGDEFAULT,i+1);
  }

  /* Return event */
  return pgGetPayload(pgGetEvent()->from);
}

/* The End */



