/* $Id: request.c,v 1.17 2000/06/08 00:15:57 micahjd Exp $
 *
 * request.c - Sends and receives request packets. dispatch.c actually
 *             processes packets once they are received.
 *             Welcome to the UNIXy side of PicoGUI...
 *             strace is your friend!
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

#include <pgnet.h>

/* Socket */
int s = 0;

/* File descriptors of all open connections */
fd_set con;
int    con_n;

/* The connections waiting for an event */
fd_set evtwait;

/* Linked list of connection buffers */
struct conbuf *conbufs = NULL;

/************* Functions used only in this file **/
/* Stuff that actually does the work, and gets called by the select loop */

/* Close a connection and clean up */
void closefd(int fd) {
  struct conbuf *p,*condemn=NULL;

#ifdef DEBUG 
  printf("Close. fd = %d\n",fd);
#endif
  handle_cleanup(fd);
  close(fd);
  FD_CLR(fd,&con);

  /* Free the connection buffers */
  if (conbufs->owner==fd) {
    condemn = conbufs;
    conbufs = conbufs->next;
  }
  else {
    p = conbufs;
    while (p->next) {
      if (p->next->owner==fd) {
	condemn = p->next;
	p->next = p->next->next;
	break;
      }
      else
	p = p->next;
    }
  }
  if (condemn) {
    if (condemn->data_dyn)
      g_free(condemn->data_dyn);
    g_free(condemn);
  }

  if (!in_shutdown)
    update();
}

void newfd(int fd) {
  struct uipkt_hello hi;
  struct conbuf *mybuf;
  memset(&hi,0,sizeof(hi));

  /* Allocate connection buffers */
  if (prerror(g_malloc((void **)&mybuf,sizeof(struct conbuf)))
      .type != ERRT_NONE) {
    closefd(fd);
    return;
  }
  memset(mybuf,0,sizeof(struct conbuf));
  mybuf->owner = fd;
  mybuf->in = mybuf->out = mybuf->q;
  mybuf->next = conbufs;
  conbufs = mybuf;

  /* Say Hi!  Send a structure with information about the server */
  hi.magic = htonl(REQUEST_MAGIC);
  hi.protover = htons(PROTOCOL_VER);
  hi.width = htons(HWR_WIDTH);
  hi.height = htons(HWR_HEIGHT);
  hi.bpp = htons(HWR_BPP);
  strcpy(hi.title,HWR);
  if (send_response(fd,&hi,sizeof(hi))) closefd(fd);
}

void readfd(int from) {
  struct conbuf *buf = find_conbuf(from);
  int r;
  
  /* Do we have a header yet? */
  if (buf->header_size < sizeof(buf->req)) {
    errno = 0;
    r = recv(from,((unsigned char*)(&buf->req))+buf->header_size,
	     sizeof(buf->req)-buf->header_size,0);
#ifdef DEBUG
    printf("recv header = %d (have %d out of %d)\n",r,buf->header_size,
	   sizeof(buf->req));
#endif
    if (r<=0) {
      if (errno!=EAGAIN)
	/* Something bad happened, (the client probably disconnected)
	   close it down */
	closefd(from);
      return;
    }
    else {
      buf->header_size += r;
    }
  }
  
  /* Yep, get data */
  if (buf->header_size >= sizeof(buf->req)) {
    /* need prep to receive data? */
    if (!buf->data) {
      /* Reorder the bytes in the header */
      buf->req.type = ntohs(buf->req.type);
      buf->req.id = ntohs(buf->req.id);
      buf->req.size = ntohl(buf->req.size);

#ifdef DEBUG
      printf("prep data (type %u, #%u, %lu bytes)\n",buf->req.type,
	     buf->req.id,buf->req.size);
#endif
      /* Will the data fit in the static buffer? */
      if (buf->req.size < PKTBUF_LEN) {
	buf->data = buf->data_stat;
      }
      else {
	/* Allocate and use a dynamic buffer */
	if (prerror(g_malloc((void**)&buf->data_dyn,buf->req.size+1))
	    .type!=ERRT_NONE) {
	  /* Oops, the client asked for too much memory!
	     Bad client!
	     Make them disappear.
	  */
	  closefd(from);
	  return;
	}
#ifdef DEBUG
	printf("Using a dynamic packet buffer\n");
#endif
	buf->data = buf->data_dyn;
      }
      
      /* Null-terminate it. Only YOU can prevent runaway strings! */
      buf->data[buf->req.size] = 0;
    }

    if (buf->data_size < buf->req.size) {
      /* NOW we can get the packet data */
      
      errno = 0;
      r = recv(from,buf->data+buf->data_size,
	       buf->req.size-buf->data_size,0);      
#ifdef DEBUG
      printf("recv data = %d\n",r);
#endif
      if (r<=0) {
	if (errno!=EAGAIN)
	  /* Something bad happened, (the client probably disconnected)
	     close it down */
	  closefd(from);
	return;
      }
      else {
	buf->data_size += r;
      }
    }

    /* Are we there yet? */
    if (buf->data_size >= buf->req.size) {
      /* Yahoo! */
      if (dispatch_packet(from,&buf->req,buf->data))
	closefd(from);
      else {
	/* Reset the structure for another packet */
	g_free(buf->data_dyn);
	buf->data_dyn = NULL;
	buf->data_size = buf->header_size = 0;
	buf->data = NULL;
      }
    }   
  }
}

/****************** 'public' functions */

/* Retrieves the buffers associated with a connection */
struct conbuf *find_conbuf(int fd) {
  struct conbuf *p=conbufs;
  while (p) {
    if (p->owner==fd)
      return p;
    p = p->next;
  }
  return NULL;
}

/* Like send, but with some error checking stuff.  Returns nonzero
   on error.
*/
int send_response(int to,const void *data,size_t len) {
  if (send(to,data,len,0)!=len) {
#ifdef DEBUG
    printf("Error in send()\n");
#endif
    closefd(to);
    return 1;
  }
  return 0;
}

/* Bind the socket and start listening */
g_error req_init(void) {
  struct sockaddr_in server_sockaddr;
  volatile int true = 1;
#ifdef WINDOWS
  WSADATA wsad;
#endif

  if (s) return;

#ifndef WINDOWS
  signal(SIGCHLD, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
#endif

#ifdef WINDOWS    /* Windows - compatible enough with BSD sockets that
		     programmers would accept WinSock, but incompatible enough
		     to break everything.  Another Microsoft 'innovation' */
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
    return mkerror(ERRT_NETWORK,
	   "Error in bind() - Is there already a PicoGUI server running?");

  if(listen(s, REQUEST_BACKLOG) == -1)
    return mkerror(ERRT_NETWORK,"Error in listen()");

  con_n = s+1;
  FD_ZERO(&con);
  FD_ZERO(&evtwait);

  return sucess;
}

void req_free(void) {
  int i;
  struct conbuf *p,*condemn=NULL;

  /* Don't clean up handles yet, handle cleanup will be done sometime
     in the shutdown process... */
  if (!s) return;
  for (i=0;i<con_n;i++)
    if (FD_ISSET(i,&con)) close(i);
  close(s);
  s = 0;

  /* Free the list! */
  p = conbufs;
  while (p) {
    condemn = p;
    p = p->next;
    g_free(condemn->data_dyn);
    g_free(condemn);
  }

  #ifdef WINDOWS
  WSACleanup();
  #endif
}

/* Yay, a big select loop! */
int reqproc(void) {
  int fd;
  int len;
  struct sockaddr_in ec;
  int i;
  int argh=1;
  fd_set rfds;
  struct timeval tv;
  
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

     C? Portable?  Hah!
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

      /* Make it non-blocking */
      ioctl(fd,FIONBIO,&argh);
      
      /* Save it for later */
      FD_SET(fd,&con);
      if ((fd+1)>con_n) con_n = fd+1;

#ifdef DEBUG
      printf("Accepted. fd = %d, con_n = %d\n",fd,con_n);
#endif
      
      newfd(fd);

      return 1;  /* Proceed */
    }
    else {
      /* An existing connection needs attention */
      for (fd=0;fd<con_n;fd++) {
	if (FD_ISSET(fd,&rfds) && FD_ISSET(fd,&con)) {
	  
	  /* Well, we're not waiting now! */
	  FD_CLR(fd,&evtwait);
	  
#ifdef DEBUG
	  printf("Incoming. fd = %d\n",fd);
#endif
	  
	  readfd(fd);
	  return 1;
	}
      }
    }
  }
  else if (i==0) {
    /* No activity before the timeout, just try again */
    return 1;
  }
  else {
    /* Error */
    return 0;
  }
}

/* The End */

