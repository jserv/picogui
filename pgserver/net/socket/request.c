/*
 * request.c - this connection is for sending requests to the server
 *             and passing return values back to the client
 * $Revision: 1.1 $
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <request.h>
#include <video.h>
#include <g_malloc.h>
#include <handle.h>
#include <widget.h>

/* #define NONBLOCKING */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

/* Table of request handlers */
DEF_REQHANDLER(ping)
DEF_REQHANDLER(update)
DEF_REQHANDLER(mkwidget)
DEF_REQHANDLER(mkbitmap)
DEF_REQHANDLER(mkfont)
DEF_REQHANDLER(mkstring)
DEF_REQHANDLER(free)
DEF_REQHANDLER(set)
DEF_REQHANDLER(get)
DEF_REQHANDLER(undef)
g_error (*rqhtab[])(int,struct uipkt_request*,void*,unsigned long*,int*) = {
  TAB_REQHANDLER(ping)
  TAB_REQHANDLER(update)
  TAB_REQHANDLER(mkwidget)
  TAB_REQHANDLER(mkbitmap)
  TAB_REQHANDLER(mkfont)
  TAB_REQHANDLER(mkstring)
  TAB_REQHANDLER(free)
  TAB_REQHANDLER(set)
  TAB_REQHANDLER(get)
  TAB_REQHANDLER(undef)
};

/* Socket */
int s = 0;

/* Dt-stack */
struct dtstack *dts;

/* File descriptors of all open connections */
fd_set con;
int    con_n;

void closefd(int fd) {
#ifdef DEBUG 
  printf("Close. fd = %d\n",fd);
#endif
  handle_cleanup(fd);
  close(fd);
  FD_CLR(fd,&con);
  update(dts);
}

/* Bind the socket and start listening */
g_error req_init(struct dtstack *m_dts) {
  struct sockaddr_in server_sockaddr;
  volatile int true = 1;

  if (s) return;

  dts = m_dts;

  signal(SIGCHLD, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    return mkerror(ERRT_NETWORK,"Error in socket()");
  
  if((setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&true, 
     sizeof(true))) == -1)
    return mkerror(ERRT_NETWORK,"Error in setsockopt()");

  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(REQUEST_PORT);
  server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(s, (struct sockaddr *)&server_sockaddr, 
     sizeof(server_sockaddr)) == -1)
    return mkerror(ERRT_NETWORK,"Error in bind()");

  if(listen(s, REQUEST_BACKLOG) == -1)
    return mkerror(ERRT_NETWORK,"Error in listen()");

  con_n = s+1;
  FD_ZERO(&con);

  return sucess;
}

void req_free(void) {
  int i;

  if (!s) return;
  for (i=0;i<con_n;i++)
    if (FD_ISSET(i,&con)) close(i);
  close(s);
  s = 0;
}

int reqproc(void) {
  int fd;
  int len;
  struct sockaddr_in ec;
  int i;
#ifdef NONBLOCKING
  int argh=1;
#endif
  fd_set rfds;
  struct timeval tv;
  char c;
  struct uipkt_request req;
  struct uipkt_response rsp;
  void *data;
  g_error e;
  int fatal;
  long remaining,de;
  unsigned char *pd;
  
  /* Get ready to select() the socket itself and all open connections */
  FD_ZERO(&rfds);
  FD_SET(s,&rfds);
  for (i=0;i<con_n;i++)     /* con stores all the active connections */
    if (FD_ISSET(i,&con)) FD_SET(i,&rfds);
  tv.tv_sec = 5;
  tv.tv_usec = 0;
  i = select(con_n,&rfds,NULL,NULL,&tv);

  if (i>0) {
    /* Something is active */

    if (FD_ISSET(s,&rfds)) {
      /* New connection */
      struct uipkt_hello hi;
      memset(&hi,0,sizeof(hi));

      if((fd = accept(s, (void *)&ec, &len)) == -1)
	return 0;

#ifdef NONBLOCKING
      /* Make it non-blocking */
      ioctl(fd,FIONBIO,&argh);
#endif
      
      /* Say Hi!  Send a structure with information about the server */
      hi.magic = htonl(REQUEST_MAGIC);
      hi.protover = htons(PROTOCOL_VER);
      hi.width = htons(HWR_WIDTH);
      hi.height = htons(HWR_HEIGHT);
      hi.bpp = htons(HWR_BPP);
      strcpy(hi.title,HWR);
      write(fd,&hi,sizeof(hi)); /* Say Hi! */

      /* Save it for later */
      FD_SET(fd,&con);
      if ((fd+1)>con_n) con_n = fd+1;
#ifdef DEBUG
      printf("Accepted. fd = %d, con_n = %d\n",fd,con_n);
#endif

      return 1;  /* Proceed */
    }
    else {
      /* An existing connection needs attention */
      for (fd=0;fd<con_n;fd++)
	if (FD_ISSET(fd,&rfds) && FD_ISSET(fd,&con)) {

#ifdef DEBUG
	  printf("Incoming. fd = %d\n",fd);
#endif

	  /* Attempt reading the packet header */
	  remaining = sizeof(req);
	  pd = (unsigned char *) &req;
	  while (remaining) {
	    de = read(fd,pd,remaining);
	    if (de<=0) {
	      /* Connection close? */
	      closefd(fd);
	      return 1;
	    }	  
	    pd += de;
	    remaining -= de;
	  }
	  
	  req.type = ntohs(req.type); 
	  req.size = ntohl(req.size);

#ifdef DEBUG
	  printf("Request. fd = %d, type = %d, id = 0x%04X, size = 0x%08X\n",
		 fd,req.type,req.id,req.size);
#endif

	  if (req.size) {
	    if (prerror(g_malloc((void **) &data, req.size)
			).type != ERRT_NONE) {
	      /* The request's size is too big. Be gone with this client,
	       * either it thinks we have more memory than we do, in which
	       * case its not much use anyway, or the packets are scrambled
	       * in which case we couldn't salvage the connection 
	       */ 
	      closefd(fd);
	      return 1;
	    }	  


	    /* Attempt reading the packet content */
	    remaining = req.size;
	    pd = data;
	    while (remaining) {
	      de = read(fd,pd,remaining);
	      if (de<=0) {
		/* Connection close? */
		g_free(data);
		closefd(fd);
		return 1;
	      }	  
	      pd += de;
	      remaining -= de;
	    }
	  }
	  else
	    data = NULL;

	  /* Call the request handler, translate the returned g_error
	   * into the error fields of the response packet */
	  if (req.type>RQH_UNDEF) req.type = RQH_UNDEF;
	  memset(&rsp,0,sizeof(rsp));
	  rsp.id = req.id;   /* This comes verbatim from the request */
	  rsp.errt = htons(ERRT_NONE);
	  fatal = 0;
	  e = (*rqhtab[req.type])(fd,&req,data,&rsp.data,&fatal);
	  g_free(data);
	  rsp.data = htonl(rsp.data);
	  if (e.type != ERRT_NONE) {
	    rsp.errt = htons(e.type);
	    strncpy(rsp.msg,e.msg,79);
	  }

#ifdef DEBUG
	  printf("Response. fd = %d, id = 0x%04X, ret = 0x%08X, errt = 0x%04X,\n          msg = '%s', \n          (%02X %02X %02X %02X %02X %02X %02X %02X) \n", fd,rsp.id,ntohl(rsp.data),ntohs(rsp.errt),rsp.msg,
		 ((unsigned char *)&rsp)[0],
		 ((unsigned char *)&rsp)[1],
		 ((unsigned char *)&rsp)[2],
		 ((unsigned char *)&rsp)[3],
		 ((unsigned char *)&rsp)[4],
		 ((unsigned char *)&rsp)[5],
		 ((unsigned char *)&rsp)[6],
		 ((unsigned char *)&rsp)[7]);
#endif

	  /* Send the response packet */
	  if (write(fd,&rsp,sizeof(rsp))<sizeof(rsp))   
	    fatal = 1;

	  if (fatal)
	    closefd(fd);
	  return 1; /* Proceed */
	}
    }
  }
  else if (i==0) {
    /* No activity within 5 seconds */
    return 1;
  }
  else {
    /* Error */
    return 0;
  }
}

/***************** Request handlers *******/

g_error rqh_ping(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  return sucess;
}

g_error rqh_update(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  update(dts);
  return sucess;
}

g_error rqh_mkwidget(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_mkwidget *arg = (struct rqhd_mkwidget *) data;
  struct widget *w,*parent;
  handle h;
  g_error e;

  if (req->size < sizeof(struct rqhd_mkwidget)) 
    return mkerror(ERRT_BADPARAM,"rqhd_mkwidget too small");

  e = rdhandle((void**) &parent,TYPE_WIDGET,owner,ntohl(arg->parent));
  if (e.type != ERRT_NONE) return e;

  if (!parent)
    /* If parent is null, create a new widget */
    e = widget_create(&w,ntohs(arg->type),dts,dts->top,&dts->top->head->next);
  else
    e = widget_derive(&w,ntohs(arg->type),parent,ntohs(arg->rship));
  
  if (e.type != ERRT_NONE) return e;

  e = mkhandle(&h,TYPE_WIDGET,owner,w);
  if (e.type != ERRT_NONE) return e;
  
  *ret = h;

  return sucess;
}

g_error rqh_mkbitmap(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_mkbitmap *arg = (struct rqhd_mkbitmap *) data;
  struct bitmap *bmp;
  unsigned char *bits;
  long bitsz;
  handle h;
  g_error e;
  int w;

  if (req->size <= sizeof(struct rqhd_mkbitmap)) 
    return mkerror(ERRT_BADPARAM,"rqhd_mkbitmap too small");

  bits = ((unsigned char *)data)+sizeof(struct rqhd_mkbitmap);
  bitsz = req->size - sizeof(struct rqhd_mkbitmap);

  if (arg->w && arg->h) {
    /* XBM */
    w = ntohs(arg->w);
    if (w%8)
      w = w/8 + 1;
    else
      w = w/8;
#ifdef DEBUG
    printf("new XBM: bitsz = %d, calculated = %d\n",bitsz,
	   (w*ntohs(arg->h)));
#endif
    if (bitsz < (w*ntohs(arg->h)))
      return mkerror(ERRT_BADPARAM,"XBM data too small");
    e = hwrbit_xbm(&bmp,bits,ntohs(arg->w),ntohs(arg->h),
		   cnvcolor(ntohl(arg->fg)),cnvcolor(ntohl(arg->bg)));
  }
  else {
    /* PNM */
    e = hwrbit_pnm(&bmp,bits,bitsz);
  }
  if (e.type != ERRT_NONE) return e;

  e = mkhandle(&h,TYPE_BITMAP,owner,bmp);
  if (e.type != ERRT_NONE) return e;
  
  *ret = h;

  return sucess;
}

g_error rqh_mkfont(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_mkfont *arg = (struct rqhd_mkfont *) data;
  handle h;
  g_error e;

  if (req->size <= sizeof(struct rqhd_mkfont)) 
    return mkerror(ERRT_BADPARAM,"rqhd_mkfont too small");

  e = findfont(&h,owner,arg->name,ntohs(arg->size),ntohl(arg->style));
  if (e.type != ERRT_NONE) return e;

  *ret = h;
  return sucess;
}

g_error rqh_mkstring(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  char *buf;
  handle h;
  g_error e;

  e = g_malloc((void **) &buf, req->size+1);
  if (e.type != ERRT_NONE) return e;
  memcpy(buf,data,req->size);
  buf[req->size] = 0;  /* Null terminate it if it isn't already */

  e = mkhandle(&h,TYPE_STRING,owner,buf);
  if (e.type != ERRT_NONE) return e;

  *ret = h;
  return sucess;
}

g_error rqh_free(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_free *arg = (struct rqhd_free *) data;
  if (req->size < sizeof(struct rqhd_free)) 
    return mkerror(ERRT_BADPARAM,"rqhd_free too small");
  
  return handle_free(owner,ntohl(arg->h));
}

g_error rqh_set(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_set *arg = (struct rqhd_set *) data;
  struct widget *w;
  g_error e;

  if (req->size < sizeof(struct rqhd_set)) 
    return mkerror(ERRT_BADPARAM,"rqhd_set too small");
  e = rdhandle((void**) &w,TYPE_WIDGET,owner,ntohl(arg->widget));
  if (e.type != ERRT_NONE) return e;

  return widget_set(w,ntohs(arg->property),ntohl(arg->glob));
}

g_error rqh_get(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_get *arg = (struct rqhd_get *) data;
  struct widget *w;
  g_error e;

  if (req->size < sizeof(struct rqhd_get)) 
    return mkerror(ERRT_BADPARAM,"rqhd_get too small");
  e = rdhandle((void**) &w,TYPE_WIDGET,owner,ntohl(arg->widget));
  if (e.type != ERRT_NONE) return e;

  *ret = widget_get(w,ntohs(arg->property));

  return sucess;
}

g_error rqh_undef(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  return mkerror(ERRT_BADPARAM,"Undefined request type");
}

/* The End */








