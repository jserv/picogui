/* $Id: picogui_client.c,v 1.28 2000/11/12 08:32:41 micahjd Exp $
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
 * Contributors: Philippe Ney <philippe.ney@smartdata.ch>
 * 
 * 
 * 
 */

/******************* Definitions and includes */

/* System includes */
#include <sys/socket.h>
#include <sys/time.h>  /* for time_t type (used in timeval structure) */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>    /* for fprintf() */
#include <malloc.h>
#include <string.h>   /* for memcpy(), memset(), strcpy() */
#include <stdarg.h>   /* needed for pgRegisterApp and pgSetWidget */
#include <math.h>     /* for floor() */

/* PicoGUI */
#include <picogui.h>            /* Basic PicoGUI include */
#include <picogui/network.h>    /* Network interface to the server */

//#define DEBUG

/* Default server */
#define PG_REQUEST_SERVER       "localhost"

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

long _pgnonblocking_on;         /* To set a timeout in the pgEventLoop */
typedef int (*wrapper_f)(void *data,unsigned long datasize);
                                /* Pointer to IO wrappers */
char *_pg_appname;              /* Name of the app's binary */

/* Structure for a retrieved and validated response code,
   the data collected by _pg_flushpackets is stored here. */
struct {
  short type;
  union {

    /* if type == PG_RESPONSE_RET */
    unsigned long retdata;

    /* if type == PG_RESPONSE_EVENT */
    struct {
      unsigned short event;
      pghandle from;
      unsigned long param;
    } event;

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
int _pg_recvtimout(void *data,unsigned long datasize);

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

/* Timout receive.. */
int _pg_recvtimeout(void *data,unsigned long datasize) {
  struct timeval tv;
  fd_set readfds;

  tv.tv_sec = floor(_pgnonblocking_on / 100);
  tv.tv_usec = (_pgnonblocking_on - (tv.tv_sec * 100)) * 10000;

  FD_ZERO(&readfds);
  FD_SET(_pgsockfd,&readfds);

  /* don't care about writefds and exceptfds: */
  select(_pgsockfd+1,&readfds,NULL,NULL,&tv);

  if (FD_ISSET(_pgsockfd, &readfds)) {
#ifdef DEBUG
    printf("Something happened on socket!\n");
#endif
    return _pg_recv(data,datasize);
  }
  else {
#ifdef DEBUG
    printf("Timed out.\n");
#endif
    pgExitEventLoop();
    return 1;
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
  if (in_defaulterr || !_pgsockfd) {
    fprintf(stderr,"*** PicoGUI ERROR (%s) : %s\n",pgErrortypeString(errortype),msg);
    exit(errortype);
  }

  /* Try a dialog box */
  in_defaulterr = 1;
  if (PG_MSGBTN_YES ==
      pgMessageDialogFmt("PicoGUI Error",PG_MSGBTN_YES | PG_MSGBTN_NO,
			 "An error of type %s occurred"
			 " in %s:\n\n%s\n\nTerminate the application?",
			 pgErrortypeString(errortype),_pg_appname,msg))
    exit(errortype);
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
  wrapper_f pf;
  
  if(!_pgnonblocking_on) pf = &_pg_recv;
  else                   pf = &_pg_recvtimeout;

  /* Read the response type */
    if ((*pf)(&_pg_return.type,sizeof(_pg_return.type)))
      return;
  _pg_return.type = ntohs(_pg_return.type);

  switch (_pg_return.type) {
    
  case PG_RESPONSE_ERR:
    {
      /* Error */
      struct pgresponse_err pg_err;
      char *msg;

      /* Read the rest of the error (already have response type) */
      pg_err.type = _pg_return.type;
      if ((*pf)(((char*)&pg_err)+sizeof(_pg_return.type),
		   sizeof(pg_err)-sizeof(_pg_return.type)))
	return;
      rsp_id = pg_err.id;
      pg_err.errt = ntohs(pg_err.errt);
      pg_err.msglen = ntohs(pg_err.msglen);
      
      /* Dynamically allocated buffer for the error message */ 
      if (!(msg = _pg_malloc(pg_err.msglen)))
	return;
      if(_pg_recv(msg,pg_err.msglen))
	return;
      
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
      if ((*pf)(((char*)&pg_ret)+sizeof(_pg_return.type),
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
      pg_ev.type = _pg_return.type;
      if ((*pf)(((char*)&pg_ev)+sizeof(_pg_return.type),
		   sizeof(pg_ev)-sizeof(_pg_return.type)))
	return;
      _pg_return.e.event.event = ntohs(pg_ev.event);
      _pg_return.e.event.from = ntohl(pg_ev.from);      
      _pg_return.e.event.param = ntohl(pg_ev.param);      
    }
    break;

  case PG_RESPONSE_DATA:
    {
      /* Something larger- return it in a dynamically allocated buffer */
      struct pgresponse_data pg_data;
      
      /* Read the rest of the error (already have response type) */
      pg_data.type = _pg_return.type;
      if ((*pf)(((char*)&pg_data)+sizeof(_pg_return.type),
		   sizeof(pg_data)-sizeof(_pg_return.type)))
	return;
      rsp_id = pg_data.id;
      _pg_return.e.data.size = ntohl(pg_data.size);
      
      if (!(_pg_return.e.data.data = 
	    _pg_malloc(_pg_return.e.data.size+1)))
	return;
      if ((*pf)(_pg_return.e.data.data,_pg_return.e.data.size))
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
  int fd;

  /* Get the app's name */
  _pg_appname = argv[0];

  /* Set default error handler */
  pgSetErrorHandler(&_pg_defaulterr);

  /* Set default to blocking receive state */
  pgSetnonblocking(0);

  /* Should use a getopt-based system here to process various args, leaving
     the extras for the client app to process. But this is ok temporarily.
  */
  if (argc != 2)
    hostname = PG_REQUEST_SERVER;
  else
    hostname = argv[1];

  if ((he=gethostbyname(hostname)) == NULL) {  /* get the host info */
    clienterr("Error resolving server hostname");
    return;
  }

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    clienterr("socket error");
    return;
  }

  server_addr.sin_family = AF_INET;                 /* host byte order */
  server_addr.sin_port = htons(PG_REQUEST_PORT);    /* short, network byte order */
  server_addr.sin_addr = *((struct in_addr *)he->h_addr);
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
		    "The PicoGUI server is newer (higher protocol\n"
		    "version) than this application. you may experience\n" 
		    "compatibility problems.",0);
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

/* Timeout setting */
void pgSetnonblocking(long Hundredthseconds) {
  if(Hundredthseconds >= 0)
    _pgnonblocking_on = Hundredthseconds;
  else
    _pgnonblocking_on = -Hundredthseconds;
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
  short event;
  pghandle from;
  long param;
  int num;

  _pgeventloop_on = 1;

  while (_pgeventloop_on) {
    /* Good practice to update before waiting on the user
       (and, unless doing animation of some sort, nowhere else) */
    pgUpdate();

    /* Wait for a new event */
    _pg_add_request(PGREQ_WAIT,NULL,0);
    pgFlushRequests();
    
    /* Save the event (a handler might call pgFlushRequests and overwrite it) */
    event = _pg_return.e.event.event;
    from = _pg_return.e.event.from;
    param = _pg_return.e.event.param;

    /* Search the handler list, executing the applicable ones */
    n = _pghandlerlist;
    num = 0;
    while (n) {
      if ( (((signed long)n->widgetkey)==PGBIND_ANY || n->widgetkey==from) &&
	   (((signed short)n->eventkey)==PGBIND_ANY || n->eventkey==event) )
	if ((*n->handler)(event,from,param))
	  goto skiphandlers;
      n = n->next;
    }

    /* Various default actions */
      
    if (event == PG_WE_CLOSE)
      exit(0);

  skiphandlers:
  }
}

void pgExitEventLoop(void) { _pgeventloop_on=0; }

pghandle pgGetEvent(unsigned short *event, unsigned long *param) {
  /* Update before waiting for the user */
  pgUpdate();

  /* Wait for a new event */
  _pg_add_request(PGREQ_WAIT,NULL,0);
  pgFlushRequests();
  if (event) *event = _pg_return.e.event.event;
  if (param) *param = _pg_return.e.event.param;
  return _pg_return.e.event.from;
}

/* Add the specified handler to the list */
void pgBind(pghandle widgetkey,unsigned short eventkey,
	    pgevthandler handler) {
  struct _pghandlernode *n;

  /* Default widget? */
  if (!widgetkey) 
    widgetkey = _pgdefault_widget;

  /* Make sure this _exact_ node doesn't already exist */
  n = _pghandlerlist;
  while (n) {
    if (n->widgetkey==widgetkey &&
	n->eventkey==eventkey &&
	n->handler==handler)
      return;
    n = n->next;
  }

  /* Allocate a new hander node */
  if (!(n = _pg_malloc(sizeof(struct _pghandlernode))))
    return;
  
  n->widgetkey = widgetkey;
  n->eventkey = eventkey;
  n->handler = handler;

  /* Add it at the beginning of the list (order doesn't
   * matter, and this is faster and smaller */

  n->next = _pghandlerlist;
  _pghandlerlist = n;
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
  if (!obj.pointer) return NULL;

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

unsigned long pgGetPayload(pghandle object) {
  object = htonl(object);
  _pg_add_request(PGREQ_GETPAYLOAD,&object,sizeof(object));
  pgFlushRequests();
  return _pg_return.e.retdata;
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

  for(va_start(v,widget);i;){
    i = va_arg(v,short);
    if(i){
      arg.property = htons(i);
      arg.glob = htonl(va_arg(v,long));
      _pg_add_request(PGREQ_SET,&arg,sizeof(arg));
    }
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
  if (!obj.pointer) return NULL;

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
  while (!(ret = pgGetPayload(pgGetEvent(NULL,NULL))));

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
    if (*p == '\n') i++;
    p++;
  }
  if (!(handletab = _pg_malloc(4*i)))
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

  free(handletab);
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
  return pgGetPayload(pgGetEvent(NULL,NULL));
}

/* The End */
