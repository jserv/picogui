/* $Id$
 *
 * pgnet.h - Server-side networking interface common to all transports
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

#ifndef _H_PGNET
#define _H_PGNET

#include <picogui/network.h>

#include <pgserver/g_error.h>
#include <pgserver/divtree.h>
#include <pgserver/video.h>
#include <pgserver/g_malloc.h>
#include <pgserver/handle.h>
#include <pgserver/widget.h>
#include <pgserver/appmgr.h>
#include <pgserver/widget.h>

#if defined(__WIN32__) || defined(WIN32)
#ifndef WINDOWS
#define WINDOWS
#endif
#ifdef __MINGW32__
#ifdef DBG
#undef DBG
#endif
#define DBG 0
#include <winsock2.h>
#undef DBG
#endif 
#define EAGAIN WSAEWOULDBLOCK
#define ioctl ioctlsocket
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#endif

/* Which clients are waiting for events */
extern fd_set evtwait;

#define REQUEST_BACKLOG 10  /* Should be high enough? */


/********* Functions provided by request.c */

g_error net_init(void);   /* Subsystem init and shutdown for network */
void net_release(void);

void net_iteration(void); /* This is called in a loop as long
			     as PicoGUI is running */

void post_event(int event,struct widget *from,s32 param,int owner,const char *data);

/* Post an event to every client */
void post_event_global(int event, struct widget *from, s32 param, char *data);

int send_response(int to,const void *data,size_t len);

/********* Buffers needed by each connection (packet and event) */

#define EVENTQ_LEN 32   /* Number of events that can be backlogged */
#define PKTBUF_LEN 513  /* Matches the size of the client's buffer */

/* One event */
struct event {
  int event;
  handle from;
  s32 param;
  char *data;
};

/* A connection buffer node */
struct conbuf {
  int owner;

  int context;   /* The owner's current context */
  handle lastevent_from;   /* Originator of the last event sent to this connection,
			    * currently used for the PG_POPUP_ATEVENT flag */

  /* Event ring buffer */
  struct event q[EVENTQ_LEN];
  struct event *in,*out;

  /* Request header */
  struct pgrequest req;

  /* If non-null, the packet was larger than PKTBUF_LEN and this is a
     dynamically allocated data buffer
  */
  unsigned char *data_dyn;

  /* Static data buffer */
  unsigned char data_stat[PKTBUF_LEN];

  /* The amount of data received.  When this equals req.size, the packet
     is ready for dispatching
  */
  u32 data_size; 

  /* Amount of the header received.  When this equals sizeof(req), data
     can be received */
  unsigned int header_size;

  /* If the prep work for receiving data is done, this is either
   * equal to data_stat or data_dyn.  NULL if the prep hasn't been done yet.
   */
  unsigned char *data;

  /* This flag is set if the data buffer really hasn't been allocated, and the
   * network code should discard the incoming packet and send an error afterwards.
   */
  unsigned int no_buffer : 1;

  struct conbuf *next;
};

/* Linked list of connection buffers */
extern struct conbuf *conbufs;

/* Retrieves the buffers associated with a connection */
struct conbuf *find_conbuf(int fd);

/* Gets the next pending event for 'owner' and if 'remove' is nonzero
   takes it out of the queue
*/
struct event *get_event(int owner,int remove);

/* Returns the number of pending events for a particular connection */
int check_event(int owner);

/* Nonzero when the main program is waiting for network/user input 
   in a select() call */
extern volatile unsigned char req_in_select;

#endif /* __H_PGNET */
/* The End */








