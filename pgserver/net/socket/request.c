/* $Id: request.c,v 1.10 2000/06/01 23:44:41 micahjd Exp $
 *
 * request.c - this connection is for sending requests to the server
 *             and passing return values back to the client
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

#include <request.h>
#include <video.h>
#include <g_malloc.h>
#include <handle.h>
#include <widget.h>
#include <appmgr.h>
#include <widget.h>
#include <theme.h>

/* #define NONBLOCKING */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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
DEF_REQHANDLER(setbg)
DEF_REQHANDLER(in_key)
DEF_REQHANDLER(in_point)
DEF_REQHANDLER(in_direct)
DEF_REQHANDLER(wait)
DEF_REQHANDLER(themeset)
DEF_REQHANDLER(register)
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
  TAB_REQHANDLER(setbg)
  TAB_REQHANDLER(in_key)
  TAB_REQHANDLER(in_point)
  TAB_REQHANDLER(in_direct)
  TAB_REQHANDLER(wait)
  TAB_REQHANDLER(themeset)
  TAB_REQHANDLER(register)
  TAB_REQHANDLER(undef)
};

/* Socket */
int s = 0;

/* Dt-stack */
struct dtstack *dts;

/* File descriptors of all open connections */
fd_set con;
int    con_n;

/* The connections waiting for an event */
fd_set evtwait;

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
#ifdef WINDOWS
  WSADATA wsad;
#endif

  if (s) return;

  dts = m_dts;

#ifndef WINDOWS
  signal(SIGCHLD, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
#endif

#ifdef WINDOWS
  if (WSAStartup(MAKEWORD(2,0),&wsad))
    return mkerror(ERRT_NETWORK,"Error in WSAStartup()");
#endif

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
  FD_ZERO(&evtwait);

  return sucess;
}

void req_free(void) {
  int i;

  if (!s) return;
  for (i=0;i<con_n;i++)
    if (FD_ISSET(i,&con)) close(i);
  close(s);
  s = 0;

  #ifdef WINDOWS
  WSACleanup();
  #endif
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
  struct response_err rsp_err;
  struct response_ret rsp_ret;
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

  /* In linux the timeout is a just-in-case for errors.  In windows we
     have to wake from our little select() coma often to pump the message
     loop or windows will get unresponsive.  That's what MS gets for
     not putting the GUI in a seperate task.
     Instead of putting the input in a seperate thread, we have to poll
     both the input and the network, sucking up CPU.
  */
#ifdef WINDOWS
  tv.tv_sec = 0;
  tv.tv_usec = 0;
#else
  tv.tv_sec = 5;
  tv.tv_usec = 0;
#endif

  i = select(con_n,&rfds,NULL,NULL,&tv);
#ifdef DEBUG
  printf("select() = %d\n",i);
#endif

  if (i>0) {
    /* Something is active */

    if (FD_ISSET(s,&rfds)) {
      /* New connection */
      struct uipkt_hello hi;
      memset(&hi,0,sizeof(hi));

      len = sizeof(struct sockaddr_in);
      if((fd = accept(s, (void *)&ec, &len)) == -1) {
#ifdef DEBUG
	printf("accept() returned -1\n");
#ifdef WINDOWS
	printf("WSAGetLastError() = %d\n",WSAGetLastError());
#endif
#endif
	return 0;
      }

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
      send(fd,&hi,sizeof(hi),0); /* Say Hi! */

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

	  /* Well, we're not waiting now! */
	  FD_CLR(fd,&evtwait);

#ifdef DEBUG
	  printf("Incoming. fd = %d\n",fd);
#endif

	  /* Attempt reading the packet header */
	  remaining = sizeof(req);
	  pd = (unsigned char *) &req;
	  while (remaining) {
	    de = recv(fd,pd,remaining,0);
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
	    if (prerror(g_malloc((void **) &data, req.size+1)
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
	      de = recv(fd,pd,remaining,0);
	      if (de<=0) {
		/* Connection close? */
		g_free(data);
		closefd(fd);
		return 1;
	      }	  
	      pd += de;
	      remaining -= de;
	    }
	    /* Tack on a null terminator */
	    ((char *)data)[req.size] = 0;
	  }
	  else
	    data = NULL;

	  /* Call the request handler, and send a response packet */
	  if (req.type>RQH_UNDEF) req.type = RQH_UNDEF;
	  fatal = 0;

	  rsp_ret.data = 0;
	  e = (*rqhtab[req.type])(fd,&req,data,&rsp_ret.data,&fatal);
	  g_free(data);

	  /* Send an error packet if there was an error */
	  if (e.type != ERRT_NONE) {
	    int errlen;
	    errlen = strlen(e.msg);

	    rsp_err.type = htons(RESPONSE_ERR);
	    rsp_err.id = req.id;
	    rsp_err.errt = htons(e.type);
	    rsp_err.msglen = htons(errlen);

	    /* Send the error */
	    if (send(fd,&rsp_err,sizeof(rsp_err),0)<sizeof(rsp_err))   
	      fatal = 1;
	    if (send(fd,e.msg,errlen,0)<errlen)   
	      fatal = 1;
	  }
	  else if (req.type != RQH_WAIT) { /*WAIT packet gets no response yet*/
	    /* Send a normal response packet */

	    rsp_ret.type = htons(RESPONSE_RET);
	    rsp_ret.id = req.id;
	    rsp_ret.data = htonl(rsp_ret.data);

	    /* Send the return packet */
	    if (send(fd,&rsp_ret,sizeof(rsp_ret),0)<sizeof(rsp_ret))   
	      fatal = 1;
	  }

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
  if (!parent) return mkerror(ERRT_BADPARAM,"NULL parent widget");

  /* Don't let an app put stuff outside its root widget */
  if (owner>=0 && parent->isroot && ntohs(arg->rship)!=DERIVE_INSIDE)
    return mkerror(ERRT_BADPARAM,
		   "App attempted to derive before or after a root widget");

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

g_error rqh_setbg(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_setbg *arg = (struct rqhd_setbg *) data;
  if (req->size < sizeof(struct rqhd_setbg)) 
    return mkerror(ERRT_BADPARAM,"rqhd_setbg too small");
  
  return appmgr_setbg(owner,ntohl(arg->h));
}

g_error rqh_undef(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  return mkerror(ERRT_BADPARAM,"Undefined request type");
}

g_error rqh_in_key(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_in_key *arg = (struct rqhd_in_key *) data;
  if (req->size < sizeof(struct rqhd_in_key)) 
    return mkerror(ERRT_BADPARAM,"rqhd_in_key too small");
  dispatch_key(ntohl(arg->type),(int) ntohl(arg->key));
  return sucess;
}

g_error rqh_in_point(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct rqhd_in_point *arg = (struct rqhd_in_point *) data;
  if (req->size < sizeof(struct rqhd_in_point)) 
    return mkerror(ERRT_BADPARAM,"rqhd_in_point too small");
  dispatch_pointing(ntohl(arg->type),ntohs(arg->x),ntohs(arg->y),
		    ntohs(arg->btn));
  return sucess;
}

g_error rqh_themeset(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct rqhd_themeset *arg = (struct rqhd_themeset *) data;
  if (req->size < sizeof(struct rqhd_themeset)) 
    return mkerror(ERRT_BADPARAM,"rqhd_themeset too small");

  /* Don't worry about errors here.  If they try to set a nonexistant
     theme, its no big deal.  Just means that the theme is a later
     version than this widget set.
  */
  
  themeset(ntohs(arg->element),ntohs(arg->state),ntohs(arg->param),
	   ntohl(arg->value));

  /* Do a global recalc (Yikes!) */
  dts->top->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
  dts->top->flags |= DIVTREE_NEED_RECALC;

  return sucess;
}

g_error rqh_in_direct(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_in_direct *arg = (struct rqhd_in_direct *) data;
  if (req->size < (sizeof(struct rqhd_in_direct)+1)) 
    return mkerror(ERRT_BADPARAM,"rqhd_in_direct too small");
  dispatch_direct(((char*)arg)+sizeof(struct rqhd_in_direct),
		  ntohl(arg->param));
  return sucess;
}

g_error rqh_wait(int owner, struct uipkt_request *req,
		 void *data, unsigned long *ret, int *fatal) {
  
  FD_SET(owner,&evtwait);
  return sucess;
}

g_error rqh_register(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct rqhd_register *arg = (struct rqhd_register *) data;
  struct app_info i;
  g_error e;
  memset(&i,0,sizeof(i));
  if (req->size < (sizeof(struct rqhd_register)+1)) 
    return mkerror(ERRT_BADPARAM,"rqhd_register too small");

  i.owner = owner;
  i.name = ntohl(arg->name);
  i.type = ntohs(arg->type);
  i.side = ntohs(arg->side);
  i.sidemask = ntohs(arg->sidemask);
  i.w = ntohs(arg->w);
  i.h = ntohs(arg->h);
  i.minw = ntohs(arg->minw);
  i.maxw = ntohs(arg->maxw);
  i.minh = ntohs(arg->minh);
  i.maxh = ntohs(arg->maxh);
 
  e = appmgr_register(&i);

  *ret = i.rootw;

  return e;
}

/* The End */








