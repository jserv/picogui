/* $Id: pgnet.h,v 1.8 2001/01/10 12:12:31 micahjd Exp $
 *
 * pgnet.h - definitions and stuff for the picogui server
 *           networking code. Most of the interesting code
 *           is needed by the client and the server, and is
 *           in picogui/network.h
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
#define WINDOWS
#ifdef WINDOWS
#include <winsock2.h>
#define EAGAIN WSAEWOULDBLOCK
#define ioctl ioctlsocket
#endif
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
#include <asm/types.h>
#endif

/* Which clients are waiting for events */
extern fd_set evtwait;

#define REQUEST_BACKLOG 10  /* Should be high enough? */

/********* Functions provided by dispatch.c */

int dispatch_packet(int from,struct pgrequest *req,void *data);

/********* Functions provided by request.c */

g_error net_init(void);   /* Subsystem init and shutdown for network */
void net_release(void);

void net_iteration(void); /* This is called in a loop as long
			     as PicoGUI is running */

void post_event(int event,struct widget *from,long param,int owner,char *data);
int send_response(int to,const void *data,size_t len);

/********* Buffers needed by each connection (packet and event) */

#define EVENTQ_LEN 16   /* Number of events that can be backlogged */
#define PKTBUF_LEN 64   /* Should be large enough for a typical packet */

/* One event */
struct event {
  int event;
  handle from;
  long param;
  char *data;
};

/* A connection buffer node */
struct conbuf {
  int owner;

  int context;   /* The owner's current context */
  
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
  unsigned long data_size; 

  /* Amount of the header received.  When this equals sizeof(req), data
     can be received */
  unsigned int header_size;

  /* If the prep work for receiving data is done, this is either
     equal to data_stat or data_dyn.  NULL if the prep hasn't been done yet
  */
  unsigned char *data;

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

/* All incoming packets are passed to a reqhandler
   owner, req, and data are from the incoming packet.
   The returned error code and 'ret' are used to
   synthesize a return packet.  Normally g_errors are
   just passed back to the client.  If it is a serious
   error that requires the connection to be closed
   (or if the client requested a connection close!)
   fatal can be set to one.
*/
/* Make a declaration for a handler */
#define DEF_REQHANDLER(n) g_error rqh_##n(int owner, struct pgrequest *req, void *data, unsigned long *ret, int *fatal);
/* Make a handler table entry */
#define TAB_REQHANDLER(n) &rqh_##n ,

/* Request handler table */
extern g_error (*rqhtab[])(int,struct pgrequest*,void*,unsigned long*,int*);

/* Nonzero when the main program is waiting for network/user input 
   in a select() call */
extern unsigned char req_in_select;

#endif /* __H_PGNET */
/* The End */








