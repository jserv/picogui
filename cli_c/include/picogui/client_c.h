/* $Id: client_c.h,v 1.14 2000/10/26 20:00:53 pney Exp $
 *
 * picogui/client_c.h - The PicoGUI API provided by the C client lib
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Contributors: Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 * 
 */

#ifndef _H_PG_CLI_C
#define _H_PG_CLI_C

/******************** Client-specific constants and data types */

/* A wildcard value for pgBind */
#define PGBIND_ANY      -1

/* A wildcard value for pgNewFont */
#define PGFONT_ANY      0

/* A more verbose way of using the default widget or rship (widget
   relationship) in PicoGUI function calls. Just using 0 is
   perfectly acceptable, but this can make your code easier to
   read. */
#define PGDEFAULT       0

/* event handler used in pgBind */
typedef void (*pgevthandler)(short event,pghandle from,long param);

/* Structure representing data, loaded or mapped into memory.
 * This is returned by the pgFrom* series of functions for loading
 * data. You probably shouldn't use anything in this structure
 * directly, for compatibility reasons.
 */
struct pgmemdata {
  void *pointer;
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

/* Set receive state to blocking or not. To have the possibility
 * to introduce a timeout in the EventLoop
 */
void pgSetnonblocking(long state);

/* Flush the request buffer, make sure everything is sent to
 * the server. Usually this is handled automatically, but
 * it might be needed in some rare situations...
 */
void pgFlushRequests(void);

/* Update the screen. Call this once after setting up a new
 * popup box, app panel, or making a change that needs to
 * become visible. Use this sparingly. Only things that have
 * been changed get recalculated, but drawing unfinished 
 * popups or applications to the screen is a Bad Thing (tm)
 */
void pgUpdate(void);

/* Attatch an event handler to a widget and/or event. A NULL
 * widget uses the default, as usual. Either the handle or the
 * event (or both!) can be the wildcard PGBIND_ANY to match all
 * handles/events. If a handler with these properties already
 * exists, it is not removed. If the widget a handler refers to
 * is deleted, the handler is deleted however.
 */
void pgBind(pghandle widgetkey,unsigned short eventkey,
	    pgevthandler handler);

/******************** Objects */

/* Delete any object that has a handle */
void pgDelete(pghandle object);

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
   parameters. Any of them can be 0 (or PGFONT_ANY)
   to ignore that parameter.
*/
pghandle pgNewFont(const char *name,short size,unsigned long style);

/* Load a compiled theme file into the server */
pghandle pgLoadTheme(struct pgmemdata obj);

/******************** Data loading */

/* Data already loaded in memory */
struct pgmemdata pgFromMemory(void *data,unsigned long length);

/* Load from a normal disk file */
struct pgmemdata pgFromFile(const char *file);

/* TODO: Load from resource. Allow apps to package necessary bitmaps
   and things in a file, named after their binary but with a '.res'
   extension.
   The server will also be able to request reloading data from these
   resource files, for example to reload bitmaps when the bit depth
   changes.

   This is just an idea, and I'll implement it later...
   This while pgmemdata business is just my attempt to leave enough
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

/* PicoGUI uses a context system, similar to contexts in C.
 * Whenever the program leaves a context, all objects created
 * while in that context are deleted. Contexts can be nested
 * almost infinitely (Well, 32768 or so... ;-)
 */
void pgEnterContext(void);
void pgLeaveContext(void);


#endif /* __H_PG_CLI_C */
/* The End */
