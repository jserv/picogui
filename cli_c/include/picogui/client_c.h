/* $Id: client_c.h,v 1.44 2001/05/15 04:27:02 micahjd Exp $
 *
 * picogui/client_c.h - The PicoGUI API provided by the C client lib
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
 *  Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 */

#ifndef _H_PG_CLI_C
#define _H_PG_CLI_C

#include <stdio.h>   /* For NULL and FILE */

/*! 
 * \file client_c.h
 * \brief C Client API Header
 * 
 * client_c.h contains declarations for all core PicoGUI API functions
 * and structures. This does not include the PGFX graphics module, or the
 * low-level canvas commands. All constants common to client and server are
 * in constants.h, and the network interface between client and server is
 * defined in network.h. Usually this file does not need to be included
 * separately, it is included with <tt>\#include <picogui.h></tt>
 */

/******************** Client-specific constants and data types */

/*!
 * \brief Generic PicoGUI event structure
 * 
 * The pgEvent structure can describe any PicoGUI event. A pointer to
 * a pgEvent structure is the standard way of representing an event in
 * PicoGUI.
 * 
 * The type, from, and extra members are valid in any event. The union, e,
 * contains possible formats the event's parameters may take. Only one is
 * valid, and this depends on the type of event. For example, if
 * <tt>event->type == PG_WE_PNTR_DOWN</tt>, 
 * \p event->e.pntr is valid and the mouse coordinates can be found in 
 * \p event->e.pntr.x and \p event->e.pntr.y .
 * 
 * \sa pgBind, pgGetEvent
 */
struct pgEvent {
   short type;      //!< Event type, a PG_WE_* or PG_NWE_* constant
   pghandle from;   //!< The widget the event was recieved from (if applicable)
   void *extra;     //!< Extra data passed to the event handler via pgBind
   
   //! Event-specific parameters
   union {
      
      //! The generic parameter. Currently unused.
      unsigned long param;
      
      //! Width and height, for PG_WE_BUILD and PG_WE_RESIZE
      struct {
	 short w;
	 short h;
      } size;

      //! Modifiers and key, for keyboard events
      struct {
	 short mods;  //!< PGMOD_* constants logically or'ed together
	 /*! 
	  * For PG_WE_KBD_CHAR, an ASCII/Unicode character. For 
	  * PG_WE_KBD_KEYUP and PG_WE_KBD_KEYDOWN, it is a PGKEY_* constant
	  */
	 short key;
      } kbd;
      
      //! Pointing device information, for PG_WE_PNTR_* and PG_NWE_PNTR_* events
      struct {
	 short x,y;
	 short btn;    //!< Bitmask of pressed buttons, left button is bit 0
	 short chbtn;  //!< Bitmask of buttons changed since last event
      } pntr;
      
      //! Streamed data, from the PG_WE_DATA event
      struct {
	 unsigned long size;
	 /*! Allocated and freed by the client library. It is only valid
	  * until the event handler returns or the client calls pgGetEvent */
	 char *pointer; 
      } data;
      
   } e;
};

//! A wildcard value for pgBind()
#define PGBIND_ANY      -1

//! A wildcard value for pgNewFont
#define PGFONT_ANY      0

/*! 
 * \brief Refer to the default widget handle
 * 
 * A more verbose way of using the default widget or rship (widget
 * relationship) in PicoGUI function calls. Just using 0 is
 * perfectly acceptable, but this can make your code easier to
 * read. 
 *
 * PGDEFAULT can be used any time PicoGUI expects a widget handle, as sort
 * of a pronoun referring to most recently created widget. 
 * (the "default widget") This includes the
 * result of pgNewWidget calls, pgRegisterApp, and pgNewPopup. When PGDEFAULT
 * (or zero) is used in place of the parent and widget relationship in
 * pgNewWidget the new widget is placed after the default widget. The only
 * exception to this is when the default widget is a root widget (created with
 * pgRegisterApp or pgNewPopup) in which case the new widget is placed inside
 * the default widget.
 * 
 * \sa pgNewWidget, pgRegisterApp, pgNewPopup, pgSetWidget
 */
#define PGDEFAULT       0

//! For forming fractions, such as the size when PG_SZMODE_CNTFRACT is used
#define pgFraction(n,d) (((n)<<8)|(d))

/*!
 * \brief The event handler used in pgBind
 * 
 * \param evt The event that triggered this handler
 * \returns Zero to continue on with other handlers, nonzero to abort further event processing
 */
typedef int (*pgevthandler)(struct pgEvent *evt);
//! The event handler for pgSetIdle
typedef void (*pgidlehandler)(void);
#ifdef FD_SET
//! The event hander for pgCustomizeSelect
typedef int (*pgselecthandler)(int n, fd_set *readfds, fd_set *writefds,
			       fd_set *exceptfds, struct timeval *timeout);
#endif

/*!
 * \brief A structure representing data, loaded or mapped into memory
 * 
 * This is returned by the pgFrom* series of functions for loading
 * data. \internal
 */
struct pgmemdata {
  void *pointer;       // when null, indicates error
  unsigned long size;
  int flags;           // PGMEMDAT_* flags or'ed together
};
#define PGMEMDAT_NEED_FREE    0x0001   // Should be free()'d when done
#define PGMEMDAT_NEED_UNMAP   0x0002   // Should be munmap()'d when done

/******************** Administration */

/*!
 * \brief Initialize PicoGUI
 * 
 * See if there are any command line args relevant to PicoGUI
 * (such as for setting the PicoGUI server) and establish
 * a connection to the server. This must be the first PicoGUI
 * call in the client, and it should almost certainly be called before
 * and command line processing.
 * 
 * pgInit processes command line arguments beginning with "--pg" and removes
 * them from the argument list and terminates argv[] with a NULL. This is
 * compatible with optarg and probably other argument-processing systems.
 * Currently the following arguments are handled:
 * 
 *  - --pgserver <server>\n
 *    Connects to the PicoGUI server specified in <server>
 *  - --version \n
 *    Prints the client library version
 * 
 * Unrecognized commands beginning with "--pg" display a list of available
 * commands. If it is unable to contact the server, a client error is
 * triggered.
 *
 * The client does not need to explicitly disconnect from the PicoGUI server
 * 
 * \sa pgRegisterApp, pgSetErrorHandler
 */
void pgInit(int argc, char **argv);

/*!
 * \brief Replace the default error handler
 * 
 * \param handler A pointer to the new handler function
 * \param errortype The general type of error, a PG_ERRT_* constant
 * \param msg A message string with more information
 * 
 * Errors can be triggered by the client (in the case of an IO error or fatal signal) 
 * or by the server. (A bug somewhere, out of memory, etc.)
 * 
 * The default error handler displays a message dialog allowing the user to optionally
 * terminate the program. If it is unable to display the message dialog, the error is
 * printed to stderr and the program is terminated.
 *
 * \sa pgInit, pgErrortypeString
 */
void pgSetErrorHandler(void (*handler)(unsigned short errortype,
				       const char *msg));

/*! 
 * \brief Convert a numerical errortype to a string
 * 
 * \param errortype A PG_ERRT_* error type constant
 * 
 * \returns A pointer to the corresponding string constant
 */
const char *pgErrortypeString(unsigned short errortype);

/*!
 * \brief Set a handler to be called periodically
 * 
 * \param t Maximum number of milliseconds to wait between calls to handler
 * \param handler Pointer to a handler function, or NULL to disable 
 * 
 * \returns Pointer to the previous handler function
 * 
 * This is based on the pgSetnonblocking code added by Philippe, but
 * this fits the event-driven model better, and most importantly it
 * handles stopping and starting the event loop automatically.
 * Note that it is still possible for PicoGUI to block if the server
 * sends a partial reply packet, but even if the idle handler were called
 * during this time the network connection would be 'jammed' so there wouldn't
 * be much point.
 * 
 * \sa pgEventLoop
 */
pgidlehandler pgSetIdle(long t,pgidlehandler handler); 

/*!
 * \brief Flush all unsent request packets to the server
 * 
 * Usually this is handled automatically, but
 * it is needed in some situations. For example, a remote input driver
 * that has no real event loop, but needs to send keyboard or mouse events.
 * The events would not actually be sent to the server until pgFlushRequests
 * is called.
 * 
 * \sa pgUpdate
 */
void pgFlushRequests(void);

/*!
 * \brief Update the screen 
 *
 * Redraw portions of the screen if necessary. This forces all unsent
 * packets to be flushed to the server, and instructs the server to
 * draw changed areas of the screen.
 * 
 * If your application is pgEventLoop (or pgGetEvent) based,
 * this is handled automatically. The server always updates the screen
 * before waiting for user interaction.
 *
 * For doing animation, consider using pgSubUpdate instead.
 * 
 * \sa pgFlushRequests, pgSubUpdate
 */
void pgUpdate(void);

/*!
 * \briefUpdate a subsection of the screen
 *
 * The given widget and all other
 * widgets contained within it are redrawn if necessary.
 * The request buffer is flushed and the section is redrawn
 * independantly and immediately.
 *
 * This function is recommended for animation.
 * 
 * \sa pgUpdate, pgFlushRequests
 */
void pgSubUpdate(pghandle widget);

/*!
 *
 * \brief Attatch an event handler to a widget and/or event.
 * 
 * \param widgetkey A widget to attach to, or PGBIND_ANY to respond to any widget's events
 * \param eventkey An event to attach to, or PGBIND_ANY to respond to any event by the selected widget(s).
 * \param handler A pointer to a PicoGUI event handler
 * \param extra The value to pass within the pgEvent's \p extra field
 * 
 * When the widgetkey and eventkey both match, the handler is called,
 * and the specified value for extra is passed in its pgEvent structure. The extra value passed
 * depends on the binding that triggered the call, not on the widget or event involved.
 * If widgetkey and eventkey are exactly the same as an existing binding, its
 * handler and extra value are reset to the ones specified here. If handler is
 * NULL the binding is deleted.
 * 
 * \sa pgevthandler, pgEvent
 */
void pgBind(pghandle widgetkey,unsigned short eventkey,
	    pgevthandler handler,void *extra);

/*!
 *
 * \brief Wait on your own file descriptors
 *
 * \param handler If non-NULL, this is used instead of select() in the client library
 *
 * This function allows you to specify a handler that acts as
 * a wrapper around the client library's calls to select(), so 
 * you can wait on your own file descriptors. 
 * 
 * To cancel this, call with a NULL handler and the select handle will be set
 * back to the standard select() system call.
 *
 * To use this function, the proper header files must be included
 * before picogui.h for select()
 * 
 * \sa pgSetIdle, pgEventLoop
 */
#ifdef FD_SET
void pgCustomizeSelect(pgselecthandler handler);
#endif

/*!
 * \brief Register exclusive access to a resouce
 *
 * \param resource A PG_OWN_* constant indicating the resource you request
 * 
 * If the resource is already in use or cannot be obtained,
 * a client error is triggered
 *
 * \sa PG_OWN_KEYBOARD, PG_OWN_POINTER, PG_OWN_SYSEVENTS
 */
void pgRegisterOwner(int resource);

/*!
 * \brief Unregister exclusive access to a resouce
 *
 * \param resource A PG_OWN_* constant indicating the resource you release
 * 
 * An error will be triggered if the client does not already own the specified
 * resource.
 * 
 * \sa pgRegisterOwner
 */
void pgUnregisterOwner(int resource);

/*!
 * \brief Simulate keyboard input remotely
 * 
 * \param type A PG_TRIGGER_* constant (see below)
 * \param key Either a PGKEY_* constant or an ASCII/Unicode value (see below)
 * \param mods Always a set of zero or more PGMOD_* constants or'ed together
 * 
 * This function can be used by networked input drivers or otherwise to simulate
 * keyboard input. To effectively do so, the client needs to send three types
 * of triggers:
 * 
 *  - PG_TRIGGER_KEYDOWN when the key is pressed, setting \p key to the PGKEY_* constant
 *    for the key
 *  - PG_TRIGGER_KEYUP when the key is released, also using a PGKEY_* constant
 *  - PG_TRIGGER_CHAR when the key is pressed, only if it is translatable to an Ascii/Unicode value.
 *    \p key should be set to this translated value taking all modifiers into account. This trigger
 *    may be repeated to implement keyboard autorepeat.
 * 
 * \sa pgSendPointerInput
 */
void pgSendKeyInput(unsigned long type,unsigned short key,
		    unsigned short mods);

/*!
 * \brief Simulate pointing device input remotely
 * 
 * \param type A PG_TRIGGER_* constant (see below)
 * \param x Horizontal coordinate of the pointing device, in the physical coordinate system. Not affected by rotation.
 * \param y Vertical coordinate
 * \param btn Bitmask of currently pressed mouse buttons. The left button is the least significant bit.
 *
 * This function can be used by networked input drivers or otherwise to simulate pointing device input.
 * The following are legal trigger types:
 * 
 *  - PG_TRIGGER_UP: Mouse button or stylus up
 *  - PG_TRIGGER_DOWN: Mouse button or stylus down
 *  - PG_TRIGGER_MOVE: Mouse movement or mouse/stylus dragging
 * 
 * \sa pgSendKeyInput
 */
void pgSendPointerInput(unsigned long type,unsigned short x,unsigned short y,
			unsigned short btn);

/*!
 * \brief Change video mode at runtime
 * 
 * \param xres New horizontal resolution
 * \param yres New vertical resolution
 * \param bpp Color depth in bits per pixel
 * \param flagmode PG_FM_* constant specifying how to combine \p flags with the current video flags
 *
 * \p xres, \p yres, and \p bpp can be zero to keep the current values. 
 * 
 * \p flagmode can have the following values:
 *  - PG_FM_SET: Set all video flags to the specified value
 *  - PG_FM_ON: Turns on specified flags, leaves others untouched
 *  - PG_FM_OFF: Turns off specified flags
 *  - PG_FM_TOGGLE: Toggles specified flags
 * 
 * \p flags specifies extra optional features that may be present in the video driver.
 * Unsupported flags are ignored.
 * It can be zero or more of the following values or'ed together:
 *  - PG_VID_FULLSCREEN: Uses a fullscreen mode if available
 *  - PG_VID_DOUBLEBUFFER: Uses double buffering if available
 *  - PG_VID_ROTATE90, PG_VID_ROTATE180, PG_VID_ROTATE270: Rotate the screen by the indicated number
 *    of degrees anticlockwise. All rotation flags are mutually exclusive.
 * 
 * \sa pgGetVideoMode
 */
void pgSetVideoMode(unsigned short xres,unsigned short yres,
		    unsigned short bpp,unsigned short flagmode,
		    unsigned long flags);

/*!
 * \brief Get information about the current video mode
 *
 * \returns A pgmodeinfo structure with information about the current video mode.
 *
 * The returned pointer is good only until the next PicoGUI call. It is recommended to use
 * something like the following:
 * 
 * \code
struct pgmodeinfo mi;
mi = *pgGetVideoMode();
 * \endcode
 *
 * \sa pgSetVideoMode, pgmodeinfo
 */
struct pgmodeinfo *pgGetVideoMode(void);

/******************** Objects */

/*!
 * \brief Delete any object that has a handle 
 *
 * \param object A handle to any type of object (String, widget, bitmap, etc.)
 * 
 * This function frees the memory in the PicoGUI server associated with \p object.
 */
void pgDelete(pghandle object);

//!  Give a widget the keyboard focus 
void pgFocus(pghandle widget);

/*! 
 * \brief Register a new application
 * 
 * \param type A PG_APP_* constant like PG_APP_NORMAL or PG_APP_TOOLBAR
 * \param name The application's name, displayed in it's panelbar if applicable
 * 
 * \returns A handle to the application's root widget
 * 
 * Optional specifications (PG_APPSPEC_*) are specified 
 * in name-value pairs, terminated with a 0.
 * Currently most PG_APPSPEC_* constants are unimplemented, but the application's
 * initial size and position can be set by using pgSetWidget on the application's
 * root widget.
 *
 * Example:
 * \code
pgRegisterApp(PG_APP_NORMAL,"My App",
              PG_APPSPEC_SIDE,PG_S_TOP,
              PG_APPSPEC_MINHEIGHT,50,
              0);
pgSetWidget(PGDEFAULT,
            PG_WP_SIZE,50,
            PG_WP_SIZEMODE,PG_SZMODE_PERCENT,
            0);
 * \endcode
 * 
 * \sa pgNewWidget, pgSetWidget, pgNewPopup
 */
pghandle pgRegisterApp(short int type,const char *name, ...);

/*!
 * \brief Create a new widget, derived from a parent widget
 * 
 * \param type A PG_WIDGET_* constant for the widget type
 * \param rship A PG_DERIVE_* constant indicating the new widget's relationship to it's parent. It can be PGDEFAULT.
 * \param parant The parent widget's handle, or PGDEFAULT.
 * 
 * \returns A handlet to the new widget
 *
 * \p rship indicates where in the widget stacking order, relative to the parent, the new widget will be:
 *  - PG_DERIVE_INSIDE: For container widgets, put the new widget inside the parent but before other widgets that may already be inside it.
 *  - PG_DERIVE_BEFORE: Before the parent widget in the stacking order
 *  - PG_DERIVE_AFTER: After the parent widget in the stacking order
 *
 * \sa pgSetWidget
 */
pghandle pgNewWidget(short int type,short int rship,
		     pghandle parent);

/*!
 * \brief Create a popup box, centered on the screen
 * \returns A handle to the popup box root widget
 * \sa pgNewPopupAt
 */
pghandle pgNewPopup(int width,int height);

/*!
 * \brief Create a popup box at the specified position
 * \returns A handle to the popup box root widget
 *
 * \p x and/or \p y can be a PG_POPUP_* constant:
 *   - PG_POPUP_CENTER: Centered on the screen, same behavior as pgNewPopup
 *   - PG_POPUP_ATCURSOR: At the pointing device's cursor. If the cursor is over a button or menuitem, the popup snaps to its edge automatically
 * 
 * \sa pgNewPopup
 */
pghandle pgNewPopupAt(int x,int y,int width,int height);

/*!
 * \brief Set widget properties
 * 
 * \param widget Widget handle, may be PGDEFAULT
 * 
 * After \p widget, pgSetWidget accepts a list of property-value pairs terminated by a zero.
 * For example:
 * 
 * \code
pgSetWidget(wLabel,
            PG_WP_TEXT,pgNewString("Hello"),
            PG_WP_FONT,pgNewFont("Helvetica",12,0),
            0);
 * \endcode
 * 
 * \sa pgNewWidget, pgGetWidget, pgNewString, pgNewFont
 */
void pgSetWidget(pghandle widget, ...);

/*!
 * \brief Get a widget property
 * 
 * \param widget Widget handle
 * \param property A widget property (PG_WP_* constant)
 * 
 * \returns The value associated with the specified property
 *
 * \sa pgSetWidget, pgNewWidget
 */
long pgGetWidget(pghandle widget,short property);

/*!
 * \brief Create a new bitmap object
 * 
 * \param A pgmemdata structure, as returned by a pgFrom* function
 * \returns A handle to the new bitmap object created in the PicoGUI server
 * 
 * \sa pgFromMemory, pgFromFile, pgFromStream, pgFromTempMemory, pgDelete, pgEnterContext, pgLeaveContext
 */
pghandle pgNewBitmap(struct pgmemdata obj);

/*!
 * \brief Create a new string object 
 *
 * \param str The string make an object with
 * \returns A handle to the new string object created in the PicoGUI server
 *
 * \sa pgFromMemory, pgFromFile, pgFromStream, pgFromTempMemory, pgDelete, pgEnterContext, pgLeaveContext
 */
pghandle pgNewString(const char *str);

/*!
 * \brief Evaluate a PicoGUI request packet
 *
 * \param reqtype A PGREQ_* constant indicating the packet type
 * \param data Pointer to the raw packet data
 * \param datasize Length of raw packet data
 * 
 * \returns Returns the request packet's return value, if any. If the request packet does not return a simple data type, the value is undefined.
 * 
 * This is a good way
 * to reuse PicoGUI's serialization capabilities to load
 * a generic binary object from file. It is advisable to
 * validate the request's type first so you don't allow
 * the input to do wierd things like change video mode
 * or leave the current context. 
 *
 * The format of the data accepted by the request packet depends on the type of packet.
 */
pghandle pgEvalRequest(short reqtype, void *data, unsigned long datasize);

/*!
 * \brief Get the contents of a string handle
 *
 * \param string Must be a handle to a string object
 * \returns A pointer to the string object's contents
 * 
 * The returned string pointer must be treated as read-only. It is only
 * valid until the next PicoGUI function call.
 * 
 * \sa pgNewString
 */
char *pgGetString(pghandle string);

/*! 
 * \brief Change a widget's text
 * 
 * \param widget A pointer to a widget with the PG_WP_TEXT property
 * \param str The string to set PG_WP_TEXT to
 *
 * This function performs the following steps:
 *  - Calls pgGetWidget to find the old text handle
 *  - Uses pgNewString to get a handle to the new text
 *  - Uses pgSetWidget to send the new handle to the widget
 *  - If the old handle was non-NULL, deletes it with pgDelete
 * 
 * It is the preferred way of setting or
 * changing the text of a button, label, or other
 * widget that takes a PG_WP_TEXT property.
 * 
 * \sa pgGetWidget, pgNewString, pgSetWidget, pgDelete
 */
void pgReplaceText(pghandle widget,const char *str);

/*!
 * \brief Change a widget's text, with formatting
 *
 * This function is equivalent to pgReplaceText, with support for printf-style formatting
 *
 * \sa pgReplaceText 
 */
void pgReplaceTextFmt(pghandle widget,const char *fmt, ...);

/* Create a new font object based on the given
 * parameters. Any of them can be 0 (or PGFONT_ANY)
 * to ignore that parameter.
 */
pghandle pgNewFont(const char *name,short size,unsigned long style);

/* In *w and *h, returns the size in pixels of the given text
 * in the given font. 
 *
 * font may be 0 to use the default font
 */
void pgSizeText(int *w,int *h,pghandle font,pghandle text);

/* Load a compiled theme file into the server */
pghandle pgLoadTheme(struct pgmemdata obj);

/* Get and set the 'payload', a app-defined chunk
 * of data attatched to any object. Good for defining
 * button return codes in a dialog, or even making
 * a linked list of objects!
 */
void pgSetPayload(pghandle object,unsigned long payload);
unsigned long pgGetPayload(pghandle object);

/* Write data to a widget.
 * (for example, a terminal widget)
 */
void pgWriteData(pghandle widget,struct pgmemdata data);

/* Wrapper around pgWriteData to send a command, for example
 * to a canvas widget. Widget, command, and param number must be followed
 * by the specified number of commands
 */
void pgWriteCmd(pghandle widget,short command,short numparams, ...);

/******************** Data loading */

/* Data already loaded in memory */
struct pgmemdata pgFromMemory(void *data,unsigned long length);

/* Data already loaded in memory, client lib will free it when done */
struct pgmemdata pgFromTempMemory(void *data,unsigned long length);

/* Load from a normal disk file */
struct pgmemdata pgFromFile(const char *file);

/* Load from an already-opened stream */
struct pgmemdata pgFromStream(FILE *f, unsigned long length);

/* TODO: Load from resource. Allow apps to package necessary bitmaps
   and things in a file, named after their binary but with a '.res'
   extension.
   The server will also be able to request reloading data from these
   resource files, for example to reload bitmaps when the bit depth
   changes.

   This is just an idea, and I'll implement it later...
   This whole pgmemdata business is just my attempt to leave enough
   hooks to make this work.
*/

/******************** Program flow */

/* The app's main event-processing loop.  This can be called
 * more than once throughout the life of the program, but
 * it shouldn't be called while it's already running.
 * 
 * If the app recieves an event while it is not waiting in an
 * EventLoop, the server will queue them until the client is
 * ready.
 */
void pgEventLoop(void);
void pgExitEventLoop(void);

/* Wait for a single event, then return it. This is good
 * for small dialog boxes, or other situations when pgBind and
 * pgEventLoop are overkill.
 */
struct pgEvent *pgGetEvent(void);

/* PicoGUI uses a context system, similar to contexts in C.
 * Whenever the program leaves a context, all objects created
 * while in that context are deleted. Contexts can be nested
 * almost infinitely (Well, 32768 or so... ;-)
 */
void pgEnterContext(void);
void pgLeaveContext(void);

/* Create a message dialog box, wait until it is
 * answered, then return the answer.
 */
int pgMessageDialog(const char *title,const char *text,unsigned long flags);

/* Like pgMessageDialog, but uses printf-style formatting */
int pgMessageDialogFmt(const char *title,unsigned long flags,const char *fmt, ...);

/* There are many ways to create a menu in PicoGUI
 * (at the lowest level, using pgNewPopupAt and the menuitem widget)
 *
 * This creates a static popup menu from a "|"-separated list of
 * menu items, and returns the number (starting with 1) of the chosen
 * item, or 0 for cancel.
 */
int pgMenuFromString(char *items);

/* This creates a menu from an array of string handles. 
 * Same return values as pgMenuFromString above.
 *
 * Important note: pgMenuFromArray does not perform context
 *                 management automatically like pgMenuFromString.
 *                 Before you start allocating strings,
 *                 call pgEnterContext, and after pgMenuFromArray
 *                 call pgLeaveContext.
 */
int pgMenuFromArray(pghandle *items,int numitems); 

#endif /* __H_PG_CLI_C */
/* The End */
