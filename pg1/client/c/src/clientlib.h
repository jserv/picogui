/* $Id$
 *
 * clientlib.h - definitions used only within the client library code itself
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
 * Philippe Ney <philippe.ney@smartdata.ch>
 * Brandon Smith <lottabs2@yahoo.com>
 * 
 * 
 */

#ifndef _CLIENTLIB_H
#define _CLIENTLIB_H


/* System includes */
#include <sys/types.h>
#ifndef __NetBSD__
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#else
#include <sys/types.h>
#include <sys/time.h>  /* for time_t type (used in timeval structure) */
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif
#include <fcntl.h>
#ifndef CONFIG_UNIX_SOCKET
#include <netinet/in.h>
#else
#include <sys/un.h>
#endif
#include <netdb.h>
#include <stdio.h>    /* for fprintf() */

/* FIXME: Check for Mac OS X using autoconf */
#if (defined(__APPLE__) && defined(__MACH__)) // Mac OS X and Darwin 
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif

#include <signal.h>
#include <unistd.h>   /* select() */
#include <string.h>   /* for memcpy(), memset(), strcpy() */
#include <stdarg.h>   /* needed for pgRegisterApp and pgSetWidget */
#include <stdlib.h>   /* for getenv() */

#ifdef __NetBSD__
#include <unistd.h>
#endif

/* PicoGUI */
#include <picogui.h>            /* Basic PicoGUI include */
#include <picogui/network.h>    /* Network interface to the server */

//#define DEBUG
//#define DEBUG_EVT

/* Default server */
#ifndef CONFIG_UNIX_SOCKET 
#  define PG_REQUEST_SERVER       "127.0.0.1"
#else
#  define PG_REQUEST_SERVER       "/var/tmp/.pgui"
#endif

/* Buffer size. When packets don't need to be sent immediately,
 * they accumulate in this buffer. It doesn't need to be very big
 * because most packets are quite small. Large packets like bitmaps
 * won't fit here anyway, so they are sent immediately.
 */
#define PG_REQBUFSIZE           512

/* A node in the list of event handlers set with pgBind */
struct _pghandlernode {
  pghandle widgetkey;
  s16 eventkey;
  pgevthandler handler;
  void *extra;
  struct _pghandlernode *next;
};

/* Structure for a retrieved and validated response code,
   the data collected by _pg_flushpackets is stored here. */
struct _pg_return_type {
  s16 type;
  union {

    /* if type == PG_RESPONSE_RET */
    u32 retdata;

    /* if type == PG_RESPONSE_EVENT */
    struct pgEvent event;

    /* if type == PG_RESPONSE_DATA */
    struct {
      u32 size;
      void *data;         /* Dynamically allocated - should be freed and
			     set to NULL when done, or it will be freed
			     next time flushpackets is called */
    } data;

  } e;  /* e for extra? ;-) */
};

/* Global vars for the client lib */
extern int _pgsockfd;                  /* Socket fd to the pgserver */
extern s16 _pgrequestid;             /* Request ID to detect errors */
extern s16 _pgdefault_rship;         /* Default relationship and widget */
extern pghandle _pgdefault_widget;        /*    when 0 is used */
extern unsigned char _pgeventloop_on;  /* Boolean - is event loop running? */
extern unsigned char _pgreqbuffer[PG_REQBUFSIZE];  /* Buffer of request packets */
extern s16 _pgreqbuffer_size;        /* # of bytes in reqbuffer */
extern s16 _pgreqbuffer_count;       /* # of packets in reqbuffer */
extern s16 _pgreqbuffer_lasttype;    /* Type of last packet, indication of what return
					* packet should be sent */
extern void (*_pgerrhandler)(u16 errortype,const char *msg); /* Error handler */
extern struct _pghandlernode *_pghandlerlist;  /* List of pgBind event handlers */

extern struct timeval _pgidle_period;  /* Period before calling idle handler */
extern pgidlehandler _pgidle_handler;  /* Idle handler */
extern unsigned char _pgidle_lock;     /* Already in idle handler? */
extern char *_pg_appname;             /* Name of the app's binary */
extern pgselecthandler _pgselect_handler;   /* Normally a pointer to select() */
extern struct _pg_return_type _pg_return; /* Response from _pg_flushpackets */

/* If this is nonzero, the application should be created in this container
 * instead of a new app in pgRegisterApp
 */
extern pghandle _pg_appletbox;

#define clienterr(msg)        (*_pgerrhandler)(PG_ERRT_CLIENT,msg)

/**** Internal functions (netcore.c) */

/* IO wrappers.  On error, they return nonzero and call clienterr() */
int _pg_send(void *data,u32 datasize);
int _pg_recv(void *data,u32 datasize);

/* Wait for a new event, recieves the type code. This is used 
 * when an idle handler or other interruption is needed */
int _pg_recvtimeout(s16 *rsptype);

/* Malloc wrapper. Reports errors */
void *_pg_malloc(size_t size);

/* Default error handler (this should never be called directly) */
void _pg_defaulterr(u16 errortype,const char *msg);

/* Put a request into the queue */
void _pg_add_request(s16 reqtype,void *data,u32 datasize);

/* Receive a response packet and store its contents in _pg_return
 * (handling errors if necessary)
 *
 * If 'eventloop' is nonzero, this is waiting for a response from the
 * 'wait' packet and it's ok to use _pg_recvtimeout to process client-defined
 * things.
 */
void _pg_getresponse(int eventwait);

/* Get rid of a pgmemdata structure when done with it */
void _pg_free_memdata(struct pgmemdata memdat);

/* Format a message in a dynamically allocated buffer */
char * _pg_dynformat(const char *fmt,va_list ap);

/* Idle handler */
void _pg_idle(void);

/**** Platform-dependant functions (platform.c) */

#ifdef UCLINUX
  int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
#endif

#endif /* _CLIENTLIB_H */

/* The End */
