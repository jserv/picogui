/* $Id: netcore.c,v 1.8 2001/07/03 05:48:15 micahjd Exp $
 *
 * netcore.c - core networking code for the C client library
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

#include "clientlib.h"

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
struct _pg_return_type _pg_return;   /* Response from _pg_flushpackets */

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
  char *s1,*copys1,*s2,*copys2;
		  
  /* Are we unable to make a dialog? (no connection, or we already tried) */  
  if (in_defaulterr)
    exit(errortype);

  /* Print on stderr too in case the dialog fails */
  fprintf(stderr,"*** PicoGUI ERROR (%s) : %s\n",pgErrortypeString(errortype),msg);

  /* We haven't even established a connection? (let the error print first) */
  if (!_pgsockfd)
    exit(errortype);

  /* Try a dialog box.
	* We must copy the strings because the pgGetString buffer is only valid
	* until the next picogui call */
  in_defaulterr = 1;
  s1 = pgGetString(pgThemeLookup(PGTH_O_DEFAULT,
											PGTH_P_STRING_PGUIERR));
  copys1 = alloca(strlen(s1)+1);
  strcpy(copys1,s1);
  s2 = pgGetString(pgThemeLookup(PGTH_O_DEFAULT,
											PGTH_P_STRING_PGUIERRDLG));
  copys2 = alloca(strlen(s2)+strlen(pgErrortypeString(errortype))+
						strlen(_pg_appname)+strlen(msg)+1);
  sprintf(copys2,s2,pgErrortypeString(errortype),_pg_appname,msg);
  if (PG_MSGBTN_YES ==
      pgMessageDialog(copys1,copys2,
							 PG_MSGBTN_YES | PG_MSGBTN_NO))
					 exit(errortype);
	   pgUpdate(); /* In case this happens in a weird place, go ahead and update. */
	   in_defaulterr = 0;
}

/* Some 'user friendly' default sig handlers */
void _pgsig(int sig) {
	 char *a,*b;
	 short id;
		  
	 switch (sig) {
    
    case SIGSEGV:
		id = PGTH_P_STRING_SEGFAULT;
  		break;
			  				
    case SIGFPE:
		id = PGTH_P_STRING_MATHERR;
      break;
     
	 default:
		return;

	 }
				
	 a = pgGetString(pgThemeLookup(PGTH_O_DEFAULT,
											 id));
	 b = alloca(strlen(a)+1);
	 strcpy(b,a);
	 clienterr(b);
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
     /* Normal receive */
     
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
      msg = alloca(pg_err.msglen+1);
      if(_pg_recv(msg,pg_err.msglen))
		      return;
      msg[pg_err.msglen] = 0;
       
      (*_pgerrhandler)(pg_err.errt,msg);
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
#ifndef CONFIG_UNIX_SOCKET
  struct hostent *he;
  struct sockaddr_in server_addr; /* connector's address information */
#else
  struct sockaddr_un server_addr; 
#endif
  const char *hostname;
  int fd,i,j,args_to_shift;
  char *arg;
  volatile int tmp;
#ifdef UCLINUX  
  struct in_addr srv_addr;
#endif
  struct stat st;

  /* Save the program's name */
  _pg_appname = argv[0];
   
  /* Set default handlers */
  pgSetErrorHandler(&_pg_defaulterr);
  _pgselect_handler = &select;
  signal(SIGSEGV,&_pgsig);
  signal(SIGFPE,&_pgsig);
   
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
	fprintf(stderr,"$Id: netcore.c,v 1.8 2001/07/03 05:48:15 micahjd Exp $\n");
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
#ifndef CONFIG_UNIX_SOCKET
  if ((he=gethostbyname(hostname)) == NULL) {  /* get the host info */
    clienterr("Error resolving server hostname");
    return;
  }
#endif
#endif


#ifndef CONFIG_UNIX_SOCKET
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#else
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
#endif
    clienterr("socket error");
    return;
  }

#ifndef CONFIG_UNIX_SOCKET
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
#else
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path,hostname);
  i = strlen(server_addr.sun_path) + sizeof(server_addr.sun_family);

  if (connect(fd, (struct sockaddr *)&server_addr, i) == -1) {
#endif
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
  if(ServerInfo.protover < PG_PROTOCOL_VER) {
	 const char *s1, *copys1, *s2;
			 
			 /* We must copy the first string temporarily because the pgGetString
			  * buffer is only valid until the next picogui call */
			 s1 = pgGetString(pgThemeLookup(PGTH_O_DEFAULT,
													  PGTH_P_STRING_PGUIWARN));
			 copys1 = alloca(strlen(s1)+1);
			 strcpy(copys1,s1);
			 s2 = pgGetString(pgThemeLookup(PGTH_O_DEFAULT,
													  PGTH_P_STRING_PGUICOMPAT)),   
			 pgMessageDialog(copys1,s2,0);
  }
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

/* The End */
