/* $Id: picogui_client.c,v 1.55 2001/03/07 18:26:07 pney Exp $
 *
 * picogui_client.c - C client library for PicoGUI
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors: 
 * Philippe Ney <philippe.ney@smartdata.ch>
 * Brandon Smith <lottabs2@yahoo.com>
 * 
 * 
 */

/******************* Definitions and includes */

#ifdef UCLINUX
#  include "platform.c"  /* for emulation of 'vsnprintf' */
#endif

/* System includes */
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>  /* for time_t type (used in timeval structure) */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>    /* for fprintf() */
#include <malloc.h>

#ifndef UCLINUX
#  include <alloca.h> /* for uClinux 'alloca' is in malloc.h */
#endif

#include <string.h>   /* for memcpy(), memset(), strcpy() */
#include <stdarg.h>   /* needed for pgRegisterApp and pgSetWidget */
#include <stdlib.h>   /* for getenv() */

/* PicoGUI */
#include <picogui.h>            /* Basic PicoGUI include */
#include <picogui/network.h>    /* Network interface to the server */

//#define DEBUG
//#define DEBUG_EVT

/* Default server */
#define PG_REQUEST_SERVER       "127.0.0.1"

/* Buffer size. When packets don't need to be sent immediately,
 * they accumulate in this buffer. It doesn't need to be very big
 * because most packets are quite small. Large packets like bitmaps
 * won't fit here anyway, so they are sent immediately.
 */
#define PG_REQBUFSIZE           512

/* A node in the list of event handlers set with pgBind */
struct _pghandlernode {
  pghandle widgetkey;
  short eventkey;
  pgevthandler handler;
  void *extra;
  struct _pghandlernode *next;
};

/* Global vars for the client lib */
int _pgsockfd;                  /* Socket fd to the pgserver */
short _pgrequestid;             /* Request ID to detect errors */
short _pgdefault_rship;         /* Default relationship and widget */
pghandle _pgdefault_widget;        /*    when 0 is used */
unsigned char _pgeventloop_on;  /* Boolean - is event loop running? */
unsigned char _pgreqbuffer[PG_REQBUFSIZE];  /* Buffer of request packets */
short _pgreqbuffer_size;        /* # of bytes in reqbuffer */
short _pgreqbuffer_count;       /* # of packets in reqbuffer */
void (*_pgerrhandler)(unsigned short errortype,const char *msg);
                                /* Error handler */
struct _pghandlernode *_pghandlerlist;  /* List of pgBind event handlers */

struct timeval _pgidle_period;  /* Period before calling idle handler */
pgidlehandler _pgidle_handler;  /* Idle handler */
unsigned char _pgidle_lock;     /* Already in idle handler? */
char *_pg_appname;              /* Name of the app's binary */
pgselecthandler _pgselect_handler;   /* Normally a pointer to select() */

/* Structure for a retrieved and validated response code,
   the data collected by _pg_flushpackets is stored here. */
struct {
  short type;
  union {

    /* if type == PG_RESPONSE_RET */
    unsigned long retdata;

    /* if type == PG_RESPONSE_EVENT */
    struct pgEvent event;

    /* if type == PG_RESPONSE_DATA */
    struct {
      unsigned long size;
      void *data;         /* Dynamically allocated - should be freed and
			     set to NULL when done, or it will be freed
			     next time flushpackets is called */
    } data;

  } e;  /* e for extra? ;-) */
} _pg_return;

#define clienterr(msg)        (*_pgerrhandler)(PG_ERRT_CLIENT,msg)

/**** Internal functions */

/* IO wrappers.  On error, they return nonzero and call clienterr() */
int _pg_send(void *data,unsigned long datasize);
int _pg_recv(void *data,unsigned long datasize);

/* Wait for a new event, recieves the type code. This is used 
 * when an idle handler or other interruption is needed */
int _pg_recvtimeout(short *rsptype);

/* Malloc wrapper. Reports errors */
void *_pg_malloc(size_t size);

/* Default error handler (this should never be called directly) */
void _pg_defaulterr(unsigned short errortype,const char *msg);

/* Put a request into the queue */
void _pg_add_request(short reqtype,void *data,unsigned long datasize);

/* Receive a response packet and store its contents in _pg_return
 * (handling errors if necessary)
 */
void _pg_getresponse(void);

/* Get rid of a pgmemdata structure when done with it */
void _pg_free_memdata(struct pgmemdata memdat);

/* Format a message in a dynamically allocated buffer */
char * _pg_dynformat(const char *fmt,va_list ap);

/* Idle handler */
void _pg_idle(void);

/******************* Internal functions */

/* Send data to the server, checking and reporting errors */
int _pg_send(void *data,unsigned long datasize) {
  if (send(_pgsockfd,data,datasize,0)!=datasize) {
    clienterr("send error");
    return 1;
  }
  return 0;
}

/* Receive... */
int _pg_recv(void *data,unsigned long datasize) {
  if (recv(_pgsockfd,data,datasize,0) < 0) {
    clienterr("recv error");
    return 1;
  }
  return 0;
}

/* Wait for a new event, recieves the type code. This is used 
 * when an idle handler or other interruption is needed */
int _pg_recvtimeout(short *rsptype) {
  struct timeval tv;
  fd_set readfds;
  struct pgrequest waitreq;
  struct pgrequest unwaitreq;
  char cruft[sizeof(struct pgresponse_ret) - sizeof(short)];	/* Unused return packet */
   
  /* Set up a packet to send to put us back on the waiting list */
  waitreq.type = htons(PGREQ_WAIT);
  waitreq.size = 0;
   
  /* Set up a packet to send to take us off the waiting list */
  unwaitreq.type = htons(PGREQ_PING);
  unwaitreq.size = 0;
   
  while (1) {
     FD_ZERO(&readfds);
     FD_SET(_pgsockfd,&readfds);
     tv = _pgidle_period;
   
     /* don't care about writefds and exceptfds: */
     
     if ((*_pgselect_handler)(_pgsockfd+1,&readfds,NULL,NULL,
			      (tv.tv_sec + tv.tv_usec) ? &tv : NULL) < 0)
       continue;

     /* Well, now we have something! */

     if (FD_ISSET(_pgsockfd, &readfds))   /* Got data from server */
	return _pg_recv(rsptype,sizeof(short));
	
     /* We'll need these */
     waitreq.id = ++_pgrequestid;
     unwaitreq.id = ++_pgrequestid;

     /* No event yet, but now we have a race condition. We need to tell the server
      * to take the client off the waiting list, but it's possible that before the command
      * reaches the server another event will already be on its way. Like any other request,
      * PGREQ_PING takes us off the waiting list. Wait for a return code: if it's an event,
      * Go ahead and return it. (There will still be a return packet on it's way, but
      * we skip that) If it's a return packet, ignore it and go on with the idle handler */

     _pg_send(&unwaitreq,sizeof(unwaitreq));
     if (_pg_recv(rsptype,sizeof(short)))
       return 1;
     if ((*rsptype) == htons(PG_RESPONSE_EVENT))
       return 0;
     _pg_recv(cruft,sizeof(cruft));
      
     /* At this point either it was a client-defined fd or a timeout.
	Either way we need to kickstart the event loop. */

     /* Run the idle handler, reset the event loop, then try again */
     _pg_idle();
     
     /* Clear the pipes... */
     pgFlushRequests();
     _pg_send(&waitreq,sizeof(waitreq));  /* Kickstart the event loop */
  }
}
   
   /* Malloc wrapper */
void *_pg_malloc(size_t size) {
  void *p;
  p = malloc(size);
  if (!p)
    clienterr("out of memory");
  return p;
}

/* In most situations this will display an error dialog, giving
 * the option to exit or continue. If it isn't even able to make
 * a dialog box, it will print to stderr.
 */
void _pg_defaulterr(unsigned short errortype,const char *msg) {
  static unsigned char in_defaulterr = 0;

  /* Are we unable to make a dialog? (no connection, or we already tried) */  
  if (in_defaulterr)
    exit(errortype);

  /* Print on stderr too in case the dialog fails */
  fprintf(stderr,"*** PicoGUI ERROR (%s) : %s\n",pgErrortypeString(errortype),msg);

  /* We haven't even established a connection? (let the error print first) */
  if (!_pgsockfd)
    exit(errortype);

  /* Try a dialog box */
  in_defaulterr = 1;
  if (PG_MSGBTN_YES ==
      pgMessageDialogFmt("PicoGUI Error",PG_MSGBTN_YES | PG_MSGBTN_NO,
			 "An error of type %s occurred"
			 " in %s:\n\n%s\n\nTerminate the application?",
			 pgErrortypeString(errortype),_pg_appname,msg))
    exit(errortype);
  pgUpdate(); /* In case this happens in a wierd place, go ahead and update. */
  in_defaulterr = 0;
}

/* Put a request into the queue */
void _pg_add_request(short reqtype,void *data,unsigned long datasize) {
  struct pgrequest *newhdr;

  /* If this will overflow the buffer, flush it and send this packet
   * individually. This has two possible uses:
   *    - there are many packets in the buffer, so it flushes them 
   *    - this latest packet is very large and won't fit in the buffer
   *      anyway (a bitmap or long string for example)
   */
  if ((_pgreqbuffer_size + sizeof(struct pgrequest) + datasize) >
      PG_REQBUFSIZE) {

    struct pgrequest req;

    pgFlushRequests();

    /* Send this one */
    req.type = htons(reqtype);
    req.id = ++_pgrequestid;
    req.size = htonl(datasize);
    _pg_send(&req,sizeof(struct pgrequest));
    _pg_send(data,datasize);

#ifdef DEBUG
    printf("Forced buffer flush. New request: type = %d, size = %d\n",reqtype,datasize);
#endif
    
    _pg_getresponse();
    return;
  } 

  /* Find a good place for the new header in the buffer and fill it in */
  newhdr = (struct pgrequest *) (_pgreqbuffer + _pgreqbuffer_size);
  _pgreqbuffer_count++;
  _pgreqbuffer_size += sizeof(struct pgrequest);
  newhdr->type = htons(reqtype);
  newhdr->id = ++_pgrequestid;
  newhdr->size = htonl(datasize);

#ifdef DEBUG
  printf("Added request: type = %d, size = %d\n",reqtype,datasize);
#endif

  /* Now the data */
  memcpy(_pgreqbuffer + _pgreqbuffer_size,data,datasize);
  _pgreqbuffer_size += datasize;
}

void _pg_getresponse(void) {
  short rsp_id = 0;
  
  /* Read the response type. This is where the client spends almost
   * all it's time waiting (and the only safe place to interrupt)
   * so handle the idling here.
   */
  
  if ( ((_pgidle_period.tv_sec + _pgidle_period.tv_usec)&&!_pgidle_lock) ||
	(_pgselect_handler != &select) ) {
     
     /* Use the interruptable wait */
     if (_pg_recvtimeout(&_pg_return.type))
       return;
  }
  else {
     /* Normal recieve */
     
     if (_pg_recv(&_pg_return.type,sizeof(_pg_return.type)))
       return;
  }

  _pg_return.type = ntohs(_pg_return.type);

  switch (_pg_return.type) {
    
  case PG_RESPONSE_ERR:
    {
      /* Error */
      struct pgresponse_err pg_err;
      char *msg;

      /* Read the rest of the error (already have response type) */
      pg_err.type = _pg_return.type;
      if (_pg_recv(((char*)&pg_err)+sizeof(_pg_return.type),
		   sizeof(pg_err)-sizeof(_pg_return.type)))
	return;
      rsp_id = pg_err.id;
      pg_err.errt = ntohs(pg_err.errt);
      pg_err.msglen = ntohs(pg_err.msglen);
      
      /* Dynamically allocated buffer for the error message */ 
      if (!(msg = _pg_malloc(pg_err.msglen+1)))
	return;
      if(_pg_recv(msg,pg_err.msglen))
	return;
      msg[pg_err.msglen] = 0;
       
      (*_pgerrhandler)(pg_err.errt,msg);
      free(msg);
    }
    break;

  case PG_RESPONSE_RET:
    {
      /* Return value */
      struct pgresponse_ret pg_ret;
      
      /* Read the rest of the error (already have response type) */
      pg_ret.type = _pg_return.type;
      if (_pg_recv(((char*)&pg_ret)+sizeof(_pg_return.type),
		   sizeof(pg_ret)-sizeof(_pg_return.type)))
	return;
      rsp_id = pg_ret.id;
      _pg_return.e.retdata = ntohl(pg_ret.data);
    }
    break;

  case PG_RESPONSE_EVENT:
    {
      /* Event */
      struct pgresponse_event pg_ev;

      /* Read the rest of the event (already have response type) */
      if (_pg_recv(((char*)&pg_ev)+sizeof(_pg_return.type),
		   sizeof(pg_ev)-sizeof(_pg_return.type)))
	return;
      pg_ev.type = _pg_return.type;
      memset(&_pg_return.e.event,0,sizeof(_pg_return.e.event));
      _pg_return.e.event.type = ntohs(pg_ev.event);
      _pg_return.e.event.from = ntohl(pg_ev.from);      
      pg_ev.param = ntohl(pg_ev.param);
       
      /* Decode the event differently based on the type */
      switch (_pg_return.e.event.type & PG_EVENTCODINGMASK) {

	 /* Easy, da? */
       default:
       case PG_EVENTCODING_PARAM:
	 _pg_return.e.event.e.param = pg_ev.param;      
	 break;
       
	 /* On some architectures this isn't necessary due to the layout
	  * of the union. Hope the optimizer notices :) */
       case PG_EVENTCODING_XY:
       case PG_EVENTCODING_KBD:   /* Same thing, just different names */
	 _pg_return.e.event.e.size.w = pg_ev.param >> 16;
	 _pg_return.e.event.e.size.h = pg_ev.param & 0xFFFF;
	 break;
	 
	 /* Decode 'mouse' parameters */
       case PG_EVENTCODING_PNTR:
	 _pg_return.e.event.e.pntr.x     = pg_ev.param & 0x0FFF;
	 _pg_return.e.event.e.pntr.y     = (pg_ev.param>>12) & 0x0FFF;
	 _pg_return.e.event.e.pntr.btn   = pg_ev.param >> 28;
	 _pg_return.e.event.e.pntr.chbtn = (pg_ev.param>>24) & 0x000F;
	 break;
	 
	 /* Transfer extra data */
       case PG_EVENTCODING_DATA:   
	 _pg_return.e.event.e.data.size = pg_ev.param;      
	 if (!(_pg_return.e.event.e.data.pointer = 
	       _pg_malloc(_pg_return.e.event.e.data.size+1)))
	   return;
	 if (_pg_recv(_pg_return.e.event.e.data.pointer,
		      _pg_return.e.event.e.data.size))
	   return;
	 /* Add a null terminator */
	 ((char *)_pg_return.e.event.e.data.pointer)
	     [_pg_return.e.event.e.data.size] = 0;
	 break;

	 /* Decode keyboard event */
	 
      }
	
    }
    break;

  case PG_RESPONSE_DATA:
    {
      /* Something larger- return it in a dynamically allocated buffer */
      struct pgresponse_data pg_data;
      
      /* Read the rest of the response (already have response type) */
      pg_data.type = _pg_return.type;
      if (_pg_recv(((char*)&pg_data)+sizeof(_pg_return.type),
		   sizeof(pg_data)-sizeof(_pg_return.type)))
	return;
      rsp_id = pg_data.id;
      _pg_return.e.data.size = ntohl(pg_data.size);
      
      if (!(_pg_return.e.data.data = 
	    _pg_malloc(_pg_return.e.data.size+1)))
	return;
      if (_pg_recv(_pg_return.e.data.data,_pg_return.e.data.size))
	return;

      /* Add a null terminator */
      ((char *)_pg_return.e.data.data)[_pg_return.e.data.size] = 0;
    }
    break;

  default:
      clienterr("Unexpected response type");
  }
  
#ifdef DEBUG
  if(rsp_id && (rsp_id != _pgrequestid)) {
    /* ID mismatch. This shouldn't happen, but if it does it's probably
       not serious.
    */
    printf("PicoGUI - incorrect packet ID (%i -> %i)\n",_pgrequestid,rsp_id);
  }
#endif

#ifdef DEBUG_EVT
   if (_pg_return.type == PG_RESPONSE_EVENT) 
     printf("Event is %d after case\n",
	    _pg_return.e.event.type);
#endif
}

void _pg_free_memdata(struct pgmemdata memdat) {
  if (memdat.flags & PGMEMDAT_NEED_FREE)
    free(memdat.pointer);
  if (memdat.flags & PGMEMDAT_NEED_UNMAP)
    munmap(memdat.pointer,memdat.size);
}

/* Format a message in a dynamically allocated buffer */
char * _pg_dynformat(const char *fmt,va_list ap) {
  /* This is adapted from the example code
     in Linux's printf manpage */

  /* Guess we need no more than 100 bytes. */
  int n, size = 100;
  char *p;

  if (!(p = _pg_malloc(size)))
    return NULL;
  while (1) {
    /* Try to print in the allocated space. */
    n = vsnprintf (p, size, fmt, ap);
    /* If that worked, return the string. */
    if (n > -1 && n < size)
      break;
    /* Else try again with more space. */
    if (n > -1)    /* glibc 2.1 */
      size = n+1; /* precisely what is needed */
    else           /* glibc 2.0 */
      size *= 2;  /* twice the old size */

    free(p);     /* Don't depend on realloc here.
		    In ucLinux, realloc is not available
		    so it is implemented by allocating
		    a new buffer and copying the data.
		    We don't need the old data so this
		    is more efficient. */    
    if (!(p = _pg_malloc(size)))
      return NULL;
  }
  return p;
}

/* Idle handler */
void _pg_idle(void) {

   if (_pgidle_lock) return;
  _pgidle_lock++;

  if (_pgidle_handler)
    (*_pgidle_handler)();

  _pgidle_lock = 0;
}

/******************* API functions */

/* Open a connection to the server, parsing PicoGUI commandline options
  if they are present
*/  
void pgInit(int argc, char **argv)
{
  int  numbytes;
  struct pghello ServerInfo;
  struct hostent *he;
  struct sockaddr_in server_addr; /* connector's address information */
  const char *hostname;
  int fd,i,j,args_to_shift;
  char *arg;
  volatile int tmp;
#ifdef UCLINUX  
  struct in_addr srv_addr;
#endif

  /* Get the app's name */
  _pg_appname = argv[0];

  /* Set default handlers */
  pgSetErrorHandler(&_pg_defaulterr);
  _pgselect_handler = &select;

  /* Default tunables */
  hostname = getenv("pgserver");
  if ((!hostname) || (!*hostname))
     hostname = PG_REQUEST_SERVER;

  /* Handle arguments we recognize, Leave others for the app */
  for (i=1;i<argc;i++) {
    arg = argv[i];

    /* It's ours if it starts with --pg */
    if (!bcmp(arg,"--pg",4)) {
      arg+=4;
      args_to_shift = 1;

      if (!strcmp(arg,"server")) {
	/* --pgserver : Next argument is the picogui server */
	args_to_shift = 2;
	hostname = argv[i+1];
	setenv("pgserver",hostname,1);    /* Child processes inherit server */
      }

      else if (!strcmp(arg,"version")) {
	/* --pgversion : For now print CVS id */
	fprintf(stderr,"$Id: picogui_client.c,v 1.55 2001/03/07 18:26:07 pney Exp $\n");
	exit(1);
      }
      
      else {
	/* Other command, print some help */
	fprintf(stderr,"PicoGUI Client Library\nCommands: --pgserver --pgversion\n");
	exit(1);
      }

      /* Remove this argument - shuffle all future args back a space */
      argc -= args_to_shift;
      for (j=i;j<argc;j++)
	argv[j] = argv[j+args_to_shift];
    }
  }
  /* Some programs might rely on this? */
  argv[argc] = NULL;

#ifdef UCLINUX
  /* get the host info.
   * gethostbyname() and gethostbyaddr() not working in uClinux.
   * Using inet_aton()
   * I let the two first in the case of... or for the future.
   */
  if ((he=gethostbyname(hostname)) != NULL) {
    srv_addr = *((struct in_addr *)he->h_addr);
  }
  else if ((he=gethostbyaddr(hostname,strlen(hostname),AF_INET)) != NULL) {
    srv_addr = *((struct in_addr *)he->h_addr);
  }
  else if (inet_aton(hostname, &srv_addr)) {
  }
  else {
    clienterr("Error resolving server hostname");
    return;
  }
#else
  if ((he=gethostbyname(hostname)) == NULL) {  /* get the host info */
    clienterr("Error resolving server hostname");
    return;
  }
#endif



  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    clienterr("socket error");
    return;
  }

  /* Try disabling the "Nagle algorithm" or "tinygram prevention" */
  tmp = 1;
  setsockopt(fd,6 /*PROTO_TCP*/,TCP_NODELAY,(void *)&tmp,sizeof(tmp));
   
  server_addr.sin_family = AF_INET;                 /* host byte order */
  server_addr.sin_port = htons(PG_REQUEST_PORT);    /* short, network byte order */
#ifdef UCLINUX
  server_addr.sin_addr = srv_addr;
#else
  server_addr.sin_addr = *((struct in_addr *)he->h_addr);
#endif
  bzero(&(server_addr.sin_zero), 8);                /* zero the rest of the struct */

  if (connect(fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    clienterr("Error connecting to server");
    return;
  }

  /* We're connected */
  _pgsockfd = fd;

  /* Don't let child processes inherit the connection */
  fcntl(fd,F_SETFD,fcntl(fd,F_GETFD,0) | FD_CLOEXEC);

  /* Receive the hello packet and convert byte order */
  if (_pg_recv(&ServerInfo,sizeof(ServerInfo)))
    return;
  ServerInfo.magic = ntohl(ServerInfo.magic);
  ServerInfo.protover = ntohs(ServerInfo.protover);
  
  /* Validate it */
  if(ServerInfo.magic != PG_REQUEST_MAGIC) {
    clienterr("server has bad magic number");
    return;
  }
  if(ServerInfo.protover < PG_PROTOCOL_VER)
    pgMessageDialog("PicoGUI Warning",
		    "The PicoGUI server is older than this program;\n"
		    "you may experience compatibility problems.",0);
}

void pgSetErrorHandler(void (*handler)(unsigned short errortype,
				       const char *msg)) {
  _pgerrhandler = handler;
}

const char *pgErrortypeString(unsigned short errortype) {
  switch (errortype) {
  case PG_ERRT_MEMORY:     return "MEMORY";
  case PG_ERRT_IO:         return "IO";
  case PG_ERRT_NETWORK:    return "NETWORK";
  case PG_ERRT_BADPARAM:   return "BADPARAM";
  case PG_ERRT_HANDLE:     return "HANDLE";
  case PG_ERRT_INTERNAL:   return "INTERNAL";
  case PG_ERRT_BUSY:       return "BUSY";
  case PG_ERRT_CLIENT:     return "CLIENT";
  case PG_ERRT_NONE:       return "NONE";
  }
  return "UNKNOWN";
}

/* Sets an idle handler using nonblocking IO. See details in client_c.h */
pgidlehandler pgSetIdle(long t,pgidlehandler handler) {
   pgidlehandler previous = _pgidle_handler;
   if (!handler) t = 0;
   if (!t) handler = 0;
   _pgidle_handler = handler;
   _pgidle_period.tv_sec = t / 1000;
   _pgidle_period.tv_usec = (t % 1000) * 1000;
   return previous;
}

/* Flushes the buffer of packets - if there's only one, it gets sent as is
  More than one packet is wrapped in a batch packet.

  This checks for errors.  If a return value is needed, the API function will
  call FlushRequests directly. If the client calls this it is assumed that they
  don't care about the return value (anything needing a return value would have
  called it explicitly)

  Return data is stored in _pg_return (yes, it's a little messy, but it's more
  efficient than a static variable in the function that must be passed out as
  a pointer) I believe in niceness for APIs and efficiency for the guts 8-)
*/
void pgFlushRequests(void) {

#ifdef DEBUG
  printf("Flushed %d packet(s), %d bytes\n",_pgreqbuffer_count,_pgreqbuffer_size);
#endif

  /* Free the data! */
  if (_pg_return.type == PG_RESPONSE_DATA &&
      _pg_return.e.data.data) {
    free(_pg_return.e.data.data);
    _pg_return.e.data.data = NULL;
  }
  if (_pg_return.type == PG_RESPONSE_EVENT &&
      (_pg_return.e.event.type & PG_EVENTCODINGMASK) == PG_EVENTCODING_DATA &&
      _pg_return.e.event.e.data.pointer) {
    free(_pg_return.e.event.e.data.pointer);
    _pg_return.e.event.e.data.pointer = NULL;
  }

  if (!_pgreqbuffer_count) {
    /* No packets */
    return;
  }
  else if (_pgreqbuffer_count==1) {
    /* One packet- send as is */
    if (_pg_send(_pgreqbuffer,_pgreqbuffer_size))
      return;
  }
  else {
    /* Many packets - use a batch packet */
    struct pgrequest batch_header;
    batch_header.type = htons(PGREQ_BATCH);
    batch_header.id = ++_pgrequestid;    /* Order doesn't matter for id */
    batch_header.size = htonl(_pgreqbuffer_size);
    if (_pg_send(&batch_header,sizeof(batch_header)))
      return;
    if (_pg_send(_pgreqbuffer,_pgreqbuffer_size))
      return;
  }

  /* Reset buffer */
  _pgreqbuffer_size = _pgreqbuffer_count = 0;

  /* Need a response packet back... */
  _pg_getresponse();
}

/* This is called after the app finishes it's initialization.
   It waits for events, and dispatches them to the functions they're
   bound to.
*/
void pgEventLoop(void) {
  struct _pghandlernode *n;
  int num;
  struct pgEvent evt;

  _pgeventloop_on = 1;

  while (_pgeventloop_on) {

    /* Wait for and event and save a copy
     * (a handler might call pgFlushRequests and overwrite it) */
    evt = *pgGetEvent();

#ifdef DEBUG_EVT
     printf("Recieved event %d from 0x%08X with param 0x%08X\n",
	    evt.type,evt.from,evt.e.param);
#endif

    /* Search the handler list, executing the applicable ones */
    n = _pghandlerlist;
    num = 0;
    while (n) {
      if ( (((signed long)n->widgetkey)==PGBIND_ANY || n->widgetkey==evt.from) &&
	   (((signed short)n->eventkey)==PGBIND_ANY || n->eventkey==evt.type) ) {
	 evt.extra = n->extra;
	 if ((*n->handler)(&evt))
	   goto skiphandlers;
      }

       n = n->next;
    }

    /* Various default actions */
      
    if (evt.type == PG_WE_CLOSE)
      exit(0);

  skiphandlers:
  }
}

void pgExitEventLoop(void) { _pgeventloop_on=0; }

struct pgEvent *pgGetEvent(void) {

  /* Run the idle handler here too, so it still gets a chance
   * even if we're flooded with events. */
  _pg_idle();

  /* Update before waiting for the user */
  pgUpdate();

  /* Wait for a new event */
  do {
     _pg_add_request(PGREQ_WAIT,NULL,0);
     pgFlushRequests();
  } while (_pg_return.type != PG_RESPONSE_EVENT);

  return &_pg_return.e.event;
}

/* Add the specified handler to the list */
void pgBind(pghandle widgetkey,unsigned short eventkey,
	    pgevthandler handler, void *extra) {
   struct _pghandlernode *p,*n = NULL;
   
   /* Default widget? */
   if (!widgetkey) 
     widgetkey = _pgdefault_widget;
   
   p = _pghandlerlist;
   
   /* Are we just deleting a node? */
   if (!handler) {
      struct _pghandlernode **where = &_pghandlerlist;
      
      while (p) {
	 if (p->widgetkey==widgetkey &&
	     p->eventkey==eventkey) {

	    *where = p->next;
	    free(p);
	    return;
	 }
	 where = &p->next;
	 p = p->next;
      }
      return;
   }
   
   /* Is the node already present? */
   while (p) {
      if (p->widgetkey==widgetkey &&
	  p->eventkey==eventkey) {
	 n = p;
	 break;
      }
      p = p->next;
   }
   
   /* Allocate a new hander node */
   if (!n) {
      if (!(n = _pg_malloc(sizeof(struct _pghandlernode))))
	return;
      
      /* Add it at the beginning of the list (order doesn't
       * matter, and this is faster and smaller */
      
      n->next = _pghandlerlist;
      _pghandlerlist = n;
      
      n->widgetkey = widgetkey;
      n->eventkey = eventkey;
   }
   
   /* Set the handler's effects... */
   n->handler = handler;
   n->extra = extra;
}

/* Set a custom handler instead of the usual select() */
void pgCustomizeSelect(pgselecthandler handler) {
  if (handler)
    _pgselect_handler = handler;
  else
    _pgselect_handler = &select;
}

/******* The simple functions that don't need args or return values */

void pgUpdate(void) {
  _pg_add_request(PGREQ_UPDATE,NULL,0);
  /* Update forces a buffer flush */
  pgFlushRequests();
}

void pgEnterContext(void) {
  _pg_add_request(PGREQ_MKCONTEXT,NULL,0);
}  

void pgLeaveContext(void) {
  _pg_add_request(PGREQ_RMCONTEXT,NULL,0);
}  

pghandle pgLoadTheme(struct pgmemdata obj) {

  /* Error */
  if (!obj.pointer) return 0;

  /* FIXME: I should probably find a way to do this that
     doesn't involve copying the data- probably flushing any
     pending packets, then writing the mmap'd file data directly
     to the socket.

     The current method is memory hungry when dealing with larger files.
  */
  _pg_add_request(PGREQ_MKTHEME,obj.pointer,obj.size);
  _pg_free_memdata(obj);
}

/******* Data loading */

/* Data already loaded in memory */
struct pgmemdata pgFromMemory(void *data,unsigned long length) {
  static struct pgmemdata x;    /* Maybe make something like this
				   global to use less memory? */
  x.pointer = data;
  x.size = length;
  x.flags = 0;
  return x;
}

/* Load from a normal disk file */
struct pgmemdata pgFromFile(const char *file) {
  static struct pgmemdata x;
  struct stat st;
  int fd;

  /* FIXME: Make this code try to use mmap(2) to load files first.
     Much more efficient for larger files. */

  fd = open(file,O_RDONLY);
  if (fd<0) {
    /* FIXME: Better error message / a way for the app to catch this error */
    clienterr("Error opening file in pgFromFile()");
    x.pointer = NULL;
    return x;
  }
  fstat(fd,&st);
  x.size = st.st_size;

  /* FIXME: more error checking (you can tell this function has been
     a quick hack so I can test theme loading :) */
  if (!(x.pointer = _pg_malloc(x.size))) {
    x.pointer = NULL;
    return x;
  }
  x.flags = PGMEMDAT_NEED_FREE;

  read(fd,x.pointer,x.size);

  close(fd);
  return x;
}

/******* A little more complex ones, with args */

void pgSetPayload(pghandle object,unsigned long payload) {
  struct pgreqd_setpayload arg;
  arg.h = htonl(object ? object : _pgdefault_widget);
  arg.payload = htonl(payload);
  _pg_add_request(PGREQ_SETPAYLOAD,&arg,sizeof(arg));
}

void pgRegisterOwner(int resource) {
  struct pgreqd_regowner arg;
  arg.res = htons(resource);
  _pg_add_request(PGREQ_REGOWNER,&arg,sizeof(arg));
}

void pgUnregisterOwner(int resource) {
  struct pgreqd_regowner arg;
  arg.res = htons(resource);
  _pg_add_request(PGREQ_UNREGOWNER,&arg,sizeof(arg));
}

void pgSendKeyInput(unsigned long type,unsigned short key,
		    unsigned short mods) {
  struct pgreqd_in_key arg;
  arg.type = htonl(type);
  arg.key  = htons(key);
  arg.mods = htons(mods);
  _pg_add_request(PGREQ_IN_KEY,&arg,sizeof(arg));
}

/* Also used by networked input devices, but to send pointing device events */
void pgSendPointerInput(unsigned long type,unsigned short x,unsigned short y,
			unsigned short btn) {
  struct pgreqd_in_point arg;
  arg.type = htonl(type);
  arg.x  = htons(x);
  arg.y  = htons(y);
  arg.btn = htons(btn);
  arg.dummy = 0;
  _pg_add_request(PGREQ_IN_POINT,&arg,sizeof(arg));
}

unsigned long pgGetPayload(pghandle object) {
  object = htonl(object);
  _pg_add_request(PGREQ_GETPAYLOAD,&object,sizeof(object));
  pgFlushRequests();
  return _pg_return.e.retdata;
}


void pgSubUpdate(pghandle widget) {
  struct pgreqd_handlestruct arg;
  arg.h = htonl(widget ? widget : _pgdefault_widget);
  _pg_add_request(PGREQ_UPDATEPART,&arg,sizeof(arg));
  pgFlushRequests();
}

void pgFocus(pghandle widget) {
  struct pgreqd_handlestruct arg;
  arg.h = htonl(widget ? widget : _pgdefault_widget);
  _pg_add_request(PGREQ_FOCUS,&arg,sizeof(arg));
}

void pgDelete(pghandle object) {
  struct pgreqd_handlestruct arg;
  struct _pghandlernode *n,*condemn = NULL;
  
  /* Ignore if the object is 0 */
  if (!object) return;

  arg.h = htonl(object);
  _pg_add_request(PGREQ_FREE,&arg,sizeof(arg));

  /* Delete handlers that rely on this widget */
  if (_pghandlerlist->widgetkey == object) {
    condemn = _pghandlerlist;
    _pghandlerlist = condemn->next;
    free(condemn);
  }
  n = _pghandlerlist;
  while (n->next) {
    if (n->next->widgetkey == object) {
      condemn = n->next;
      n->next = condemn->next;
      free(condemn);
    }
    n = n->next;
  }
}

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
pghandle pgRegisterApp(short int type,const char *name, ...) {
  va_list v;
  struct pgreqd_register *arg;
  short *spec;
  int numspecs,i;
  
  /* First just count the number of APPSPECs we have */
  for (va_start(v,name),numspecs=0;va_arg(v,short);
       va_arg(v,short),numspecs++);
  va_end(ap);

  /* Allocate */
  if (!(arg = malloc(sizeof(struct pgreqd_register)+numspecs*4)))
    return;
  /* Move pointer */
  spec = (short int *)(((char*)arg)+sizeof(struct pgreqd_register));

  /* Fill in the required params */
  arg->name = htonl(pgNewString(name));
  arg->type = htons(type);
  arg->dummy = 0;

  /* Fill in the optional APPSPEC params */
  for (va_start(v,name),i=numspecs<<1;i;
       i--,*(spec++)=htons(va_arg(v,short)));
  va_end(ap);

  _pg_add_request(PGREQ_REGISTER,arg,sizeof(struct pgreqd_register)+numspecs*4);

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_INSIDE;
  _pgdefault_widget = _pg_return.e.retdata;
  
  /* Return the new handle */
  return _pg_return.e.retdata;
}

void  pgSetWidget(pghandle widget, ...) {
  va_list v;
  struct pgreqd_set arg;
  short *spec;
  int numspecs,i;

  /* Set defaults values */
  arg.widget = htonl(widget ? widget : _pgdefault_widget);
  arg.dummy = 0;

  va_start(v,widget);
  for (;;) {
    i = va_arg(v,short);
    if (!i) break;
    arg.property = htons(i);
    arg.glob = htonl(va_arg(v,long));
    _pg_add_request(PGREQ_SET,&arg,sizeof(arg));
  }
  va_end(v);
}

pghandle pgNewWidget(short int type,short int rship,pghandle parent) {
  struct pgreqd_mkwidget arg;

  /* We don't need to validate the type here, the server does that. */
    
  arg.type = htons(type);

  /* Default placement is after the previous widget
   * (Unless is was a special widget, like a root widget)
   * Passing 0 for 'rship' and 'parent' to get the defaults? */
  arg.parent = htonl(parent ? parent : _pgdefault_widget);
  arg.rship  = htons(rship  ? rship  : _pgdefault_rship);

  _pg_add_request(PGREQ_MKWIDGET,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_AFTER;
  _pgdefault_widget = _pg_return.e.retdata;
  
  /* Return the new handle */
  return _pg_return.e.retdata;
}

pghandle pgNewPopupAt(int x,int y,int width,int height) {
  struct pgreqd_mkpopup arg;
  arg.x = htons(x);
  arg.y = htons(y);
  arg.w = htons(width);
  arg.h = htons(height);
  _pg_add_request(PGREQ_MKPOPUP,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_INSIDE;
  _pgdefault_widget = _pg_return.e.retdata;
  
  /* Return the new handle */
  return _pg_return.e.retdata;
}

pghandle pgNewFont(const char *name,short size,unsigned long style) {
  struct pgreqd_mkfont arg;
  memset(&arg,0,sizeof(arg));

  if (name)
    strcpy(arg.name,name);
  else
    *arg.name = 0;
  arg.style = htonl(style);
  arg.size = htons(size);
  _pg_add_request(PGREQ_MKFONT,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
}

pghandle pgNewPopup(int width,int height) {
  /* Tell the server to center it */
  return pgNewPopupAt(PG_POPUP_CENTER,-1,width,height);
}

pghandle pgNewBitmap(struct pgmemdata obj) {

  /* Error */
  if (!obj.pointer) return 0;

  /* FIXME: I should probably find a way to do this that
     doesn't involve copying the data- probably flushing any
     pending packets, then writing the mmap'd file data directly
     to the socket.

     The current method is memory hungry when dealing with larger files.
  */
  _pg_add_request(PGREQ_MKBITMAP,obj.pointer,obj.size);
  _pg_free_memdata(obj);

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the property */
  return _pg_return.e.retdata;
}

pghandle pgNewString(const char* str) {
  if (!str) return 0;

  /* Passing the NULL terminator to the server is redundant.
   * no need for a +1 on that strlen...
   */
  _pg_add_request(PGREQ_MKSTRING,(void *) str,strlen(str));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
}

long pgGetWidget(pghandle widget,short property) {
  struct pgreqd_get arg;
  arg.widget = htonl(widget ? widget : _pgdefault_widget);
  arg.property = htons(property);
  arg.dummy = 0;
  _pg_add_request(PGREQ_GET,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the property */
  return _pg_return.e.retdata;
}

/* Measure a piece of text in a font, in pixels */
void pgSizeText(int *w,int *h,pghandle font,pghandle text) {
  struct pgreqd_sizetext arg;
  arg.text = htonl(text);
  arg.font = htonl(font);
  _pg_add_request(PGREQ_SIZETEXT,&arg,sizeof(arg));
   
  /* Get the return value */
  pgFlushRequests();
  if (w) *w = _pg_return.e.retdata >> 16;
  if (h) *h = _pg_return.e.retdata & 0xFFFF;
}
   
/* Get the contents of a string handle. */
char *pgGetString(pghandle string) {
  string = htonl(string);
  _pg_add_request(PGREQ_GETSTRING,&string,sizeof(pghandle));
  pgFlushRequests();
  return _pg_return.e.data.data;
}

/* Get and delete the previous text, and set the
   text to a new string made with the given text */
void pgReplaceText(pghandle widget,const char *str) {
  pghandle oldtext;

  if (!widget) widget = _pgdefault_widget;

  oldtext = pgGetWidget(widget,PG_WP_TEXT);
  pgSetWidget(widget,PG_WP_TEXT,pgNewString(str),0);
  pgDelete(oldtext);
}

/* Like pgReplaceText, but supports printf-style
 * text formatting */
void pgReplaceTextFmt(pghandle widget,const char *fmt, ...) {
  char *p;
  va_list ap;
  
  va_start(ap,fmt);
  if (!(p = _pg_dynformat(fmt,ap)))
    return;
  pgReplaceText(widget,p);
  free(p);
  va_end(ap);
}

/* Like pgMessageDialog, but uses printf-style formatting */
int pgMessageDialogFmt(const char *title,unsigned long flags,const char *fmt, ...) {
  char *p;
  int ret;
  va_list ap;

  va_start(ap,fmt);
  if (!(p = _pg_dynformat(fmt,ap)))
    return;
  ret = pgMessageDialog(title,p,flags);
  free(p);
  va_end(ap);
  return ret;
}

/* Create a message box, wait until it is
 * answered, then return the answer.
 */
int pgMessageDialog(const char *title,const char *text,unsigned long flags) {
  struct pgreqd_mkmsgdlg arg;
  pghandle from;
  unsigned long ret;

  /* New context for us! */
  pgEnterContext();

  /* Build the dialog box */
  arg.title = htonl(pgNewString(title));
  arg.text =  htonl(pgNewString(text));
  arg.flags = htonl(flags);
  _pg_add_request(PGREQ_MKMSGDLG,&arg,sizeof(arg));

  /* Run it (ignoring zero-payload events) */
  while (!(ret = pgGetPayload(pgGetEvent()->from)));

  /* Go away now */
  pgLeaveContext();

  return ret;
}

/* There are many ways to create a menu in PicoGUI
 * (at the lowest level, using pgNewPopupAt and the menuitem widget)
 *
 * This creates a static popup menu from a "|"-separated list of
 * menu items, and returns the number (starting with 1) of the chosen
 * item, or 0 for cancel.
 */
int pgMenuFromString(char *items) {
  struct pgreqd_mkmsgdlg arg;
  pghandle from;
  unsigned long ret;
  unsigned long *handletab;
  int i;
  char *p;

  if (!items || !*items) return 0;

  /* Count how many items we'll need */
  i = 1;
  p = items;
  while (*p) {
    if (*p == '|') i++;
    p++;
  }
  if (!(handletab = alloca(4*i)))
    return;

  /* New context for us! */
  pgEnterContext();
  
  /* Send over the strings individually, store handles */
  i = 0;
  do {
    if (!(p = strchr(items,'|'))) p = items + strlen(items);
    _pg_add_request(PGREQ_MKSTRING,(void *) items,p-items);
    items = p+1;
    pgFlushRequests();
    handletab[i++] = _pg_return.e.retdata;
  } while (*p);

  ret = pgMenuFromArray(handletab,i);
  pgLeaveContext();
  return ret;
}

/* This creates a menu from an array of string handles. 
 * Same return values as pgMenuFromString above.
 *
 * Important note: pgMenuFromArray expects that a new
 *                 context will be entered before the
 *                 string handles are created.
 *                 Therefore, it contains a call to
 *                 pgLeaveContext() as part of its clean-up.
 */
int pgMenuFromArray(pghandle *items,int numitems) {
  int i;
  /* This function's a lot smaller than it sounds :) */

  for (i=0;i<numitems;i++)         /* Swap bytes */
    items[i] = htonl(items[i]);
  _pg_add_request(PGREQ_MKMENU,items,4*numitems);
  for (i=0;i<numitems;i++)         /* Unswap */
    items[i] = ntohl(items[i]);

  /* Return event */
  return pgGetPayload(pgGetEvent()->from);
}

/* Write data to a widget.
 * (for example, a terminal widget)
 */
void pgWriteData(pghandle widget,struct pgmemdata data) {
  unsigned long *buf;

  /* FIXME: Shouln't be recopying this... */

  if (!data.pointer) return;
  if (!(buf = _pg_malloc(data.size+4))) return;
  *buf = htonl(widget ? widget : _pgdefault_widget);
  memcpy(buf+1,data.pointer,data.size);

  _pg_add_request(PGREQ_WRITETO,buf,data.size+4);

  _pg_free_memdata(data);
  free(buf);
}

/* Wrapper around pgWriteData to send a command, for example
 * to a canvas widget. Widget, command, and param number must be followed
 * by the specified number of commands
 */
void pgWriteCmd(pghandle widget,short command,short numparams, ...) {
   struct pgcommand *hdr;
   signed long *params;
   unsigned long bufsize;
   char *buf;
   va_list v;
   
   bufsize = numparams * sizeof(signed long) + sizeof(struct pgcommand);
   buf = alloca(bufsize);
   hdr = (struct pgcommand *) buf;
   params = (signed long *) (buf + sizeof(struct pgcommand));
   
   hdr->command = htons(command);
   hdr->numparams = htons(numparams);
      
   va_start(v,numparams);
   for (;numparams;numparams--) {
      *params = htonl(va_arg(v,signed long));
      params++;
   }
   va_end(V);
   
   pgWriteData(widget,pgFromMemory(buf,bufsize));
}

/* The End */
