/*
 * request.h - this connection is for sending requests to the server
 *             and passing return values back to the client
 * $Revision: 1.1 $ 
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#ifndef _H_REQUEST
#define _H_REQUEST

#include <g_error.h>
#include <divtree.h>

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
struct uipkt_response {
  unsigned short id;
  unsigned short errt;
  unsigned long data;
  char msg[80];
};
struct uipkt_hello {
  unsigned long  magic; /* These 2 from this file */
  unsigned short protover;
  unsigned short width;  /* These 4 values from hardware.h */
  unsigned short height;
  unsigned short bpp;
  char title[50];
};

g_error req_init(struct dtstack *m_dts);
void req_free(void);
int reqproc(void);

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
#define RQH_PING     0      /* Simply returns if server is ok |   none  */
#define RQH_UPDATE   1      /* Call update()                  |   none  */
#define RQH_MKWIDGET 2      /* Makes a widget, returns handle |  struct */
#define RQH_MKBITMAP 3      /* Makes a bitmap, returns handle |  struct */
#define RQH_MKFONT   4      /* Makes a fontdesc, ret's handle |  struct */
#define RQH_MKSTRING 5      /* Makes a string, returns handle |  chars  */
#define RQH_FREE     6      /* Frees a handle                 |  struct */
#define RQH_SET      7      /* Set a widget param             |  struct */
#define RQH_GET      8      /* Get a widget param, return it  |  struct */

#define RQH_UNDEF    9      /* types > this will be truncated. return error */

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

#endif /* __H_REQUEST */
/* The End */








