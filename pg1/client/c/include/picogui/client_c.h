/* $Id$
 *
 * picogui/client_c.h - The PicoGUI API provided by the C client lib
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
 *  Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 */

#ifndef _H_PG_CLI_C
#define _H_PG_CLI_C

#include <stdio.h>   /* For NULL and FILE */
#ifdef __NetBSD__  
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif

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

/*!
 * \defgroup pgapi Core PicoGUI API
 *
 * These are the basic PicoGUI APIs used for connecting to the server
 * and manipulating data within it. This does not include the PGFX and
 * Standard Dialog modules.
 *
 * \{
 */

/******************** Client-specific constants and data types */

/*!
 * \defgroup constdata Constants and Data Types
 *
 * pgEvent and other data structures and constants specific to the client.
 * These values are interpreted in the client library, not the server.
 *
 * \{
 */

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
   s16 type;      //!< Event type, a PG_WE_* or PG_NWE_* constant
   pghandle from;   //!< The widget the event was recieved from (if applicable)
   void *extra;     //!< Extra data passed to the event handler via pgBind
   
   //! Event-specific parameters
   union {
      
      //! The generic parameter. Currently unused.
      u32 param;
      
      //! Width and height, for PG_WE_BUILD and PG_WE_RESIZE
      struct {
	 s16 w;
	 s16 h;
      } size;

      //! Modifiers and key, for keyboard events
      struct {
	 s16 mods;  //!< PGMOD_* constants logically or'ed together
	 /*! 
	  * For PG_WE_KBD_CHAR, an ASCII/Unicode character. For 
	  * PG_WE_KBD_KEYUP and PG_WE_KBD_KEYDOWN, it is a PGKEY_* constant
	  */
	 s16 key;
      } kbd;
      
      //! Pointing device information, for PG_WE_PNTR_* and PG_NWE_PNTR_* events
      struct {
	 s16 x,y;
	 s16 btn;    //!< Bitmask of pressed buttons, left button is bit 0
	 s16 chbtn;  //!< Bitmask of buttons changed since last event
      } pntr;
      
      //! Streamed data, from the PG_WE_DATA event
      struct {
	 u32 size;
	 /*! Allocated and freed by the client library. It is only valid
	  * until the event handler returns or the client calls pgGetEvent */
	 char *pointer; 

	 //! Client-side representation of a pgserver trigger, for client-side input filters
	 union pg_client_trigger *trigger;
      } data;
      
   } e;
};

//! A wildcard value for pgBind()
#define PGBIND_ANY      -1

//! A wildcard value for pgNewFont
#define PGFONT_ANY      0

/*! 
 * \brief RGB hardware-independant color
 * 
 * The format is 24-bit RGB, similar to that used by HTML.
 * The following are some example colors:
 * \code
#define BLACK   0x000000
#define WHITE   0xFFFFFF
#define GREY    0x808080
#define RED     0xFF0000
#define GREEN   0x00FF00
#define BLUE    0x0000FF
#define YELLOW  0xFFFF00
 * \endcode
 * 
 * Video drivers may define other formats that are selected by
 * setting a bit in the color's high byte. For example, in text-mode
 * drivers, a high byte set to 0x20 would indicate a raw character code.
 * Using the  driver, the expression (0x20000F00 | 'A') would be
 * a capital "A" with a white foreground and black background.
 * These formats are video-driver dependant, and under normal circumstances
 * the high byte should always be zero.
 */
typedef u32 pgcolor;

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
typedef void (*pgselectbh)(int result, fd_set *readfds, fd_set *writefds,
			   fd_set *exceptfds);
#endif
//! Filter function for pgFilePicker()
typedef int (*pgfilter)(const char *string,const char *pattern);

#define PG_FILE_SAVEBTN    (1<<0)  //!< Use a 'save' button instead of 'open'
#define PG_FILE_MUSTEXIST  (1<<1)  //!< The chosen file must already exist
#define PG_FILE_MUSTWRITE  (1<<2)  //!< The file must be writeable
#define PG_FILE_MUSTREAD   (1<<3)  //!< The file must be readable
#define PG_FILE_SHOWDOT    (1<<4)  //!< Show . and .. directories
#define PG_FILE_SHOWHIDDEN (1<<5)  //!< Show hidden files
#define PG_FILE_SHOWBAK    (1<<6)  //!< Show editor backups
#define PG_FILE_SHOWDEV    (1<<7)  //!< Show device nodes (dangerous)
#define PG_FILE_FIELD      (1<<8)  //!< The user can enter filenames in a field

//! Default flags for a file open dialog box
#define PG_FILEOPEN     PG_FILE_MUSTREAD
//! Default flags for a file save dialog box
#define PG_FILESAVE    (PG_FILE_SAVEBTN | PG_FILE_FIELD)

/* Constants for the message dialog box flags */
#define PG_MSGBTN_OK        0x0001
#define PG_MSGBTN_CANCEL    0x0002
#define PG_MSGBTN_YES       0x0004
#define PG_MSGBTN_NO        0x0008
#define PG_MSGICON_ERROR    0x0010
#define PG_MSGICON_MESSAGE  0x0020
#define PG_MSGICON_QUESTION 0x0040
#define PG_MSGICON_WARNING  0x0080

#define PG_MSGBTNMASK       (PG_MSGBTN_OK|PG_MSGBTN_CANCEL|\
                             PG_MSGBTN_YES|PG_MSGBTN_NO)
#define PG_MSGICONMASK      (PG_MSGICON_ERROR|PG_MSGICON_MESSAGE|\
                             PG_MSGICON_QUESTION|PG_MSGICON_WARNING)

/*!
 * \brief A structure representing data, loaded or mapped into memory
 * 
 * This is returned by the pgFrom* series of functions for loading
 * data, and used by many PicoGUI functions that need to refer to a chunk of data.
 * 
 * \internal
 * 
 * \sa pgFromFile, pgFromStream, pgFromMemory, pgFromTempMemory, pgNewBitmap, pgLoadTheme
 */
struct pgmemdata {
  void *pointer;       //!< when null, indicates error
  u32 size;  //!< size in bytes of data block
  int flags;           //!< PGMEMDAT_* flags or'ed together
};
#define PGMEMDAT_NEED_FREE    0x0001   //!< pgmemdata should be free()'d when done
#define PGMEMDAT_NEED_UNMAP   0x0002   //!< pgmemdata should be munmap()'d when done

//! \}

/******************** Administration */

/*!
 * \defgroup admin Administrative Functions
 *
 * Functions that affect the entire server, or the connection between client
 * and server. Includes pgInit, error handling, and exclusive access
 *
 * \{
 */

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
void pgSetErrorHandler(void (*handler)(u16 errortype,
				       const char *msg));

/*!
 * \brief Load an input driver by name and return a handle
 *
 * \param name Driver name as reported by 'pgserver -l'
 *
 * \returns A handle to the loaded driver
 */
pghandle pgLoadDriver(const char *name);

/*! 
 * \brief Convert a numerical errortype to a string
 * 
 * \param errortype A PG_ERRT_* error type constant
 * 
 * \returns A pointer to the corresponding string constant
 */
const char *pgErrortypeString(u16 errortype);

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
pgidlehandler pgSetIdle(s32 t, pgidlehandler handler); 

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
 * This function is recommended for animation. Areas of the screen other than
 * the specified widget and its children are never updated, and SubUpdates can
 * occur in toolbars even while a popup dialog is onscreen.
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
void pgBind(pghandle widgetkey,s16 eventkey,
	    pgevthandler handler,void *extra);

#ifdef FD_SET
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
 * IMPORTANT: The select() handler can not make any PicoGUI calls, because the
 * event queue is in an unknown state. The 'bottomhalf' function will be called
 * shortly after the select handler returns, and it is allowed to make
 * PicoGUI API calls.
 * 
 * To cancel this, call with a NULL handler and the select handle will be set
 * back to the standard select() system call.
 *
 * To use this function, the proper header files must be included
 * before picogui.h for select()
 * 
 * \sa pgSetIdle, pgEventLoop
 */
void pgCustomizeSelect(pgselecthandler handler, pgselectbh bottomhalf);
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
void pgSetVideoMode(u16 xres,u16 yres,
		    u16 bpp,u16 flagmode,
		    u32 flags);

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

/*!
 * \brief Send a message to the drivers
 *
 * \param message A PGDM_* driver message constant
 * \param param Defined by the type of message sent
 *
 * This command can send 'extra' commands that may be hardware-specific,
 * like beeps, cursor blanking, and backlight control.
 */
void pgDriverMessage(u32 message, u32 param);

/*!
 * \brief Send a message to a widget owned by any application
 *
 * \param dest Handle of the destination widget
 * \param data A pgmemdata structure containing the data, as returned by a pgFrom* function
 *
 * The \p data parameter is sent as the \p data in a PG_WE_APPMSG
 * event on behalf of the \p dest widget.
 */
void pgAppMessage(pghandle dest, struct pgmemdata data);

/*!
 * \brief Send a message to a widget owned by any application, and wait
 *        for an answer
 *
 * \param dest Handle of the destination widget
 * \param data A pgmemdata structure containing the data, as returned by a pgFrom* function
 *
 * \returns Returns a pointer to the answer message. This will have to
 *          be freed by the calling application.
 *
 * The \p data parameter is sent as the \p data in a PG_WE_APPMSG
 * event on behalf of the \p dest widget.
 *
 * This call acts very similarly to \p pgAppMessage, except that the
 * calling client is blocked until the remote widget has sent an
 * answer message. The answer is application specific.
 *
 * NOTE:
 * The data is assumed to be a structure whose very first field
 * is of type \p pghandle. This field will be used by pgSyncAppMessage
 * to store the widget handle to which the receiver will send
 * the answer.
 */
void * pgSyncAppMessage (pghandle dest, struct pgmemdata data);

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
pghandle pgEvalRequest(s16 reqtype, void *data, u32 datasize);

/*!
 * \brief Set the inactivity timer
 *
 * \param Inactivity timer value in milliseconds
 *
 * This sets the inactivity timer. Set it to zero periodically if you want
 * to prevent screensavers or sleep modes from activating even if there is
 * no user input.
 *
 * \sa pgGetInactivity
 */
void pgSetInactivity(u32 time);

/*!
 * \brief Get the inactivity timer
 *
 * \returns The inactivity timer value in milliseconds
 *
 * This timer is maintained by PicoGUI. It continually increments, but it is
 * cleared whenever user input is recieved and it can be set by pgSetInactivity
 * 
 * \sa pgSetInactivity
 */
u32 pgGetInactivity(void);

/*!
 * \brief Get a server resource
 *
 * \returns The resource handle associated with the given PGRES_* constant
 */
pghandle pgGetServerRes(u32 id);

/*!
 * \brief Get the name of a key
 *
 * \returns A string corresponding to a PGKEY_ code.  This function and its
 * sister pgKeyByName are meant, for example, for dumping input or
 * saving/loading keybindings.
 */
char *pgKeyName (u32 key);

/*!
 * \brief Get a key from its name
 *
 * \returns The PGKEY_ code corresponding to the given string.  This function
 * and its sister pgKeyByName are meant, for example, for dumping input or
 * saving/loading keybindings.
 */
u32 pgKeyByName (const char *name);
//! \}

/******************** Objects */

/*!
 * \defgroup pgobjects Object Manipulation
 *
 * Functions for creating and manipulating handles and the objects they
 * represent. Includes Applications, Widgets, and Strings.
 *
 * \{
 */

/*!
 * \brief Delete any object that has a handle 
 *
 * \param object A handle to any type of object (String, widget, bitmap, etc.)
 * 
 * This function frees the memory in the PicoGUI server associated with \p object.
 */
void pgDelete(pghandle object);

/*!
 * \brief Duplicate an object that has a handle
 *
 * \param object A handle to one of several types of PicoGUI objects
 *
 * Some objects simply can't be duplicated: For example, it would not make
 * sense to duplicate a widget, driver, or theme. At the time of this
 * writing, the only object type for which duplication is implemented is the
 * string object.
 *
 * \sa pgDelete, pgNewString
 */
pghandle pgDup(pghandle object);

/*!
 * \brief Change the handle context of an object
 *
 * \param object A handle to any PicoGUI object
 * \param delta The value to add to the context level
 * 
 * A positive delta value increases the object's context, equivalent to
 * adding extra pgEnterContext() layers. The delta value may be negative, to
 * 'send' the handle to a higher-level context. For example, you may want
 * to return data from a dialog box:
 * \code
pgEnterContext();
pgDialogBox("My Dialog");
... Allocate lots of memory ...
pgChangeContext(important_data,-1);
pgLeaveContext();
return important_data;
 * \endcode
 *
 * \sa pgEnterContext, pgLeaveContext
 */
void pgChangeContext(pghandle object, s16 delta);

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
 * \sa pgNewWidget, pgSetWidget, pgNewPopup, PG_APP_NORMAL, PG_APP_TOOLBAR
 */
pghandle pgRegisterApp(s16 type,const char *name, ...);

/*!
 * \brief Create a new widget, derived from a parent widget
 * 
 * \param type A PG_WIDGET_* constant for the widget type
 * \param rship A PG_DERIVE_* constant indicating the new widget's relationship to it's parent. It can be PGDEFAULT.
 * \param parent The parent widget's handle, or PGDEFAULT.
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
pghandle pgNewWidget(s16 type,s16 rship,
		     pghandle parent);

/*!
 * \brief Create a new widget without a parent
 *
 * \param type A PG_WIDGET_* constant for the widget type
 *
 * This function creates a widget, but does not attach it to the parent widget.
 * You can still set the widget's parameters and attach child widgets to this one,
 * but the widget cannot be drawn until you call pgAttachWidget.
 *
 * \sa pgAttachWidget, pgNewWidget
 */
pghandle pgCreateWidget(s16 type);

/*!
 * \brief Attach a widget to a new parent
 *
 * \param parent The parent widget's handle, or PGDEFAULT.
 * \param rship A PG_DERIVE_* constant indicating the new widget's relationship to it's parent. It can be PGDEFAULT.
 * \param widget The widget to attach
 *
 * This is necessary if you earlier created a widget using pgCreateWidget and now need to attach it
 * to a parent, or if you want to reattach a widget to a different parent. If the widget has any subwidgets,
 * they are moved along with the specified widget.
 *
 * \sa pgCreateWidget, pgDeleteWidget
 */
void pgAttachWidget(pghandle parent, s16 rship, pghandle widget);

/*!
 * \brief Finds a widget in relation to another widget
 *
 * \param widget The widget being referenced
 * \param direction A direction to traverse specified with a PG_TRAVERSE_* constant
 * \param count The number of steps to take in that direction
 *
 * There are four possible values for \p direction at this time:
 *  - PG_TRAVERSE_CHILDREN returns the count'th child of the specified widget
 *  - PG_TRAVERSE_FORWARD returns the widget added count'th widgets after this widget
 *  - PG_TRAVERSE_BACKWARD the opposite of forward
 *  - PG_TRAVERSE_CONTAINER travels to the widget's container, for \p count iterations
 *  - PG_TRAVERSE_APP travels to the root widget that contains the specified widget, then forward in the application list for \p count iterations. If the \p widget is 0, it returns the first app root widget. Note that the app list is continuously sorted by "Z-order".
 *
 */
pghandle pgTraverseWidget(pghandle widget, int direction, int count);

/*!
 * \brief Create a popup box, centered on the screen
 * \returns A handle to the popup box root widget
 *
 * \p width and/or \p height can be PGDEFAULT (zero) to determine the size automatically. This
 * is preferred because the app should assume as little as possible about physical
 * screen coordinates.
 *
 * NOTE: This function is now just a shortcut for creating a popup widget and setting
 *       its PG_WP_ABSOLUTEX, PG_WP_ABSOLUTEY, PG_WP_WIDTH, and PG_WP_HEIGHT properties
 *
 * \sa pgNewPopupAt
 */
pghandle pgNewPopup(int width,int height);

/*!
 * \brief Create a cursor that can be used for input filters
 * \returns A handle to the cursor
 *
 * \sa pgInFilterSend
 */
pghandle pgNewCursor(void);

/*!
 * \brief Create a new client-side input filter
 *
 * \param insert_after This is the handle of the input filter to insert the new one after, or 0 to make this the first
 * \param accept_trigs Mask of PG_TRIGGER_* constants for triggers to send in a PG_NWE_INFILTER event
 * \param absorb_trigs Specifies a mask of triggers to prevent from automatically passing on to the next filter
 *
 * \returns A handle to the new input filter
 *
 * \sa pgNewCursor, pgInFilterSend
 */
pghandle pgNewInFilter(pghandle insert_after, u32 accept_trigs, u32 absorb_trigs);

/*!
 * \brief Send an event back from a client-side input filter
 *
 * \param trig Client-side trigger union, representing the event
 *
 * \sa pgNewCursor, pgNewInFilter
 */
void pgInFilterSend(union pg_client_trigger *trig);

/*!
 * \brief Create a popup box at the specified position
 * \returns A handle to the popup box root widget
 *
 * \p width and/or \p height can be PGDEFAULT (zero) to determine the size automatically. This
 * is preferred because the app should assume as little as possible about physical
 * screen coordinates.
 *
 * \p x and/or \p y can be a PG_POPUP_* constant:
 *   - PG_POPUP_CENTER: Centered on the screen, same behavior as pgNewPopup
 *   - PG_POPUP_ATCURSOR: At the pointing device's cursor. If the cursor is over a button or menuitem, the popup snaps to its edge automatically
 * 
 * NOTE: This function is now just a shortcut for creating a popup widget and setting
 *       its PG_WP_ABSOLUTEX, PG_WP_ABSOLUTEY, PG_WP_WIDTH, and PG_WP_HEIGHT properties
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
s32 pgGetWidget(pghandle widget,s16 property);

/*!
 * \brief Create a new bitmap object from existing data
 * 
 * \param obj A pgmemdata structure, as returned by a pgFrom* function
 * \returns A handle to the new bitmap object created in the PicoGUI server
 * 
 * \sa pgFromMemory, pgFromFile, pgFromStream, pgFromTempMemory, pgDelete, pgEnterContext, pgLeaveContext, pgCreateBitmap
 */
pghandle pgNewBitmap(struct pgmemdata obj);

/*!
 * \brief Create a new bitmap object
 *
 * \param width Width, in pixels, of the new bitmap
 * \param height Height, in pixels, of the new bitmap
 * \returns A handle to the new bitmap object. It's contents are undefined
 *
 * \sa pgNewBitmap
 */
pghandle pgCreateBitmap(s16 width, s16 height);

/*!
 * \brief Map a bitmap into a shared memory segment
 *
 * \param bitmap Handle to the bitmap to map
 * \returns A pgshmbitmap structure with the SHM key and format info, valid until the next PicoGUI call
 *
 * This isn't well documented yet, see picogui/network.h for the pgshmbitmap
 * structure.
 *
 * \sa pgNewBitmap, pgCreateBitmap
 */
struct pgshmbitmap *pgMakeSHMBitmap(pghandle bitmap);

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
 * \brief Create a new array object  
 * 
 * \param dat The data to put in the array 
 * \param size Number of entries in the array 
 * \returns A handle to the new array object 
 * 
 */  
pghandle pgNewArray(const s32* dat, u16 size);  
 
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

/*!
 * \brief Create a new font object
 * 
 * \param name The name of the font to search for, or NULL
 * \param size The size (height in pixels) of the font to search for, or zero
 * \param style Zero or more PG_FSTYLE_* flags or'ed together
 * 
 * \returns A handle to the new font object created in the PicoGUI server
 * 
 * Based on the supplied parameters, finds the closest installed font and creates
 * an object describing it. For example:
 * \code
fDefault = pgNewFont(NULL,0,PG_FSTYLE_DEFAULT);                  // Find the font marked as default
fBold    = pgNewFont(NULL,0,PG_FSTYLE_DEFAULT | PG_FSTYLE_BOLD); // Bold version of the default font
fBig     = pgNewFont(NULL,40,PG_FSTYLE_ITALIC);                  // A large italic font
fFlush   = pgNewFont("Helvetica",0,PG_FSTYLE_FLUSH);             // Helvetica at the default size, with no space at the edges
 * \endcode
 * 
 * \sa pgNewString, pgDelete, pgEnterContext, pgLeaveContext, pgGetFontStyle
 */
pghandle pgNewFont(const char *name,s16 size,u32 style);

/*!
 * \brief Get information about a font style
 *
 * \param index A zero-based index to select a font style in the order that
 *              the were compiled or loaded into pgserver
 * \param name  Pointer to a buffer to store the font name in. Must be
 *              40 bytes long
 * \param size  Pointer that the font size is returned in. For bitmapped
 *              fonts (all PicoGUI currently supports) this is height in
 *              pixels
 * \param fontrep Pointer that the font representation is returned in. This
 *                is a combination of one or more PG_FR_* flags.
 * \param flags   Pointer that font style flags are returned in. This is
 *                a combination of PG_FSTYLE_* flags
 * \returns Nonzero if the index was valid and data was stored in the supplied
 *          pointers
 *
 * This function can be used to iterate through the available fonts. For
 * example:
 * \code
char name[40];
u16 size;
u16 fontrep;
u32 flags;
s16 i;

i = 0;
while (pgGetFontStyle(i++, name, &size, &fontrep, &flags)) {
   printf("Font #%d: %s\n"
          "    size: %d\n"
	  " fontrep: 0x%04X\n"
	  "   flags: 0x%08X\n\n",
	  i,name,size,fontrep,flags);
}
 * \endcode
 *
 * \sa pgNewFont
 */
int pgGetFontStyle(s16 index, char *name, u16 *size,
		   u16 *fontrep, u32 *flags);

/*!
 * \brief Measure a string of text
 * 
 * \param w The address to return the width in
 * \param h The address to return the height in
 * \param font A font to render the text in
 * \param text A handle to the text to measure
 * 
 * In \p *w and \p *h, returns the size in pixels of the given text
 * in the given font. Font may be PGDEFAULT to use the default font.
 * 
 * Note that if you use pgNewText to create a string object just for this function
 * call, you should delete it afterwards to prevent a memory leak:
 * \code
pghandle sText;
int w,h;

sText = pgNewString("Hello, World!");
pgSizeText(&w,&h,PGDEFAULT,sText);
pgDelete(sText);
 * \endcode
 * 
 * Alternatively, defining a context with pgEnterContext and pgLeaveContext
 * will clean up the string object automatically:
 * \code
pgEnterContext();
pgSizeText(&w,&h,PGDEFAULT,pgNewString("Hello, World!");
pgLeaveContext();
 * \endcode
 * 
 * \sa pgEnterContext, pgLeaveContext, pgNewString, pgNewFont
 */
void pgSizeText(int *w,int *h,pghandle font,pghandle text);

/*!
 * \brief Return the size of a bitmap object
 *
 * \param w The address to return the width in
 * \param h The address to return the height in
 * \param bitmap Handle to a valid PicoGUI bitmap object
 *
 * \sa pgCreateBitmap, pgNewBitmap
 */
void pgSizeBitmap(int *w, int *h, pghandle bitmap);

/*!
 * \brief Load a compiled theme
 * 
 * \param obj A pgmemdata structure, as returned by a pgFrom* function
 * \returns A handle to the new theme object created in the PicoGUI server
 * 
 * The compiled theme data can be generated using the \p themec utility. The theme
 * can be unloaded by calling pgDelete with the returned theme handle.
 * 
 * \sa pgFromMemory, pgFromFile, pgFromStream, pgFromTempMemory, pgDelete, pgEnterContext, pgLeaveContext
 */
pghandle pgLoadTheme(struct pgmemdata obj);

/*!
 * \brief Load a compiled Widget Template
 * 
 * \param obj A pgmemdata structure, as returned by a pgFrom* function
 * \returns A handle to the new Widget Template object created in the PicoGUI server
 * 
 * This widget template is like a cookie-cutter that can be used to instantiate a whole tree
 * of widgets or other objects. You instantiate the template using pgDup().
 * 
 * \sa pgFromMemory, pgFromFile, pgFromStream, pgFromTempMemory, pgDelete, pgDup
 */
pghandle pgLoadWidgetTemplate(struct pgmemdata obj);

/*!
 * \brief Find a theme object's ID given its name
 * 
 * \param key The "name" property to search for
 * \returns The theme ID, or zero if it's not found
 *
 * This function is useful for finding custom theme objects. Theme objects
 * defined as \p custom are assigned an ID automatically at load time.
 * These objects can be found with this function as long as each is assigned
 * a unique \p name property.
 *
 * \sa pgLoadTheme, pgFindWidget
 */
int pgFindThemeObject(const char *key);

/*!
 * \brief Load memory into a string handle
 * 
 * \param obj A pgmemdata structure, as returned by a pgFrom* function
 * \returns A handle to the new string object created in the PicoGUI server
 * 
 * This is like pgNewString() except that the string is loaded from
 * a pgmemdata structure, and it does not need to be null-terminated. This
 * makes it easy to load text files, for instance.
 * 
 * \sa pgFromMemory, pgFromFile, pgFromStream, pgFromTempMemory, pgDelete, pgEnterContext, pgLeaveContext
 */
pghandle pgDataString(struct pgmemdata obj);

/*!
 * \brief Retrieve a theme property
 *
 * \param object A PGTH_O_* theme object constant
 * \param property A PGTH_P_* theme property constant
 *
 * \returns The theme property's value
 *
 * \sa pgLoadTheme
 */
u32 pgThemeLookup(s16 object, s16 property);

/*! 
 * \brief Set an object's payload
 * 
 * \param object A handle to any PicoGUI object
 * \param payload A 32-bit piece of application-defined data
 * 
 * The "payload" is a client-defined chunk
 * of data attatched to any object that has a handle.
 * Some good uses for this are assigning numerical values to
 * buttons, or even creating a linked list of objects
 * by storing a handle in the payload.
 * It is usually possible for the client to store pointers
 * in the payload, but this is not recommended, for two reasons:
 *  - If the pgserver is buggy or compromised, the client is vulnerable to crashes or data corruption
 *  - If the client-side architecture uses pointers of more than 32 bits, it will not work
 * 
 * \sa pgGetPayload, pgGetEvent
 */
void pgSetPayload(pghandle object,u32 payload);

/*!
 * \brief Get an object's payload
 * 
 * \param object A handle to any PicoGUI object
 * \returns The 32-bit piece of application-defined data set using pgSetPayload
 * 
 * See pgSetPayload for more information about payloads and their uses.
 * 
 * \sa pgSetPayload
 */
u32 pgGetPayload(pghandle object);

/*!
 * \brief Write data to a widget
 *
 * \param widget The handle of the widget to receive data
 * \param data A pgmemdata structure containing the data, as returned by a pgFrom* function
 * 
 * Write a chunk of widget-defined data to a widget. For example, this can be used to send
 * text to a terminal widget or commands to a canvas widget. (For canvas drawing pgWriteCmd or PGFX
 * should usually be used instead)
 * 
 * \sa pgFromMemory, pgFromFile, pgFromStream, pgFromTempMemory, PG_WIDGET_TERMINAL, PG_WIDGET_CANVAS, pgWriteCmd, pgNewCanvasContext
 */
void pgWriteData(pghandle widget,struct pgmemdata data);

/*!
 * \brief Write a command to a widget
 *
 * \param widget The handle of the widget to receive the command
 * \param command A widget-defined command number
 * \param numparams The number of parameters following this one
 * 
 * This function creates a pgcommand structure from it's arguments and
 * uses pgWriteData to send it to the specified widget.
 * Currently this is used as the low-level interface to the canvas widget.
 * 
 * \sa pgWriteData, PG_WIDGET_CANVAS, pgNewCanvasContext
 */
void pgWriteCmd(pghandle widget,s32 command,s16 numparams, ...);

/*!
 * \brief Render a gropnode to a bitmap
 *
 * \param bitmap A bitmap handle to render to. Alternatively, if the app has registered exclusive display access this can be zero to draw directly to the display.
 * \param groptype A PG_GROP_* constant indicating the type of gropnode
 *
 * Gropnode parameters follow the gropnode type.
 *
 * \sa pgWriteCmd, pgNewBitmapContext, pgRegisterOwner
 */
void pgRender(pghandle bitmap,s16 groptype, ...);

/*!
 * \brief Search for a widget by its PG_WP_NAME property
 *
 * \param key The name to search for
 * \return The handle of the found widget, or zero if no widget matches
 *         the supplied name
 *
 * Every widget can be given a name by setting it's PG_WP_NAME property
 * to a string handle. This function can search for a widget's handle based
 * on this name. Note that this function will search all widgets, even those
 * not owned by this application.
 *
 * \sa PG_WP_NAME, pgSetWidget
 */
pghandle pgFindWidget(const char *key);

//! \}

/******************** Data loading */

/*!
 * \defgroup dataload Data Loading functions
 *
 * The pgFrom*() functions specify various ways to reference data using
 * the pgmemdata structure.
 *
 * \{
 */

/*!
 * \brief Refer to data loaded into memory
 * 
 * \param data A pointer to data loaded into memory
 * \param length The length, in bytes, of the data referred to
 * \returns A pgmemdata structure describing the data
 * 
 * When using pgFromMemory, the data pointer must remain valid for
 * a relatively long period of time, usually until the request buffer
 * is flushed. If you would rather have the client library free the
 * memory for you when it is done, see pgFromTempMemory
 * 
 * \sa pgFromFile, pgFromStream, pgFromTempMemory
 */
struct pgmemdata pgFromMemory(void *data,u32 length);

/*!
 * \brief Refer to data loaded into memory, free when done
 * 
 * \param data A pointer to data loaded into memory
 * \param length The length, in bytes, of the data referred to
 * \returns A pgmemdata structure describing the data
 * 
 * The data pointer must have been dynamically allocated with malloc() or equivalent.
 * When the client library is done using it, \p data will be freed with the free() function.
 * 
 * \sa pgFromMemory, pgFromFile, pgFromStream
 */
struct pgmemdata pgFromTempMemory(void *data,u32 length);


/*!
 * \brief Refer to data in a file
 * 
 * \param file The name of the file containing data to be referred to
 * \returns A pgmemdata structure describing the data. This is needed by many PicoGUI API functions that require data as input.
 * 
 * Depending on implementation the file may be loaded into memory temporarily, or memory-mapped if possible
 * 
 * \sa pgFromMemory, pgFromTempMemory, pgFromStream
 */
struct pgmemdata pgFromFile(const char *file);

/*!
 * \brief Refer to data in an opened stream
 * 
 * \param f C stream, as returned by \p fopen() in \p stdio.h
 * \param length The number of bytes to read from the stream
 * \returns A pgmemdata structure describing the data
 * 
 * Depending on implementation, the data may be read from the stream into memory, or memory-mapped if possible.
 * The chunk of data referred to begins at the stream's current position and extends \p length bytes past it.
 * The stream's position is advanced by \p length bytes.
 * 
 * \sa pgFromMemory, pgFromTempMemory, pgFromFile
 */
struct pgmemdata pgFromStream(FILE *f, u32 length);

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

//! \}

/******************** Program flow */

/*!
 * \defgroup progflow Program Flow
 *
 * Event loops and handle contexts
 *
 * \{
 */

/*!
 * \brief Event processing and dispatching loop
 * 
 * pgEventLoop waits for events from the PicoGUI server and dispatches
 * them according to bindings set up with pgBind. The handler set with
 * pgSetIdle is also called if applicable.
 * 
 * pgEventLoop can be called more than once throughout the life of the
 * program, but it is not re-entrant.
 * 
 * If the app recieves an event while it is not waiting in an
 * EventLoop, the server will queue them until the client is
 * ready.
 * 
 * \sa pgGetEvent, pgExitEventLoop, pgBind, pgSetIdle
 */
void pgEventLoop(void);

/*!
 * \brief Exit the current event loop
 * 
 * If the client is currently inside an event loop, this function
 * sets a flag to exit it at the next possible opportunity
 * 
 * \sa pgEventLoop
 */
void pgExitEventLoop(void);

/*!
 * \brief Wait for a single event
 *
 * \returns A pgEvent structure
 * 
 * This is good for small dialog boxes, or other situations when pgBind and
 * pgEventLoop are overkill. pgGetEvent can be used while an event loop is already
 * in progress, for example in a pgSetIdle or pgBind handler function.
 *
 * You can also use this in combination with pgCheckEvent to passively check
 * for new events while performing some other operation, such as animation.
 *
 * Important! Note that the returned pointer is only valid until the next
 * PicoGUI call! It's usually a good idea to use something like this:
 *
 * \code

struct pgEvent evt;

evt = *pgGetEvent();

 * \endcode
 *
 * If the relevant values from the pgEvent structure will be copied elsewhere
 * before the next PicoGUI call, that is alright too. Thus, the following code
 * is perfectly fine:
 *
 * \code

i = pgGetPayload( pgGetEvent()->from );

 * \endcode
 * 
 * \sa pgEventLoop, pgBind, pgSetIdle, pgSetPayload, pgGetPayload, pgCheckEvent
 */
struct pgEvent *pgGetEvent(void);

/*!
 * \brief Check the number of pending events
 * 
 * \returns The number of events in the application's queue
 * 
 * The PicoGUI server keeps a ring buffer of waiting events for each
 * client connected to it. This function returns the number of events waiting
 * in this buffer. Note that this buffer is usually relatively small.
 * At the time of this writing, it is set to hold 16 events. If the buffer
 * is full, old events will be discarded.
 * 
 * You can use this function if, for some reason, you need to poll PicoGUI
 * events instead of waiting for them. In the middle of a long operation,
 * for example, you may wish to periodically check if the user clicks a cancel
 * button. If this function indicates that there are events waiting,
 * pgGetEvent will return immediately with the oldest queued event.
 * 
 * \sa pgGetEvent, pgEventLoop
 */
int pgCheckEvent(void);

/*!
 * \brief Dispatch an event to registered handlers
 *
 * \param evt Pointer to the event to dispatch. This should not be the same
 *            pointer returned by pgGetEvent(), as it is only valid until the
 *            next PicoGUI call! See pgGetEvent() for more information.
 *
 * This function searches all registered event handlers, and dispatches the
 * event to any applicable handlers. It also provides various default handlers,
 * such as closing the program on recieving PG_WE_CLOSE.
 *
 * \sa pgGetEvent, pgCheckEvent, pgEventLoop
 */
void pgDispatchEvent(struct pgEvent *evt);

/*!
 * \brief Get and dispatch new events if there are any
 *
 * This function is a non-blocking version of pgEventLoop().
 * It calls pgCheckEvent(), and if there are any new events it uses
 * pgGetEvent() and pgDispatchEvent() to retrieve and process any
 * pending events.
 *
 * This is good to call during an animation or other lengthy operation to
 * check for the user clicking the close button, canceling the operation, etc.
 *
 * \sa pgGetEvent, pgCheckEvent, pgDispatchEvent
 */
void pgEventPoll(void);

/*!
 * \brief Enter a new context
 * 
 * PicoGUI uses a context system, similar to a variable's scope in C.
 * Whenever the program leaves a context, all objects created
 * while in that context are deleted. No memory is used by creating a context,
 * and they can be nested a very large number of times.
 *
 * \returns the ID of the new context
 * 
 * Here is an example, indented to show the context levels:
 * \code
pghandle x,y,z;

pgEnterContext();
  x = pgNewString("X");
  pgEnterContext();
    y = pgNewString("Y");
  pgLeaveContext();           // y is deleted
  z = pgNewString("Z");
pgLeaveContext();             // x and z are deleted
 * \endcode
 * 
 * \sa pgLeaveContext
 */
int pgEnterContext(void);

/*!
 * \brief Leave a context
 * 
 * When leaving a context, all objects created within it are deleted, and the context
 * ID is decremented. This default behavior simulates a stack of contexts.
 * See pgEnterContext for an example.
 * 
 * \sa pgEnterContext, pgDeleteHandleContext
 */
void pgLeaveContext(void);

/*!
 * \brief Delete all handles in one context
 *
 * This lets you use contexts as individuals with an ID rather than as a stack.
 * pgLeaveContext() deletes the current context (stored per-connection) and
 * decrements that current context. This function deletes the specified context
 * without touching the current context number. This way new contexts can be
 * requested and discarded indefinitely (or at least until the IDs wrap around,
 * in which case the server will skip context nubmers that are in use)
 *
 * \sa pgEnterContext
 */
void pgDeleteHandleContext(int id);

/*!
 * \brief Set the context ID used when creating new handles
 *
 * \sa pgDeleteHandleContext, pgEnterContext, pgLeaveContext, pgGetContext
 */
void pgSetContext(int id);


/*!
 * \brief Get the current context ID
 *
 * \sa pgDeleteHandleContext, pgEnterContext, pgLeaveContext, pgSetContext
 */
int pgGetContext(void);

//! \}

//! \}

#endif /* __H_PG_CLI_C */
/* The End */
