/* $Id: client_c.h,v 1.2 2000/09/15 18:10:48 pney Exp $
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


/********************* Include files *********************/
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h>

#include "network.h"


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
void pgErrorHandler(void (*handler)(short int errortype,
				    const char *msg));

/******************** Objects */

/* Delete any object that has a handle */
void pgDelete(pghandle object);

/* Creates a new widget, derived from a parent widget
 * using the spefified relationship (PG_DERIVE_* constant)
 *
 * If the parent is null, default to the last widget created
 */
pghandle pgNewWidget(short int type,short int rship,
		     pghandle parent);

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


/* Flushes the buffer of packets
 */
long _flushpackets(const void *in_pgr,int pgr_len,
                   const void *in_data,int data_len,
		   struct pgreturn *in_pgret);

/* Like send, but with some error checking stuff.  Returns nonzero
 * on error.
 */
int send_response(int to,const void *data,int len);

void Update();
void _wait();
void _mkpopup(short in_x,short in_y,short in_w,short in_h);
void NewPopup(short in_x,short in_y,short in_w,short in_h);


#endif /* __H_PG_CLI_C */
/* The End */
