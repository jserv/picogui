/* $Id: netcore.c,v 1.28 2002/01/17 10:58:40 gobry Exp $
 *
 * netcore.c - core networking code for the C client library
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * Thread-safe code added by RidgeRun Inc.
 * Copyright (C) 2001 RidgeRun, Inc.  All rights reserved.
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
#include <ctype.h>
#include <stdlib.h>    /* for atol() */
#include <errno.h>

#ifdef ENABLE_THREADING_SUPPORT
#include <pthread.h>
static sem_t _pgreqbufferSem;
static void pgFlushRequests(void);
static pthread_t idleLoopThread;
static pthread_t eventLoopThread;
static sem_t idleLoopSem;
static sem_t eventLoopSem;
static struct pgEvent pgEventThreadEvent;
static void *pgDispatchEvent(void *na);
#endif

/* Global vars for the client lib */
int _pgsockfd;                  /* Socket fd to the pgserver */
short _pgrequestid;             /* Request ID to detect errors */
short _pgdefault_rship;         /* Default relationship and widget */
pghandle _pgdefault_widget;        /*    when 0 is used */
unsigned char _pgeventloop_on;  /* Boolean - is event loop running? */
unsigned char _pgreqbuffer[PG_REQBUFSIZE];  /* Buffer of request packets */
short _pgreqbuffer_size;        /* # of bytes in reqbuffer */
short _pgreqbuffer_count;       /* # of packets in reqbuffer */
short _pgreqbuffer_lasttype;    /* Type of last packet, indication of what return
				 * packet should be sent */
void (*_pgerrhandler)(unsigned short errortype,const char *msg);
                                /* Error handler */
struct _pghandlernode *_pghandlerlist;  /* List of pgBind event handlers */

struct timeval _pgidle_period;  /* Period before calling idle handler */
pgidlehandler _pgidle_handler;  /* Idle handler */
unsigned char _pgidle_lock;     /* Already in idle handler? */
char *_pg_appname;              /* Name of the app's binary */
pgselecthandler _pgselect_handler;   /* Normally a pointer to select() */
pgselectbh _pgselect_bottomhalf;     /* Select handler bottom-half */
struct _pg_return_type _pg_return;   /* Response from _pg_flushpackets */

/* If this is nonzero, the application should be created in this container
 * instead of a new app in pgRegisterApp
 */
pghandle _pg_appletbox;

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
  while (1) {
    if (recv(_pgsockfd,data,datasize,0) >= 0) break;
    
    /* if we have been interrupted (by a signal for instance), restart
       the reception */
    if (errno != EINTR) {
      clienterr("recv error");
      return 1;
    }
  }
  return 0;
}

/* Wait for a new event, recieves the type code. This is used 
 * when an idle handler or other interruption is needed */
int _pg_recvtimeout(short *rsptype) {
  struct timeval tv;
  fd_set readfds;
  int result;
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

     result = (*_pgselect_handler)(_pgsockfd+1,&readfds,NULL,NULL,
				   (tv.tv_sec + tv.tv_usec) ? &tv : NULL);
     if (result < 0)
       continue;

     /* Well, now we have something! */

     if (FD_ISSET(_pgsockfd, &readfds))   /* Got data from server */
	return _pg_recv(rsptype,sizeof(short));
	
     /* If we get this far, it's not a return packet- it's either a pgIdle timeout or a
      * user-defined file descriptor added with pgCustomizeSelect. This code below needs
      * to safely suspend the event loop, call the appropriate handlers, then kickstart it
      * with a new 'wait' packet. This is only possible if we are currently waiting on a
      * response from the 'wait' packet. If it's something else, this will clobber the 
      * packet's return value and get the event loop out of sync. This is why
      * _pg_recvtimeout should only be used in event waits.
      */

     /* We'll need these */
     waitreq.id = ++_pgrequestid;
     unwaitreq.id = ++_pgrequestid;

     /* No event yet, but now we have a race condition. We need to tell the server
      * to take the client off the waiting list, but it's possible that before the command
      * reaches the server another event will already be on its way. Like any other request,
      * PGREQ_PING takes us off the waiting list. Wait for a return code: if it's an event,
      * Go ahead and return it. (There will still be a return packet on it's way, but
      * we skip that) If it's a return packet, ignore it and go on with the idle handler */
#ifdef ENABLE_THREADING_SUPPORT
     _pg_add_request(PGREQ_PING, NULL, 0, -1, 1);
#else
     _pg_send(&unwaitreq,sizeof(unwaitreq));
#endif     
     if (_pg_recv(rsptype,sizeof(short)))
       return 1;
     if ((*rsptype) == htons(PG_RESPONSE_EVENT))
       /* Important! We need to indicate to the caller that there's an extra
	* return packet on it's way after this event packet.
	*/
       return 2;
     _pg_recv(cruft,sizeof(cruft));
      
     /* At this point either it was a client-defined fd or a timeout.
	Either way we need to kickstart the event loop. */
#ifdef ENABLE_THREADING_SUPPORT
      sem_post(&idleLoopSem);
      _pg_add_request(PGREQ_WAIT, NULL, 0, -1, 1);
#else      
     /* At this point, it's safe to make PicoGUI API calls. */
     if (_pgselect_bottomhalf)
       (*_pgselect_bottomhalf)(result,&readfds);
     _pg_idle();
     
     /* Clear the pipes... */
     pgFlushRequests();
     _pg_send(&waitreq,sizeof(waitreq));  /* Kickstart the event loop */
#endif     
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
		      PG_MSGBTN_YES | PG_MSGBTN_NO | PG_MSGICON_ERROR))
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
#ifdef ENABLE_THREADING_SUPPORT
void _pg_add_request(short reqtype,void *data,unsigned long datasize, unsigned int id, unsigned char flush) {
#else   
void _pg_add_request(short reqtype,void *data,unsigned long datasize) {
#endif
   
  struct pgrequest *newhdr;
  int padding;

#ifdef ENABLE_THREADING_SUPPORT  
  sem_wait(&_pgreqbufferSem);
#endif
  
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
#ifdef ENABLE_THREADING_SUPPORT
    req.id = id;
#else    
    req.id = ++_pgrequestid;
#endif    
    req.size = htonl(datasize);
    _pg_send(&req,sizeof(struct pgrequest));
    _pg_send(data,datasize);

#ifdef DEBUG
    printf("Forced buffer flush. New request: type = %d, size = %d\n",reqtype,datasize);
#endif

#ifdef ENABLE_THREADING_SUPPORT
    //
    // Flush the buffer if the event loop is *not* started
    //
    if ( !_pgeventloop_on )
       _pg_getresponse(0);

    sem_post(&_pgreqbufferSem);
#else    
    _pg_getresponse(0);
#endif
    
    return;
  } 

  padding = _pgreqbuffer_size & 3;  
#ifdef ENABLE_THREADING_SUPPORT
  if (padding) {
     fprintf(stderr, "What the heck is this.  The server does not take into account this padding!\n");
     exit(-1);
  }
#else  
  /* Pad the request buffer to a 32-bit boundary */
  if (padding) {
    padding = 4-padding;
    memset(_pgreqbuffer + _pgreqbuffer_size, 0, padding);
    _pgreqbuffer_size += padding;
  }
#endif
  
  /* Find a good place for the new header in the buffer and fill it in */
  newhdr = (struct pgrequest *) (_pgreqbuffer + _pgreqbuffer_size);
  _pgreqbuffer_count++;
  _pgreqbuffer_size += sizeof(struct pgrequest);
  _pgreqbuffer_lasttype = reqtype;
  newhdr->type = htons(reqtype);
#ifdef ENABLE_THREADING_SUPPORT
  newhdr->id = id;
#else  
  newhdr->id = ++_pgrequestid;
#endif  
  newhdr->size = htonl(datasize);

#ifdef DEBUG
  printf("Added request: type = %d, size = %d\n",reqtype,datasize);
#endif

  /* Now the data */
  memcpy(_pgreqbuffer + _pgreqbuffer_size,data,datasize);
  _pgreqbuffer_size += datasize;

#ifdef ENABLE_THREADING_SUPPORT  
  //
  // If intentional flush is set, flush the buffer
  //
  if ( flush )
     pgFlushRequests();
  
  sem_post(&_pgreqbufferSem);
#endif
  
}

void _pg_getresponse(int eventwait) {
  short rsp_id = 0;
  int extra_packet = 0;

  /* Read the response type. This is where the client spends almost
   * all it's time waiting (and the only safe place to interrupt)
   * so handle the idling here.
   */

#ifdef ENABLE_THREADING_SUPPORT
  if ( eventwait && ( (_pgidle_period.tv_sec + _pgidle_period.tv_usec) || 
		      (_pgselect_handler != &select) ) ) {
#else     
  if ( eventwait && ( ((_pgidle_period.tv_sec + _pgidle_period.tv_usec)&&!_pgidle_lock) ||
		      (_pgselect_handler != &select) ) ) {
#endif       
     
    /* Use the interruptable wait (only for event waits!) */
    switch (_pg_recvtimeout(&_pg_return.type)) {
    case 2:
      /* There's an extra packet after this one (return code from 'ping')
       * so we'll need to suck that up after we're done processing the
       * actual event packet.
       */
      extra_packet = 1;
      break;
    case 0:
      break;
    default:
      return;
    }
  }
  else {
     /* Normal receive */
     
     if (_pg_recv(&_pg_return.type,sizeof(_pg_return.type)))
       return;
  }

  _pg_return.type = ntohs(_pg_return.type);

  switch (_pg_return.type) {

#ifdef ENABLE_THREADING_SUPPORT
  case PG_RESPONSE_ERR:
    {
      /* Error */
      struct pgresponse_err pg_err;
      char *msg;
      pg_err.type = _pg_return.type;
      
      //
      // Read the rest of the return error.  Don't panic if we can't read it all
      //
      if ( ! _pg_recv(((char*)&pg_err)+sizeof(_pg_return.type), sizeof(pg_err)-sizeof(_pg_return.type)) ) {

         pg_err.errt = ntohs(pg_err.errt);
         pg_err.msglen = ntohs(pg_err.msglen);
         
         /* Dynamically allocated buffer for the error message */ 
         msg = alloca(pg_err.msglen+1);

         //
         // Read the message
         //
         if( !_pg_recv(msg,pg_err.msglen)) {
            msg[pg_err.msglen] = 0;
         }
         
         (*_pgerrhandler)(pg_err.errt,msg);

         //
         // If a proper id, extract the semaphore and post
         //
         if ( pg_err.id != -1 ) {
            
            pgClientReturnData *crd = (pgClientReturnData *)pg_err.id;
            crd->ret.type = pg_err.type;
            sem_post(&crd->sem);
         
         }  // End of reading the error packet
      
      }
    }
    break;

#else     
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
#endif

#ifdef ENABLE_THREADING_SUPPORT
  case PG_RESPONSE_RET:
    {
      /* Return value */
      struct pgresponse_ret pg_ret;
      pg_ret.type = _pg_return.type;
      
      //
      // Read the rest of the return error.  Don't panic if we can't read it all
      //
      if ( !_pg_recv(((char*)&pg_ret)+sizeof(_pg_return.type), sizeof(pg_ret)-sizeof(_pg_return.type)) ) {

         if ( pg_ret.id != -1 ) {

            pgClientReturnData *crd = (pgClientReturnData *)pg_ret.id;
            crd->ret.type = pg_ret.type;
            crd->ret.e.retdata = ntohl(pg_ret.data);
            sem_post(&crd->sem);
            
         }  // End of client API call response handling
         else {

            //
            // I don't think I need this stuff, because this case should only be necessary for the event loop, but
            // .....
            //
            _pg_return.e.retdata = ntohl(pg_ret.data);
         }
         
      }  // End of reading response packet

    }
    break;

#else    
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
#endif
    
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

#ifdef ENABLE_THREADING_SUPPORT
  case PG_RESPONSE_DATA:
    {
      /* Something larger- return it in a dynamically allocated buffer */
      struct pgresponse_data pg_data;
      pg_data.type = _pg_return.type;
      
      //
      // Read the rest of the return error.  Don't panic if we can't read it all
      //
      if ( !_pg_recv(((char*)&pg_data)+sizeof(_pg_return.type), sizeof(pg_data)-sizeof(_pg_return.type)) )

         //
         // Is this a client API call?
         //
         if ( pg_data.id != -1 ) {

            pgClientReturnData *crd = (pgClientReturnData *)pg_data.id;
            crd->ret.type = pg_data.type;            
            crd->ret.e.data.size = ntohl(pg_data.size);

            //
            // Allocate return data
            //
            if ( (crd->ret.e.data.data = _pg_malloc(pg_data.size)) ) {

               //
               // Receive the data into the buffer.  Add a null terminator because the original pgui code did ;)
               //
               _pg_recv(crd->ret.e.data.data, crd->ret.e.data.size);
               ((char *)crd->ret.e.data.data)[crd->ret.e.data.size] = 0;
            }

            sem_post(&crd->sem);            

         }  // End of Client API processing
         else {

            //
            // Don't know if this case is really valid, since the event loop should not receive and handle these
            // types of responses, but I'll do this anyway.
            //
            _pg_return.e.data.size = ntohl(pg_data.size);
            if (!(_pg_return.e.data.data = _pg_malloc(_pg_return.e.data.size+1))) {
               if (_pg_recv(_pg_return.e.data.data,_pg_return.e.data.size)) {
                  /* Add a null terminator */
                  ((char *)_pg_return.e.data.data)[_pg_return.e.data.size] = 0;
               }
            }
         }  

    }   // End of handling PG_RESPONSE_DATA
    break;

#else    
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
#endif
    
  default:
      clienterr("Unexpected response type");
  }

#ifndef ENABLE_THREADING_SUPPORT   
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
#endif  //    ENABLE_THREADING_SUPPORT

   /* If there was an extra packet (left over from a 'ping') 
    * we should suck that up now
    */
   if (extra_packet) {
     struct pgresponse_ret cruft;
     _pg_recv(&cruft,sizeof(cruft));
   }
 
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
#ifdef ENABLE_THREADING_SUPPORT
void * _pg_idle(void *na) {
   while (1) {
      sem_wait(&idleLoopSem);
      if ( _pgidle_handler)
         (*_pgidle_handler)();
   }
}
#else   
void _pg_idle(void) {
#if 0
  struct timeval now, elapsed;
  static struct timeval then = {0,0};
#endif

  /* Make sure we aren't recursing */
   if (_pgidle_lock) return;
  _pgidle_lock++;

#if 0
  /* Make sure we've waited long enough */
  gettimeofday(&now,NULL);
  elapsed.tv_sec = now.tv_sec - then.tv_sec;
  elapsed.tv_usec = now.tv_usec - then.tv_usec;
  if (elapsed.tv_usec < 0) {
    elapsed.tv_sec--;
    elapsed.tv_usec += 1000000;
  }
  if (elapsed.tv_sec < _pgidle_period.tv_sec)
    return;
  if (elapsed.tv_sec == _pgidle_period.tv_sec && 
      elapsed.tv_usec < _pgidle_period.tv_usec)
    return;
  then = now;
#endif

  /* Call the idle handle */
  if (_pgidle_handler)
    (*_pgidle_handler)();

  _pgidle_lock = 0;
}

#endif  //  ENABLE_THREADING_SUPPORT
 
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
  unsigned short port = PG_REQUEST_PORT;
  const char *appletparam = NULL;
  int fd,i,j,args_to_shift;
  char *arg;
  volatile int tmp;
#ifdef UCLINUX  
  struct in_addr srv_addr;
#endif
  struct stat st;
  int enable_warning = 1;

  /* Save the program's name */
  _pg_appname = argv[0];
   
  /* Set default handlers */
  pgSetErrorHandler(&_pg_defaulterr);
  _pgselect_handler = &select;
  _pgselect_bottomhalf = NULL;
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
	fprintf(stderr,"$Id: netcore.c,v 1.28 2002/01/17 10:58:40 gobry Exp $\n");
	exit(1);
      }

      else if (!strcmp(arg,"applet")) {
	/* --pgapplet : Create the app in a public container instead of
	 *              registering a new app.
	 */

	args_to_shift = 2;
	appletparam = argv[i+1];
      }

      else if (!strcmp(arg,"nowarn")) {
	enable_warning = 0;
      }
      
      else {
	/* Other command, print some help */
	fprintf(stderr,"PicoGUI Client Library\nCommands: --pgserver --pgversion --pgapplet --pgnowarn\n");
	exit(1);
      }

      /* Remove this argument - shuffle all future args back a space */
      argc -= args_to_shift;
      for (j=i;j<argc;j++)
	argv[j] = argv[j+args_to_shift];
      i -= args_to_shift;
    }
  }
  /* Some programs might rely on this? */
  argv[argc] = NULL;

  /* Separate the display number from the hostname */
  arg = strrchr(hostname,':');
  if (arg) {
    port = PG_REQUEST_PORT + atoi(arg+1);
    *arg = 0;
  }
  /* Only a display? Use default server */
  if (!*hostname)
    hostname = PG_REQUEST_SERVER;
   
#ifndef CONFIG_UNIX_SOCKET
#  ifdef UCLINUX
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
#  else
  if ((he=gethostbyname(hostname)) == NULL) {  /* get the host info */
    clienterr("Error resolving server hostname");
    return;
  }
#  endif
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
   
  server_addr.sin_family = AF_INET;         /* host byte order */
  server_addr.sin_port = htons(port);       /* short, network byte order */
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
    perror ("connect");
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

  if((ServerInfo.protover < PG_PROTOCOL_VER) && enable_warning) {
	 const char *s1, *s2;
	 char * copys1;
	 
	 /* We must copy the first string temporarily because the pgGetString
	  * buffer is only valid until the next picogui call */
	 s1 = pgGetString(pgThemeLookup(PGTH_O_DEFAULT,
					PGTH_P_STRING_PGUIWARN));
	 copys1 = alloca(strlen(s1)+1);
	 strcpy(copys1, s1);
	 s2 = pgGetString(pgThemeLookup(PGTH_O_DEFAULT,
					PGTH_P_STRING_PGUICOMPAT)),   
	   pgMessageDialog(copys1,s2,0);
  }

  /* Set up applet handle */
  if (appletparam) {
    /* Is it a number or widget name? */
    if (isdigit(*appletparam))
      _pg_appletbox = atol(appletparam);
    else
      _pg_appletbox = pgFindWidget(appletparam);
  }

#ifdef ENABLE_THREADING_SUPPORT
   //
   // Setup the _pgreqbuffer semaphore as a MUTEX
   //
   sem_init(&_pgreqbufferSem, 0, 1 /* initial value */);
   sem_init(&idleLoopSem, 0, 0);
   sem_init(&eventLoopSem, 0, 0);
   pthread_create(&idleLoopThread, NULL, _pg_idle, NULL);
   pthread_create(&eventLoopThread, NULL, pgDispatchEvent, NULL);
#endif
   
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
  case PG_ERRT_FILEFMT:    return "FILEFMT";
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
#ifdef ENABLE_THREADING_SUPPORT
void pgFlushRequests(void) {

  //
  // Send something if we've got something
  //
  if ( _pgreqbuffer_count > 0 ) {
     
     //
     // Two cases.  If we've got one packet, just send the one.  Else we've got many requests, batch
     // them.
     //
     if ( _pgreqbuffer_count == 1 ) {
        _pg_send(_pgreqbuffer,_pgreqbuffer_size);
     }
     else {

        /* Many packets - use a batch packet */
        struct pgrequest batch_header;
        unsigned char *ptr = _pgreqbuffer;
        struct pgrequest *req;
        int index = 1;

        batch_header.type = htons(PGREQ_BATCH);

        //
        // In the case of batch, the id needs to be that of the previous entry
        //
        while ( index < _pgreqbuffer_count ) {
           req = (struct pgrequest *)ptr;
           ptr += (sizeof(struct pgrequest) + ntohl(req->size));
           ++index;
        }

        batch_header.id = ((struct pgrequest *)ptr)->id;
        batch_header.size = htonl(_pgreqbuffer_size);
        _pg_send(&batch_header,sizeof(batch_header));
        _pg_send(_pgreqbuffer,_pgreqbuffer_size);
    }

    /* Reset buffer */
    _pgreqbuffer_size = _pgreqbuffer_count = 0;
    
    //
    // Flush the buffer if the event loop is *not* started
    //
    if ( !_pgeventloop_on ) {
      /* Need a response packet back... If the last packet added to the buffer was a 'wait',
       * flag _pg_getresponse that it's ok to run idle handlers and select handlers. */
      _pg_getresponse(_pgreqbuffer_lasttype == PGREQ_WAIT);
    }

  }  // End if _pgreqbuffer_count > 0

}  // End of pgFlushRequests
#else 
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

  /* Need a response packet back... If the last packet added to the buffer was a 'wait',
   * flag _pg_getresponse that it's ok to run idle handlers and select handlers. */
  _pg_getresponse(_pgreqbuffer_lasttype == PGREQ_WAIT);
}
#endif // ENABLE_THREADING_SUPPORT

#ifdef ENABLE_THREADING_SUPPORT
void *pgDispatchEvent(void *na) {
  struct _pghandlernode *n;
  struct pgEvent *currentEvent;
  
  /* Search the handler list, executing the applicable ones */

  //
  // Top-level loop forever
  //
  while ( 1 ) {

     //
     // Sync with the system.  Only run this handler when prompted to do so
     //
     sem_wait(&eventLoopSem);
     currentEvent = &pgEventThreadEvent;
     
     //
     // Get pointer to the handler list & search it
     //
     n = _pghandlerlist;
     while (n) {
        
       if ( (((signed long)n->widgetkey)==PGBIND_ANY || n->widgetkey==currentEvent->from) &&
	         (((signed short)n->eventkey)==PGBIND_ANY || n->eventkey==currentEvent->type) ) {

          //
          // Found an event.  Process it
          //
          currentEvent->extra = n->extra;

          //
          // Run the event handler if present.  If the event handler returns something other than 0,
          // don't check for any more handlers for this input.
          //
          if ( n->handler(currentEvent) )
             break;   // Break out of inner loop
          
       }  // End of if found event
       
       //
       // Goto the next possible handler
       //
       n = n->next;

     }  // End of handler iteration
  
     /* Various default actions */
  
     if (currentEvent->type == PG_WE_CLOSE)
        exit(0);

  }  // End of forever event loop
  
} // End of pgDispatchEvent
 
/* This is called after the app finishes it's initialization.
   It waits for events, and dispatches them to the functions they're
   bound to.
*/
void pgEventLoop(void) {

  _pgeventloop_on = 1;

  while (_pgeventloop_on) {

    /* Wait for and event and save a copy
     * (a handler might call pgFlushRequests and overwrite it) */

    pgGetEvent();
    memcpy(&pgEventThreadEvent, &_pg_return.e.event, sizeof(struct pgEvent));
    sem_post(&eventLoopSem);

  }
}

#else 
/* This is called after the app finishes it's initialization.
   It waits for events, and dispatches them to the functions they're
   bound to.
*/
void pgEventLoop(void) {
  struct pgEvent evt;

  _pgeventloop_on = 1;

  while (_pgeventloop_on) {

    /* Wait for and event and save a copy
     * (a handler might call pgFlushRequests and overwrite it) */

    evt = *pgGetEvent();
    pgDispatchEvent(&evt);
  }
}

void pgDispatchEvent(struct pgEvent *evt) {
  struct _pghandlernode *n;
  
  /* Search the handler list, executing the applicable ones */
  
  n = _pghandlerlist;
  while (n) {
    if ( (((signed long)n->widgetkey)==PGBIND_ANY || 
	  n->widgetkey==evt->from) &&
	 (((signed short)n->eventkey)==PGBIND_ANY || 
	  n->eventkey==evt->type) ) {
      evt->extra = n->extra;
      
      /* Run the handler, and quit processing the
       * event if it returns nonzero.
       */
      if ((*n->handler)(evt))
	return;
    }
    
    n = n->next;
  }
  
  /* Various default actions */
  
  if (evt->type == PG_WE_CLOSE)
    exit(0);
}
#endif
 
void pgExitEventLoop(void) { _pgeventloop_on=0; }

#ifdef ENABLE_THREADING_SUPPORT
struct pgEvent *pgGetEvent(void) {
  
  //
  // Run the idle loop.
  //
  sem_post(&idleLoopSem);
  
  /* Update before waiting for the user */
  pgUpdate();

  /* Wait for a new event */
  do {
     _pg_add_request(PGREQ_WAIT,NULL,0, -1, 1);
     _pg_getresponse(1);
  } while (_pg_return.type != PG_RESPONSE_EVENT);

  return &_pg_return.e.event;
  
}
#else 
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

#endif // ENABLE_THREADING_SUPPORT
 
/* Add the specified handler to the list */
void pgBind(pghandle widgetkey,short eventkey,
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
void pgCustomizeSelect(pgselecthandler handler, pgselectbh bottomhalf) {
  if (handler) {
    _pgselect_handler = handler;
    _pgselect_bottomhalf = bottomhalf;
  }
  else {
    _pgselect_handler = &select;
    _pgselect_bottomhalf = NULL;
  }
}
 
#ifdef ENABLE_THREADING_SUPPORT 
void pgEventPoll(void) {
  /* This is like pgEventLoop, but completely nonblocking */
  
  while (pgCheckEvent()) {
    pgGetEvent();
    sem_post(&eventLoopSem);    
  }
}
#endif
 
/* The End */
