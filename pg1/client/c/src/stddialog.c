/* $Id$
 *
 * stddialog.c - Various preconstructed dialog boxes the application
 *               may use. These are implemented 100% client-side using
 *               the normal C client API.
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

#include "clientlib.h"

/* This is now pretty much obsolete, since there's the dialogbox widget to do all the work */
pghandle pgDialogBox(const char *title) {
  pghandle popupHandle;

  popupHandle = pgCreateWidget(PG_WIDGET_DIALOGBOX);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgNewString(title),
	      0);
  _pgdefault_rship = PG_DERIVE_INSIDE;

  /* return the newly created handle */
  return popupHandle;
}

/* Like pgMessageDialog, but uses printf-style formatting */
int pgMessageDialogFmt(const char *title,u32 flags,const char *fmt, ...) {
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
void dlgbtn(pghandle tb,u32 payload,int textproperty,
	    int iconproperty,int key) {

  pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,tb);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgGetServerRes(textproperty),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,key),
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 iconproperty),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  iconproperty+1),
	      0);
  pgSetPayload(PGDEFAULT,payload);
}
void dlgicon(pghandle at,int prop) {
  pghandle bit;

  bit = pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,prop);
  if (bit) {

  pgNewWidget(PG_WIDGET_LABEL,PG_DERIVE_AFTER,at);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_LEFT,
	      PG_WP_IMAGE,bit,
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,prop+1),
	      0);
  }
}

/* Create a message box, wait until it is
 * answered, then return the answer.
 */
int pgMessageDialog(const char *title,const char *text,u32 flags) {
  u32 ret;
  pghandle wToolbar;

  /* New context for us! */
  pgEnterContext();

  /* Default flags if none are supplied */
  if (!(flags & PG_MSGBTNMASK))
    flags |= PG_MSGBTN_OK;
  if (!(flags & PG_MSGICONMASK)) {
    /* Assume that if the user can say no, it's a question */
    
    if (flags & (PG_MSGBTN_CANCEL | PG_MSGBTN_NO))
      flags |= PG_MSGICON_QUESTION;
    else
      flags |= PG_MSGICON_MESSAGE;
  }

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
    dlgbtn(wToolbar,PG_MSGBTN_CANCEL,PGRES_STRING_CANCEL,
	   PGTH_P_ICON_CANCEL,PGTH_P_HOTKEY_CANCEL);
  if (flags & PG_MSGBTN_OK)
    dlgbtn(wToolbar,PG_MSGBTN_OK,PGRES_STRING_OK,
	   PGTH_P_ICON_OK,PGTH_P_HOTKEY_OK);
  if (flags & PG_MSGBTN_YES)
    dlgbtn(wToolbar,PG_MSGBTN_YES,PGRES_STRING_YES,
	   PGTH_P_ICON_YES,PGTH_P_HOTKEY_YES);
  if (flags & PG_MSGBTN_NO)
    dlgbtn(wToolbar,PG_MSGBTN_NO,PGRES_STRING_NO,
	   PGTH_P_ICON_NO,PGTH_P_HOTKEY_NO);

  /* Icons */
  if (flags & PG_MSGICON_ERROR)
    dlgicon(wToolbar,PGTH_P_ICON_ERROR);
  if (flags & PG_MSGICON_MESSAGE)
    dlgicon(wToolbar,PGTH_P_ICON_MESSAGE);
  if (flags & PG_MSGICON_QUESTION)
    dlgicon(wToolbar,PGTH_P_ICON_QUESTION);
  if (flags & PG_MSGICON_WARNING)
    dlgicon(wToolbar,PGTH_P_ICON_WARNING);

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
  pgNewPopupAt(PG_POPUP_ATEVENT,PG_POPUP_ATEVENT,0,0);
  
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
  pghandle returnHandle;
  returnHandle = pgNewPopupAt(PG_POPUP_ATEVENT,PG_POPUP_ATEVENT,0,0);

  for (i=0;i<numitems;i++) {
    printf("Menu Item => %d\n", pgNewWidget(PG_WIDGET_MENUITEM,0,0));
    pgSetWidget(PGDEFAULT,
		PG_WP_TEXT,items[i],
		0);
    pgSetPayload(PGDEFAULT,i+1);
  }

  /* Return event */
  return pgGetPayload(pgGetEvent()->from);
}

/* Like a messge dialog, with an input field */
pghandle pgInputDialog(const char *title, const char *message,
		       pghandle deftxt) {
  pghandle wToolbar,wField,wOk,wCancel,from;
  pghandle str = 0;

  /* New context for us! */
  pgEnterContext();

  /* Create the important widgets */
  pgDialogBox(title);
  wToolbar = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);
  wField = pgNewWidget(PG_WIDGET_TOOLBAR,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      0);
  pgNewWidget(PG_WIDGET_LABEL,0,0);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_ALL,
	      PG_WP_TEXT,pgNewString(message),
	      0);
  wField = pgNewWidget(PG_WIDGET_FIELD,PG_DERIVE_INSIDE,wField);
  pgSetWidget(PGDEFAULT,
	      PG_WP_SIDE,PG_S_BOTTOM,
	      PG_WP_SIDE,PG_S_ALL,
	      /* Only set the property if nonzero */
	      deftxt ? PG_WP_TEXT : 0,deftxt,
	      0);

  /* buttons */
  wCancel = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgGetServerRes(PGRES_STRING_CANCEL),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_HOTKEY_CANCEL),
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_CANCEL),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_CANCEL_MASK),
	      0);
  wOk = pgNewWidget(PG_WIDGET_BUTTON,PG_DERIVE_INSIDE,wToolbar);
  pgSetWidget(PGDEFAULT,
	      PG_WP_TEXT,pgGetServerRes(PGRES_STRING_OK),
	      PG_WP_SIDE,PG_S_RIGHT,
	      PG_WP_HOTKEY,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_HOTKEY_OK),
	      PG_WP_IMAGE,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					 PGTH_P_ICON_OK),
	      PG_WP_BITMASK,pgThemeLookup(PGTH_O_POPUP_MESSAGEDLG,
					  PGTH_P_ICON_OK_MASK),
	      0);

  /* Run */
  pgFocus(wField);
  for (;;) {
    from = pgGetEvent()->from;
    if (from == wOk) {
      /* Make a copy, because the one used in PG_WP_TEXT is deleted
       * automatically by the field widget on deletion. 
       */
      str = pgDup(pgGetWidget(wField,PG_WP_TEXT));
      /* Send it back up to the caller's context */
      pgChangeContext(str,-1);
      break;
    }
    else if (from == wCancel)
      break;
  }
  pgLeaveContext();
  return str;
}

/* The End */



