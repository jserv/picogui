/* $Id$
 *
 * picogui/stddialog.h - Various preconstructed dialog boxes the application
 *                       may use. These are implemented 100% client-side using
 *                       the normal C client API.
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
 * \defgroup stddialog Standard Dialogs
 *
 * Various prepackaged dialogs are supplied by the client library,
 * for things like choosing files, picking dates, or entering text.
 *
 * \{
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
int pgMessageDialog(const char *title,const char *text,u32 flags);

/*!
 * \brief Create a message dialog box, with formatting
 * 
 * This function is equivalent to pgMessage, with support for printf-style formatting
 * 
 * \sa pgMessageDialog
 */
int pgMessageDialogFmt(const char *title,u32 flags,const char *fmt, ...);

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

/*!
 * \brief Allow the user to edit a one-line string
 *
 * \param title   The title string displayed across the dialog's top
 * \param message If non-NULL, text to display above the field
 * \param deftxt  If nonzero, a string handle for default field text
 *
 * \returns A string handle with the value of the input field
 *          upon clicking "Ok" or zero if the dialog was cancelled.
 *          It is the app's responsibility to use pgDelete or contexts to
 *          delete this handle.
 *
 * \sa pgMessageDialog, pgMessageDialogFmt, pgDelete, pgEnterContext
 */
pghandle pgInputDialog(const char *title, const char *message,
		       pghandle deftxt);

/*! 
 * \brief Allow the user to select any font
 *
 * \param title   The title string displayed across the dialog's top
 * 
 * \returns A font handle describing the selected font
 *          upon clicking "Ok" or zero if the dialog was cancelled.
 *          It is the app's responsibility to use pgDelete or contexts to
 *          delete this handle.
 *
 * \sa pgNewFont, pgDialogBox
 */
pghandle pgFontPicker(const char *title);

/*!
 * \brief Select a file to load or save
 *
 * \param filefilter An optional function to filter the displayed files
 * \param pattern    This parameter is passed to the filter function
 * \param deffile    An optional default file name
 * \param flags      PG_FILE_* flags to control the dialog's operation
 * \param title   The title string displayed across the dialog's top
 *
 * \returns A fully qualified path name for the selected file. This string
 *          pointer is guaranteed to be valid until the next call to
 *          pgFilePicker()
 *
 * \sa pgfilter, PG_FILEOPEN, PG_FILESAVE
 */
const char *pgFilePicker(pgfilter filefilter, const char *pattern,
			 const char *deffile,int flags, const char *title);

/*!
 * \brief Display a dialog box allowing the user to select a color
 *
 * \param c Initially, the color to display. Returns the selected color
 * \param title   The title string displayed across the dialog's top
 * \returns Nonzero if the dialog was closed with the "Ok" button
 *
 * \sa pgDialogBox
 */
int pgColorPicker(pgcolor *c, const char *title);

//! \}

#endif /* __H_STDDIALOG */
/* The End */
