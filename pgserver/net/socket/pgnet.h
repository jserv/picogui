/* $Id: pgnet.h,v 1.10 2000/08/01 18:11:27 micahjd Exp $
 *
 * pgnet.h - header for all PicoGUI networking stuff (request/packet/event...)
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
 * Contributors:
 * 
 * 
 * 
 */

#ifndef _H_PGNET
#define _H_PGNET

#include <g_error.h>
#include <divtree.h>
#include <video.h>
#include <g_malloc.h>
#include <handle.h>
#include <widget.h>
#include <appmgr.h>
#include <widget.h>
#include <theme.h>

#if defined(__WIN32__) || defined(WIN32)
#define WINDOWS
#include <windows.h>
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

extern volatile int in_shutdown;

/* Which clients are waiting for events */
extern fd_set evtwait;

#define REQUEST_PORT    30450
#define PROTOCOL_VER    0x0001
#define REQUEST_MAGIC   0x31415926
#define REQUEST_BACKLOG 10  /* Should be high enough? */

/* Packet structures */
struct uipkt_request {
  unsigned short type;
  unsigned short id;  /* Just to make sure requests match up with responses */
  unsigned long size; /* The request is followed by size bytes of data */
};  
#define MAX_RESPONSE_SZ 12  /* in bytes */
#define RESPONSE_ERR 1
struct response_err {
  unsigned short type;    /* RESPONSE_ERR - error code */
  unsigned short id;
  unsigned short errt;
  unsigned short msglen;  /* Length of following message */
};
#define RESPONSE_RET 2
struct response_ret {
    unsigned short type;    /* RESPONSE_RET - return value */
    unsigned short id;
    unsigned long data;
};
#define RESPONSE_EVENT 3
struct response_event {
    unsigned short type;    /* RESPONSE_EVENT */
    unsigned short event;
    unsigned long from;
    unsigned long param;
};
struct uipkt_hello {
  unsigned long  magic; /* These 2 from this file */
  unsigned short protover;
  unsigned short width;  /* These 4 values from hardware.h */
  unsigned short height;
  unsigned short bpp;
  char title[50];
};

/********* Functions provided by dispatch.c */

int dispatch_packet(int from,struct uipkt_request *req,void *data);

/********* Functions provided by request.c */

g_error req_init(void);
void req_free(void);
int reqproc(void);
void post_event(int event,struct widget *from,long param,int owner);
int send_response(int to,const void *data,size_t len);

/********* Buffers needed by each connection (packet and event) */

#define EVENTQ_LEN 10   /* Number of events that can be backlogged */
#define PKTBUF_LEN 64   /* Should be large enough for a typical packet */

/* One event */
struct event {
  int event;
  handle from;
  long param;
};

/* A connection buffer node */
struct conbuf {
  int owner;

  int context;   /* The owner's current context */
  
  /* Event ring buffer */
  struct event q[EVENTQ_LEN];
  struct event *in,*out;

  /* Request header */
  struct uipkt_request req;

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
#define DEF_REQHANDLER(n) g_error rqh_##n(int owner, struct uipkt_request *req, void *data, unsigned long *ret, int *fatal);
/* Make a handler table entry */
#define TAB_REQHANDLER(n) &rqh_##n ,

/* Constants for request handlers                                 args  */
#define RQH_PING      0      /* Simply returns if server is ok |   none  */
#define RQH_UPDATE    1      /* Call update()                  |   none  */
#define RQH_MKWIDGET  2      /* Makes a widget, returns handle |  struct */
#define RQH_MKBITMAP  3      /* Makes a bitmap, returns handle |  struct */
#define RQH_MKFONT    4      /* Makes a fontdesc, ret's handle |  struct */
#define RQH_MKSTRING  5      /* Makes a string, returns handle |  chars  */
#define RQH_FREE      6      /* Frees a handle                 |  struct */
#define RQH_SET       7      /* Set a widget param             |  struct */
#define RQH_GET       8      /* Get a widget param, return it  |  struct */
#define RQH_SETBG     9      /* bequeath a new background bmp  |  struct */
#define RQH_IN_KEY    10     /* Dispatch keyboard input        |  struct */
#define RQH_IN_POINT  11     /* Dispatch pointing device input |  struct */
#define RQH_IN_DIRECT 12     /* Dispatch direct input          |  struct */
#define RQH_WAIT      13     /* Wait for an event              |  none   */
#define RQH_THEMESET  14     /* Set an element in the theme    |  struct */
#define RQH_REGISTER  15     /* Register a new application     |  struct */
#define RQH_MKPOPUP   16     /* Create a popup root widget     |  struct */
#define RQH_SIZETEXT  17     /* Find the size of text          |  struct */
#define RQH_BATCH     18     /* Executes many requests         |  requests */
#define RQH_GRABKBD   19     /* Become the keyboard owner      |  none */
#define RQH_GRABPNTR  20     /* Own the pointing device        |  none */
#define RQH_GIVEKBD   21     /* Give the keyboard back         |  none */
#define RQH_GIVEPNTR  22     /* Give the pointing device back  |  none */
#define RQH_MKCONTEXT 23     /* Enters a new context           |  none */
#define RQH_RMCONTEXT 24     /* Cleans up and kills the context|  none */
#define RQH_FOCUS     25     /* Force focus to specified widget|  struct */

#define RQH_UNDEF     26     /* types > this will be truncated. return error */

/* Structures passed to request handlers as 'data'.
 * Dummy variables pad it to a multiple of 4 bytes (compiler likes it?)
 */
struct rqhd_mkwidget {
  unsigned short rship;
  unsigned short type;
  unsigned long parent;
};
struct rqhd_free {
  unsigned long h;
};
struct rqhd_mkbitmap {
  unsigned short w;       /* If these are 0, the following data is a */
  unsigned short h;       /* pnm bitmap.  Otherwise, these are the dimensions
			     of xbm data following it. */
  unsigned long fg;       /* Foreground and background colors if this is a */
  unsigned long bg;       /* xbm bitmap. */
};
struct rqhd_mkfont {
  char name[40];
  unsigned long style;
  unsigned short size;
  unsigned short dummy;
};
struct rqhd_set {
  unsigned long widget;
  unsigned long glob;
  unsigned short property;
  unsigned short dummy;
};
struct rqhd_get {
  unsigned long widget;
  unsigned short property;
  unsigned short dummy;
};
struct rqhd_setbg {
  unsigned long h;   /* 0 to restore original */
};
struct rqhd_in_key {
  unsigned long type;   /* A TRIGGER_* constant */
  unsigned short key;
  unsigned short mods;
};
struct rqhd_in_point {
  unsigned long type;   /* A TRIGGER_* constant */
  unsigned short x;
  unsigned short y;
  unsigned short btn;  /* button bitmask */
  unsigned short dummy;
};
struct rqhd_in_direct {
  unsigned long param;   /* The arbitrary parameter */
  /* The rest of the packet is read as a string */
};
struct rqhd_themeset {
  unsigned long value;
  unsigned short element;
  unsigned short state;
  unsigned short param;
  unsigned short dummy;
};
struct rqhd_register {
  /* This is just a subset of app_info, organized for network
     transmission */

  unsigned long name;
  unsigned short type;
  unsigned short side;
  unsigned short sidemask;
  unsigned short w;
  unsigned short h;
  unsigned short minw;
  unsigned short maxw;
  unsigned short minh;
  unsigned short maxh;
  unsigned short dummy;
};
struct rqhd_mkpopup {
  unsigned short x;
  unsigned short y;
  unsigned short w;
  unsigned short h;
};
struct rqhd_sizetext {
  unsigned long text;
  unsigned long font;
};
struct rqhd_focus {
  unsigned long h;
};

#endif /* __H_PGNET */
/* The End */








