/* $Id: client_c.h,v 1.7 2000/09/22 18:04:28 pney Exp $
 *
 * picogui/client_c.h - The PicoGUI API provided by the C client lib
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

/******************** Client-specific constants */

/* A wildcard value for pgBind */
#define PGBIND_ANY      -1

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

/* Attatch an event handler to a widget and/or event. A NULL
 * widget uses the default, as usual. Either the handle or the
 * event (or both!) can be the wildcard PGBIND_ANY to match all
 * handles/events. If a handler with these properties already
 * exists, it is not removed. If the widget a handler refers to
 * is deleted, the handler is deleted however.
 */
void pgBind(pghandle widgetkey,unsigned short eventkey,
	    void (*handler)(unsigned short event,pghandle from,
			    unsigned long param));

/* Add a pair widget/event in the binding list.
 * If the pair is the first on, return the pointer on it
 * else return NULL.
 */
struct pgbindlist *pgBindAdd(struct pgbindlist *bind);

/* Set properties of a widget. If the widget is null, default
 * to the last widget created. After that, it accepts a list
 * of property-value pairs, terminated by a 0.
 */
void pgSetWidget(pghandle widget, ...);

/* Create a new string object */
pghandle pgNewString(const char *str);

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

/* PicoGUI uses a context system, similar to contexts in C.
 * Whenever the program leaves a context, all objects created
 * while in that context are deleted. Contexts can be nested
 * almost infinitely (Well, 32768 or so... ;-)
 */
void pgEnterContext(void);
void pgLeaveContext(void);


#endif /* __H_PG_CLI_C */
/* The End */
