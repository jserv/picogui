/* $Id: client_c.h,v 1.43 2001/05/04 23:26:55 micahjd Exp $
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

/******************** Client-specific constants and data types */

/* Generic event structure passed to event handlers */
struct pgEvent {
   short type;      /* Event type, a PG_WE_* or PG_NWE_* constant */
   pghandle from;   /* The widget it was recieved from (if applicable) */
   void *extra;     /* Extra data passed to the event handler via pgBind */
   
   /* Event-specific parameters */
   union {
      
      /* The generic parameter */
      unsigned long param;
      
      /* Width and height, for resize and build events */
      struct {
	 short w;
	 short h;
      } size;

      /* Modifiers and key, for keyboard events */
      struct {
	 short mods;
	 short key;
      } kbd;
      
      /* Pointing device information, for PG_WE_PNTR_*
       * and PG_NWE_PNTR_* events */
      struct {
	 short x,y;    /* Position */
	 short btn;    /* Bitmask of pressed buttons */
	 short chbtn;  /* Bitmask of buttons changed since last event */
      } pntr;
      
      /* Streamed data, from the PG_WE_DATA event */
      struct {
	 unsigned long size;
	 char *pointer;      /* Automatically freed */
      } data;
      
   } e;
};

/* A wildcard value for pgBind */
#define PGBIND_ANY      -1

/* A wildcard value for pgNewFont */
#define PGFONT_ANY      0

/* A more verbose way of using the default widget or rship (widget
   relationship) in PicoGUI function calls. Just using 0 is
   perfectly acceptable, but this can make your code easier to
   read. */
#define PGDEFAULT       0

/* For forming fractions, such as the size when PG_SZMODE_CNTFRACT is used */
#define pgFraction(n,d) (((n)<<8)|(d))

/* event handler used in pgBind */
typedef int (*pgevthandler)(struct pgEvent *evt);
/* event handler for pgSetIdle */
typedef void (*pgidlehandler)(void);
#ifdef FD_SET
/* event hander for pgCustomizeSelect */
typedef int (*pgselecthandler)(int n, fd_set *readfds, fd_set *writefds,
			       fd_set *exceptfds, struct timeval *timeout);
#endif

/* Structure representing data, loaded or mapped into memory.
 * This is returned by the pgFrom* series of functions for loading
 * data. You probably shouldn't use anything in this structure
 * directly, for compatibility reasons.
 */
struct pgmemdata {
  void *pointer;       /* when null, indicates error */
  unsigned long size;
  int flags;           /* PGMEMDAT_* flags or'ed together */
};
#define PGMEMDAT_NEED_FREE    0x0001   /* Should be free()'d when done */
#define PGMEMDAT_NEED_UNMAP   0x0002   /* Should be munmap()'d when done */

/******************** Administration */

/* See if there are any command line args relevant to PicoGUI
 * (such as for setting the PicoGUI server) and establish
 * a connection to the server 
 *
 * On error, it exits the program with an error message.
 *
 * Shutdown (disconnecting from the server, freeing memory)
 * is handled via atexit()
 */
void pgInit(int argc, char **argv);

/* This sets up an error handler. Normally, errors from the
 * server are displayed and the program is terminated.
 * Server errors should only happen if there is a bug in the
 * program, the system is out of memory, or another Bad
 * Thing happened.  To override this behavior, set up a new
 * error handler
 */
void pgSetErrorHandler(void (*handler)(unsigned short errortype,
				       const char *msg));

/* Convert a numerical errortype to a string. Useful for
 * error handlers
 */
const char *pgErrortypeString(unsigned short errortype);

/* After 't' milliseconds of event loop inactivity,
 * the supplied function is called.
 * To deactivate the idle handler, set 't' to 0 or the handler to NULL.
 * 
 * Returns the previous idle handler, if any.
 * 
 * This is based on the pgSetnonblocking code added by Philippe, but
 * this fits the event-driven model better, and most importantly it
 * handles stopping and starting the event loop automatically.
 * Note that it is still possible for PicoGUI to block if the server
 * sends a partial reply packet, but even if the idle handler were called
 * during this time the network connection would be 'jammed' so there wouldn't
 * be much point. 
 */
pgidlehandler pgSetIdle(long t,pgidlehandler handler); 

/* Flush the request buffer, make sure everything is sent to
 * the server. Usually this is handled automatically, but
 * it might be needed in some rare situations...
 */
void pgFlushRequests(void);

/* Update the screen. 
 *
 * If your application is pgEventLoop (or pgGetEvent) based,
 * this is handled automatically.
 *
 * Do not use this for animation, use pgSubUpdate() instead
 */
void pgUpdate(void);

/* Update a subsection of the screen
 *
 * The given widget and all other
 * widgets contained within it. The section is redrawn independantly and
 * immediately. (This command does flush the buffers)
 *
 * This can be used for animation (changing widget parameters without
 * user input)
 */
void pgSubUpdate(pghandle widget);

/* Attatch an event handler to a widget and/or event.
 * Widgetkey may be PGDEFAULT to attach to the most recent widget, or
 * PGBIND_ANY to respond to any widget's events.
 * Eventkey may be a PG_WE_* or PG_NWE_* constant to match a particular event
 * or PGBIND_ANY to match any event.
 * When the widgetkey and eventkey both match, the handler is called,
 * and the specified value for extra is passed in its pgEvent structure.
 * 
 * If widgetkey and eventkey are exactly the same as an existing binding, its
 * handler and extra value are reset to the ones specified here. If handler is
 * NULL the binding is deleted.
 */
void pgBind(pghandle widgetkey,unsigned short eventkey,
	    pgevthandler handler,void *extra);

/* This is how to wait for your own file descriptors during the
 * PicoGUI event loop's select(). If the handler is non-null,
 * use the user-supplied handler instead of select()
 *
 * To cancel this, call with a NULL handler
 *
 * To use this function, the proper header files must be included
 * before picogui.h for select()
 */
#ifdef FD_SET
void pgCustomizeSelect(pgselecthandler handler);
#endif

/* These functions register and unregister, respectively, exclusive access
 * for the specified resource. The parameter must be a PG_OWN_* constant
 */
void pgRegisterOwner(int resource);
void pgUnregisterOwner(int resource);

/* This function is used by networked input devices to send keyboard events
 * as if from a local keyboard.
 * 
 * type must be a PG_TRIGGER_* constant, the events follow the same conventions
 * as the compiled-in video drivers.
 */
void pgSendKeyInput(unsigned long type,unsigned short key,
		    unsigned short mods);

/* Also used by networked input devices, but to send pointing device events */
void pgSendPointerInput(unsigned long type,unsigned short x,unsigned short y,
			unsigned short btn);

/* Change video mode at runtime
 * xres and yres specify a new resolution, or 0 to not change it
 * bpp specifies a new bit depth, 0 to not change it
 * flagmode is a PG_FM_* constant specifying how to combine the specified
 * flags with the existing flags. Flags contol extra driver features such
 * as fullscreen mode and screen rotation.
 */
void pgSetVideoMode(unsigned short xres,unsigned short yres,
		    unsigned short bpp,unsigned short flagmode,
		    unsigned long flags);

/* Get information about the current video mode.
 * The returned pointer is good only until the next PicoGUI call */
struct pgmodeinfo *pgGetVideoMode(void);

/******************** Objects */

/* Delete any object that has a handle */
void pgDelete(pghandle object);

/* Give a widget the keyboard focus */
void pgFocus(pghandle widget);

/* Register application. The type and name are required.
 * Optional specifications (PG_APPSPEC_*) are specified 
 * in name-value pairs, terminated with a 0.
 *
 * Example:
 *   pgRegisterApp(PG_APP_NORMAL,"My App",
 *                 PG_APPSPEC_SIDE,PG_S_TOP,
 *                 PG_APPSPEC_MINHEIGHT,50,
 *                 0);
 *
 */
pghandle pgRegisterApp(short int type,const char *name, ...);

/* Creates a new widget, derived from a parent widget
 * using the spefified relationship (PG_DERIVE_* constant)
 *
 * If the parent is null, default to the last widget created
 */
pghandle pgNewWidget(short int type,short int rship,
		     pghandle parent);

/* Make a new popup box, centered on the screen. After
 * creating a popup box, widgets are placed inside it by
 * default (if NULL is used for 'parent')
 * If you need to specify the x,y position, use pgNewPopupAt
 */
pghandle pgNewPopup(int width,int height);
pghandle pgNewPopupAt(int x,int y,int width,int height);

/* Set properties of a widget. If the widget is null, default
 * to the last widget created. After that, it accepts a list
 * of property-value pairs, terminated by a 0.
 */
void pgSetWidget(pghandle widget, ...);

/* Return a widget property. */
long pgGetWidget(pghandle widget,short property);

/* Create a new bitmap object. */
pghandle pgNewBitmap(struct pgmemdata obj);

/* Create a new string object */
pghandle pgNewString(const char *str);

/* Evaluate a PicoGUI request packet. This is a good way
 * to reuse PicoGUI's serialization capabilities to load
 * a generic binary object from file. It is advisable to
 * validate the request's type first so you don't allow
 * the input to do wierd things like change video mode
 * or leave the current context. 
 *
 * If the request does not return a handle, it will still
 * run but pgEvalRequest()'s return value is undefined
 */
pghandle pgEvalRequest(short reqtype, void *data, unsigned long datasize);

/* Get the contents of a string handle.
 *
 * The dynamically allocated string is managed by PicoGUI,
 * but never rely on its integrity after another PicoGUI
 * function call. If you need it for a long time, copy it.
 */
char *pgGetString(pghandle string);

/* Deletes the widget's previous text, and
 * sets the widget's text to a newly allocated
 * string object.
 *
 * This is the preferred way of setting or
 * changing the text of a button, label, or other
 * widget that takes a PG_WP_TEXT property.
 */
void pgReplaceText(pghandle widget,const char *str);

/* Like pgReplaceText, but supports printf-style
 * text formatting */
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
