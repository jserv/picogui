/* $Id: stddialog.h,v 1.3 2001/07/30 07:15:02 micahjd Exp $
 *
 * picogui/stddialog.h - Various preconstructed dialog boxes the application
 *                       may use. These are implemented 100% client-side using
 *                       the normal C client API.
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
 */

#ifndef _H_PG_STDDIALOG
#define _H_PG_STDDIALOG

/*! 
 * \file stddialog.h
 * \brief Standard Dialog Boxes
 * 
 * This file defines various standard dialog boxes available in the 
 * C client library.
 * Usually this file does not need to be included
 * separately, it is included with <tt>\#include <picogui.h></tt>
 */

/*!
 * \brief Create a dialog box with title
 *
 * This helpful function creates a centered, automatically sized dialog box
 * equivalent to "pgNewPopup(PGDEFAULT,PGDEFAULT)" and creates a title widget
 * with the given text.
 *
 * \returns A handle to the dialog box title widget
 *
 * \sa pgNewPopup, pgNewPopupAt
 */
pghandle pgDialogBox(const char *title);

/*!
 * \brief Create a message dialog box
 *
 * \param title The title string displayed across the dialog's top
 * \param text Text to display within the dialog box
 * \param flags One or more PG_MSGBTN_* constants or'ed together, or zero for the default
 * \returns The PG_MSGBTN_* constant indicating which button was activated
 * 
 * This creates a modal dialog box in the center of the screen, displaying the
 * given message, and waits for an answer. If \p flags is zero, a simple dialog
 * with only an "Ok" button is created. Possible PG_MSGBTN_* flags include:
 *  - PG_MSGBTN_OK
 *  - PG_MSGBTN_CANCEL
 *  - PG_MSGBTN_YES
 *  - PG_MSGBTN_NO
 * 
 * \sa pgMessageDialogFmt
 */
int pgMessageDialog(const char *title,const char *text,unsigned long flags);

/*!
 * \brief Create a message dialog box, with formatting
 * 
 * This function is equivalent to pgMessage, with support for printf-style formatting
 * 
 * \sa pgMessageDialog
 */
int pgMessageDialogFmt(const char *title,unsigned long flags,const char *fmt, ...);

/*! 
 * \brief Create a popup menu from a string
 * 
 * \param items A list of menu items, separated by pipe characters. The menu items may contain newlines.
 * \returns The number (starting with 1) of the chosen item, or zero for a click outside the menu
 * 
 * This function is a high-level way to create simple menus, equivalent to calling pgNewPopupAt to create
 * a popup menu and filling it with menuitem widgets.
 * 
 * \sa pgMenuFromArray, pgNewPopup, pgNewPopupAt
 */
int pgMenuFromString(char *items);

/*!
 * \brief Create a popup menu from string handles
 * 
 * \param items An array of handles to string objects for each menu item
 * \param numitems The number of string handles
 * \returns The number (starting with 1) of the chosen item, or zero for a click outside the menu
 * 
 * Unlike pgMenuFromString, this function does not perform memory
 * management automatically. Before the client begins creating string
 * objects for the \p items array, call pgEnterContext.
 * After pgMenuFromArray returns, call pgLeaveContext to free the string
 * handles and destroy the popup menu. Note that the popup menu will not
 * disappear until the call to pgLeaveContext. This means it is possible to delay
 * calling pgLeaveContext to display other popups on top of the menu, for
 * example to create a tree of popup menus.
 *
 * \sa pgMenuFromString, pgNewPopup, pgNewPopupAt
 */
int pgMenuFromArray(pghandle *items,int numitems); 

/*!
 * \brief Show a date on a calendar, allow the user to select a new date
 *
 * \param year Initially, the year to display. Returns the selected year
 * \param month Initially, the month to display. Returns the selected month
 * \param day Initially, the day to display. Returns the selected day
 * \param title Title to display in the dialog box
 * \returns Nonzero if a new date was selected, zero if the
 *          cancel button was pressed
 *
 * \p year, \p month, and/or \p day may be zero to use the current date.
 * The year must be fully specified. For example, use 1984 instead of just 84.
 * Of course the dialog is Y2K compliant!  On systems with 32-bit time_t, it
 * does not allow years beyond 2037. With a 64-bit time_t this isn't a problem.
 * Months and days are both one-based.
 *
 * \sa pgDialogBox
 */
int pgDatePicker(int *year, int *month, int *day, const char *title);

#endif /* __H_STDDIALOG */
/* The End */
