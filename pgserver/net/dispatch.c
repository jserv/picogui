/* $Id: dispatch.c,v 1.71 2001/12/30 22:11:09 micahjd Exp $
 *
 * dispatch.c - Processes and dispatches raw request packets to PicoGUI
 *              This is the layer of network-transparency between the app
 *              and the PicoGUI internal functions. Everything is packaged
 *              in network byte order, and a lot of parameter validation is
 *              performed.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 * Thread-safe code added by RidgeRun Inc.
 * Copyright (C) 2001 RidgeRun, Inc.  All rights reserved.
 * pgCreateWidget & pgAttachWidget functionality added by RidgeRun Inc.
 * Copyright (C) 2001 RidgeRun, Inc.  All rights reserved.
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

#include <pgserver/common.h>

#include <pgserver/pgnet.h>

/* First bring in function prototypes for all handlers */
#define RQH DEF_REQHANDLER
#include "requests.inc"
#undef RQH

/* Normal function table? */

#ifndef RUNTIME_FUNCPTR
/* Yep */

#define RQH TAB_REQHANDLER
g_error (*rqhtab[])(int,struct pgrequest*,void*,unsigned long*,int*) = {
#include "requests.inc"
};
#undef RQH

#else
/* Nope, do some funky stuff */

#define RQH(x) *p = &rqh_##x; p++;
g_error (*rqhtab[PGREQ_UNDEF+1])(int,struct pgrequest*,void*,unsigned long*,int*);
void rqhtab_init(void) {
   g_error (**p)(int,struct pgrequest*,void*,unsigned long*,int*);
   p = rqhtab;
#include "requests.inc"
}
#undef RQH
   
#endif /* RUNTIME_FUNCPTR */

/* Macro for casting the arguments */
#define reqarg(x) \
  struct pgreqd_##x *arg = (struct pgreqd_##x *) data; \
  if (req->size < sizeof(struct pgreqd_##x)) \
     return mkerror(PG_ERRT_BADPARAM,57);

/* Process an incoming packet, and if applicable generate a response packet.
 * Response packets are generated with call(s) to send_response. Returns
 * nonzero if the connection with the client is to be closed.
 */
int dispatch_packet(int from,struct pgrequest *req,void *data) {
  int fatal=0;
  unsigned long ret_data=0;
  g_error e;

  /* No invalid pointers for you! */
  if (req->type>PGREQ_UNDEF) req->type = PGREQ_UNDEF;
  
  /* Dispatch to one of the handlers in the table */
  e = (*rqhtab[req->type])(from,req,data,&ret_data,&fatal);
  
  if (errtype(e) == PG_ERRT_NONE) {
    /* No error, send a return code packet */

    struct pgresponse_ret rsp;
    
    rsp.type = htons(PG_RESPONSE_RET);
    rsp.id = htonl(req->id);
    rsp.data = htonl(ret_data);
    
    /* Send the return packet */
    fatal |= send_response(from,&rsp,sizeof(rsp));
  }
  else if (e != ERRT_NOREPLY) {
    /* If we need a reply, send error message */

    int errlen;
    struct pgresponse_err rsp;
    const char *errmsg;

    errlen = strlen(errmsg = errortext(e));
    
    rsp.type = htons(PG_RESPONSE_ERR);
    rsp.id = htonl(req->id);
    rsp.errt = htons(errtype(e));
    rsp.msglen = htons(errlen);

    /* Send the error */
    fatal |= send_response(from,&rsp,sizeof(rsp)) | 
             send_response(from,errmsg,errlen);
  }

  return fatal;
}

/***************** Request handlers *******/

g_error rqh_ping(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  return success;
}

g_error rqh_update(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  update(NULL,1);
  return success;
}

g_error rqh_mkwidget(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct widget *w = NULL,*parent = NULL;
  handle h;
  handle xh;
  g_error e,etmp;
  reqarg(mkwidget);

  /* Don't allow direct creation of 'special' widgets that must
     be created by other means (app registration, popup boxes)
  */
  switch (ntohs(arg->type)) {
  case PG_WIDGET_PANEL:
  case PG_WIDGET_MENUBAR:
  case PG_WIDGET_POPUP:
  case PG_WIDGET_BACKGROUND:
    return mkerror(PG_ERRT_BADPARAM,58);
  }

  etmp = rdhandle((void**) &parent,PG_TYPE_WIDGET,owner,xh=ntohl(arg->parent));
  if (iserror(etmp)) {
    if (ntohs(arg->rship) == PG_DERIVE_INSIDE) {
      /* Allow creating widgets in foreign containers if the container
       * has its 'publicbox' bit set
       */
      e = rdhandle((void**) &parent,PG_TYPE_WIDGET,-1,xh);
      errorcheck;
      if (!parent->publicbox)
	return etmp;
    }
    else
      return etmp;
  }
  if (!parent) return mkerror(PG_ERRT_BADPARAM,59);

  /* Don't let an app put stuff outside its root widget */
  if (owner>=0 && parent->isroot && ntohs(arg->rship)!=PG_DERIVE_INSIDE)
    return mkerror(PG_ERRT_BADPARAM,60);

  e = widget_derive(&w,ntohs(arg->type),parent,xh,ntohs(arg->rship),owner);
  errorcheck;

  e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
  errorcheck;
  
  *ret = h;

  return success;
}

g_error rqh_createwidget(int owner, struct pgrequest *req,
		                   void *data, unsigned long *ret, int *fatal) {
  struct widget *w;
  handle h;
  g_error e;
  /* Fake divtree to assign to unattached widgets */
  static struct divtree fakedt;
  static struct divnode fakedt_head;
  reqarg(createwidget);

  /* Don't allow direct creation of 'special' widgets that must
     be created by other means (app registration, popup boxes)
  */
  switch (ntohs(arg->type)) {
  case PG_WIDGET_PANEL:
  case PG_WIDGET_POPUP:
  case PG_WIDGET_BACKGROUND:
    return mkerror(PG_ERRT_BADPARAM,58);
  }

  memset(&fakedt,0,sizeof(fakedt));
  memset(&fakedt_head,0,sizeof(fakedt_head));
  fakedt.head = &fakedt_head;
  e = widget_create(&w, ntohs(arg->type), &fakedt, 0, owner);
  errorcheck;

  e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
  errorcheck;
  
  *ret = h;

  return success;
}

g_error rqh_attachwidget(int owner, struct pgrequest *req,
		                   void *data, unsigned long *ret, int *fatal) {
  struct widget *w,*parent;
  handle h;
  handle xh;
  g_error e,etmp;
  reqarg(attachwidget);

  etmp = rdhandle((void**) &parent,PG_TYPE_WIDGET,owner,xh=ntohl(arg->parent));
  if (iserror(etmp)) {
    if (ntohs(arg->rship) == PG_DERIVE_INSIDE) {
      /* Allow creating widgets in foreign containers if the container
       * has its 'publicbox' bit set
       */
      e = rdhandle((void**) &parent,PG_TYPE_WIDGET,-1,xh);
      errorcheck;
      if (!parent->publicbox)
	return etmp;
    }
    else
      return etmp;
  }
  if (!parent) return mkerror(PG_ERRT_BADPARAM,59);

  /* Don't let an app put stuff outside its root widget */
  if (owner>=0 && parent->isroot && ntohs(arg->rship)!=PG_DERIVE_INSIDE)
    return mkerror(PG_ERRT_BADPARAM,60);

  //
  // Get the handle to the widget
  //
  etmp = rdhandle((void **) &w, PG_TYPE_WIDGET, owner, ntohl(arg->widget));
  if ( iserror(etmp) )
     return etmp;

  //
  // Call widget_derive to actually do the attaching.  widget_derive will notice that the widget already
  // has been created and not create it again.  In this case, it will just do the attachment.
  return widget_derive(&w,w->type,parent,xh,ntohs(arg->rship),owner);  

} // rqh_attachwidget

g_error rqh_mkbitmap(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  hwrbitmap bmp;
  handle h;
  g_error e;
  
  e = VID(bitmap_load) (&bmp,data,req->size);
  errorcheck;
  e = mkhandle(&h,PG_TYPE_BITMAP,owner,bmp);
  errorcheck;
  
  *ret = h;
  return success;
}

g_error rqh_mkfont(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  handle h;
  g_error e;
  reqarg(mkfont);

  e = findfont(&h,owner,arg->name,ntohs(arg->size),ntohl(arg->style));
  errorcheck;

  *ret = h;
  return success;
}

g_error rqh_mkstring(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  char *buf;
  handle h;
  g_error e;

  e = g_malloc((void **) &buf, req->size+1);
  errorcheck;
  memcpy(buf,data,req->size);
  buf[req->size] = 0;  /* Null terminate it if it isn't already */

  e = mkhandle(&h,PG_TYPE_STRING,owner,buf);
  errorcheck;

  *ret = h;
  return success;
}

g_error rqh_free(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  reqarg(handlestruct);  
  return handle_free(owner,ntohl(arg->h));
}

g_error rqh_set(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct widget *w;
  g_error e;
  reqarg(set);

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,owner,ntohl(arg->widget));
  errorcheck;

  return widget_set(w,ntohs(arg->property),ntohl(arg->glob));
}

g_error rqh_get(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct widget *w;
  g_error e;
  reqarg(get);

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,owner,ntohl(arg->widget));
  errorcheck;

  *ret = widget_get(w,ntohs(arg->property));

  return success;
}

g_error rqh_undef(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  return mkerror(PG_ERRT_BADPARAM,62);
}

g_error rqh_in_key(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
#ifdef CONFIG_NOREMOTEINPUT
  return mkerror(PG_ERRT_BADPARAM,104);
#else
  reqarg(in_key);
  dispatch_key(ntohl(arg->type),(int) ntohs(arg->key),ntohs(arg->mods));
  return success;
#endif
}

g_error rqh_in_point(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
#ifdef CONFIG_NOREMOTEINPUT
  return mkerror(PG_ERRT_BADPARAM,104);
#else  
  reqarg(in_point);
  dispatch_pointing(ntohl(arg->type),ntohs(arg->x),ntohs(arg->y),
		    ntohs(arg->btn));
  return success;
#endif
}

g_error rqh_in_direct(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
#ifdef CONFIG_NOREMOTEINPUT
  return mkerror(PG_ERRT_BADPARAM,104);
#else
  reqarg(in_direct);
  dispatch_direct(((char*)arg)+sizeof(struct pgreqd_in_direct),
		  ntohl(arg->param));
  return success;
#endif
}

g_error rqh_wait(int owner, struct pgrequest *req,
		 void *data, unsigned long *ret, int *fatal) {
  struct event *q;

  /* Is there anything here already? */
  if (q = get_event(owner,1)) {
    struct pgresponse_event rsp;

    rsp.type = htons(PG_RESPONSE_EVENT);
    rsp.event = htons(q->event);
    rsp.from = htonl(q->from);
    rsp.param = htonl(q->param);
    send(owner,&rsp,sizeof(rsp),0);
    if ((q->event & PG_EVENTCODINGMASK) == PG_EVENTCODING_DATA) {
      send(owner,q->data,q->param,0);
      /* The data would be freed automatically when the 
       * queue slot is reused, but we might as well free it
       * here. We still need the free in eventq.c in case the
       * queue overflows.
       */
      g_free(q->data);
      q->data = NULL;
    }
  }
  else {
    /* Nop. Off to the waiting list... */
  
    FD_SET(owner,&evtwait);
#ifdef DEBUG_NET
    printf("Client (#%d) added to waiting list\n",owner);
#endif
  }

  return ERRT_NOREPLY;
}

g_error rqh_register(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct app_info i;
  g_error e;
  short int *spec = (short int *)(((char*)data)+sizeof(struct pgreqd_register));
  unsigned long remaining = req->size - sizeof(struct pgreqd_register);
  reqarg(register);

  memset(&i,0,sizeof(i));

  /* Note: most of this doesn't work yet, just trying to
     get a framework in place so apps written now won't
     break later.
  */

  i.owner = owner;
  i.name = ntohl(arg->name);
  i.type = ntohs(arg->type);

  i.side = PG_S_TOP;
  i.sidemask = 0xFFFF;
  i.w = 10000;           /* !!! like i said... */
  i.h = 10000;
  i.minw = -1;
  i.maxw = -1;
  i.minh = -1;
  i.maxh = -1;

  /* Process APPSPECs */
  for (;remaining >= 4;remaining -= 4) {
    short key,value;
    key   = ntohs(*(spec++));
    value = ntohs(*(spec++));

    switch (key) {
      
    case PG_APPSPEC_SIDE:
      i.side = value;
      break;

    case PG_APPSPEC_SIDEMASK:
      i.sidemask = value;
      break;

    case PG_APPSPEC_WIDTH:
      i.w = value;
      break;

    case PG_APPSPEC_HEIGHT:
      i.h = value;
      break;

    case PG_APPSPEC_MINWIDTH:
      i.minw = value;
      break;

    case PG_APPSPEC_MAXWIDTH:
      i.maxw = value;
      break;

    case PG_APPSPEC_MINHEIGHT:
      i.minh = value;
      break;

    case PG_APPSPEC_MAXHEIGHT:
      i.maxh = value;
      break;

    default:
      return mkerror(PG_ERRT_BADPARAM,79);  /*  Bad PG_APPSPEC_*  */

    }
  }

  e = appmgr_register(&i);

  *ret = i.rootw;

  return e;
}

g_error rqh_mkpopup(int owner, struct pgrequest *req,
		  void *data, unsigned long *ret, int *fatal) {
  struct widget *w;
  handle h;
  g_error e;
  reqarg(mkpopup);

  e = create_popup((s16)ntohs(arg->x),(s16)ntohs(arg->y),
		   (s16)ntohs(arg->w),(s16)ntohs(arg->h),&w,owner);
  errorcheck;

  e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
  errorcheck;
  
  *ret = h;

  return success;
}

g_error rqh_sizetext(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct fontdesc *fd;
  char *txt;
  s16 w,h;
  g_error e;
  reqarg(sizetext);

  if (arg->font)
    e = rdhandle((void**) &fd,PG_TYPE_FONTDESC,owner,ntohl(arg->font));
  else
    e = rdhandle((void**) &fd,PG_TYPE_FONTDESC,-1,defaultfont);
  errorcheck;

  e = rdhandle((void**) &txt,PG_TYPE_STRING,owner,ntohl(arg->text));
  errorcheck;

  sizetext(fd,&w,&h,txt);

  /* Pack w and h into ret */
  *ret = (((u32)w)<<16) | h;

  return success;
}

/* This accepts a packet that contains many individual request packets.
   This is for stacking together lots of commands (a batch!) that execute
   in sequence.  If any command fails, its error code is returned and the
   other commands are ignored.  Only the return value from the last
   command is saved.
*/
g_error rqh_batch(int owner, struct pgrequest *req,
		  void *data, unsigned long *ret, int *fatal) {
  int remaining = req->size;
  unsigned char *p = (unsigned char *) data;
  struct pgrequest *subreq;
  void *subdata;
  unsigned char null_save;
  g_error e;
  int padding;

  while (remaining) {
    /* Extract a request header */
    subreq = (struct pgrequest *) p;
    p += sizeof(struct pgrequest);
    remaining -= sizeof(struct pgrequest);    
    if (remaining<0)
      return mkerror(PG_ERRT_BADPARAM,63);

    /* Reorder the bytes in the header */
    subreq->type = ntohs(subreq->type);
    subreq->id = ntohl(subreq->id);
    subreq->size = ntohl(subreq->size);

    /* Extract the data */
    subdata = (void *) p;
    /* The data is padded to a 32-bit boundary */
    p += subreq->size;
    remaining -= subreq->size;
    if (remaining<0)
      return mkerror(PG_ERRT_BADPARAM,64);    

    /* _temporarily_ insert a null terminator (writing over the next
       request header, then putting it back)
    */
    null_save = *p;
    *p = 0;
    
    /* No invalid pointers for you! */
    if (subreq->type>PGREQ_UNDEF) subreq->type = PGREQ_UNDEF;
    
    /* Dispatch to one of the handlers in the table */
    e = (*rqhtab[subreq->type])(owner,subreq,subdata,ret,fatal);
    errorcheck;

    /* Undo the null terminator */
    *p = null_save;

    /* The next packet is padded to a 32-bit boundary */
    if (remaining) {
      padding = ((u32)(p - (u8*)data)) & 3;
      if (padding) {
	padding = 4-padding;
	p += padding;
	remaining -= padding;
	if (remaining<0)
	  remaining = 0;
      }
    }
  }

  return e;
}

g_error rqh_regowner(int owner, struct pgrequest *req,
		    void *data, unsigned long *ret, int *fatal) {
#ifdef CONFIG_NOEXCLUSIVE
   return mkerror(PG_ERRT_BADPARAM,105);
#else
   reqarg(regowner);
   
   switch (ntohs(arg->res)) {
      
    case PG_OWN_KEYBOARD:
      if (keyboard_owner)
	return mkerror(PG_ERRT_BUSY,65);
      keyboard_owner = owner;
      break;

    case PG_OWN_POINTER:
      if (pointer_owner)
	return mkerror(PG_ERRT_BUSY,66);
      pointer_owner = owner;
      break;
      
    case PG_OWN_SYSEVENTS:
      if (sysevent_owner)
	return mkerror(PG_ERRT_BUSY,98);
      sysevent_owner = owner;
      break;

    case PG_OWN_DISPLAY:
      if (display_owner)
	return mkerror(PG_ERRT_BUSY,10);
      display_owner = owner;
      break;
      
    default:
      return mkerror(PG_ERRT_BADPARAM,99);
      break;
   }
   return success;
#endif
}
      
g_error rqh_unregowner(int owner, struct pgrequest *req,
		    void *data, unsigned long *ret, int *fatal) {
#ifdef CONFIG_NOEXCLUSIVE
   return mkerror(PG_ERRT_BADPARAM,105);
#else
   reqarg(regowner);
   
   switch (ntohs(arg->res)) {
      
    case PG_OWN_KEYBOARD:
      if (keyboard_owner==owner)
	keyboard_owner = 0;
      break;

    case PG_OWN_POINTER:
      if (pointer_owner==owner)
	pointer_owner = 0;
      break;
      
    case PG_OWN_SYSEVENTS:
      if (sysevent_owner==owner)
	sysevent_owner = 0;
      break;

    case PG_OWN_DISPLAY:
      if (display_owner==owner) {
	struct divtree *p;

	display_owner = 0;

	/* Force redraw everything */
	p = dts->top;
	while (p) {
	  p->flags |= DIVTREE_ALL_REDRAW;
	  p = p->next;
	}
	update(NULL,1);
      }
      break;

   }
   return success;
#endif
}
      
g_error rqh_setmode(int owner, struct pgrequest *req,
		    void *data, unsigned long *ret, int *fatal) {
   reqarg(setmode);
   
   /* Pass this on to the video subsystem */
   return video_setmode(ntohs(arg->xres),ntohs(arg->yres),
			ntohs(arg->bpp),ntohs(arg->flagmode),
			ntohl(arg->flags));
}

g_error rqh_mkcontext(int owner, struct pgrequest *req,
		      void *data, unsigned long *ret, int *fatal) {
  struct conbuf *cb = find_conbuf(owner);
  if (!cb) return mkerror(PG_ERRT_INTERNAL,69);

  cb->context++;

  return success;
}

g_error rqh_rmcontext(int owner, struct pgrequest *req,
		      void *data, unsigned long *ret, int *fatal) {
  struct conbuf *cb = find_conbuf(owner);
  if (!cb) return mkerror(PG_ERRT_INTERNAL,69);
  if (cb->context<=0) return mkerror(PG_ERRT_BADPARAM,70);

  handle_cleanup(owner,cb->context);
  cb->context--;
  
  return success;
}

g_error rqh_focus(int owner, struct pgrequest *req,
		  void *data, unsigned long *ret, int *fatal) {
  g_error e;
  struct widget *w;
  reqarg(handlestruct);

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,owner,ntohl(arg->h));
  errorcheck;
  
  request_focus(w);

  return success;
}

g_error rqh_getstring(int owner, struct pgrequest *req,
		      void *data, unsigned long *ret, int *fatal) {
  struct pgresponse_data rsp;
  char *string;
  unsigned long size;
  g_error e;
  reqarg(handlestruct);

  e = rdhandle((void**) &string,PG_TYPE_STRING,owner,ntohl(arg->h));
  errorcheck;
  if (!string)
    return mkerror(PG_ERRT_HANDLE,93);   /* Null string in getstring */

  /* Send a PG_RESPONSE_DATA back */
  rsp.type = htons(PG_RESPONSE_DATA);
  rsp.id = htonl(req->id);
  size = strlen(string)+1;
  rsp.size = htonl(size);
  
  *fatal |= send_response(owner,&rsp,sizeof(rsp));  
  *fatal |= send_response(owner,string,size);  
  return ERRT_NOREPLY;
}

g_error rqh_setpayload(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  unsigned long *ppayload;
  g_error e;
  reqarg(setpayload);
  
  e = handle_payload(&ppayload,owner,ntohl(arg->h));
  errorcheck;
  
  *ppayload = ntohl(arg->payload);

  return success;
}

g_error rqh_getpayload(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  unsigned long *ppayload;
  g_error e;
  reqarg(handlestruct);
  
  e = handle_payload(&ppayload,owner,ntohl(arg->h));
  errorcheck;
  
  *ret = *ppayload;

  return success;
}

g_error rqh_mktheme(int owner, struct pgrequest *req,
		    void *data, unsigned long *ret, int *fatal) {
  struct pgmemtheme *th;
  handle h;
  g_error e;
  
  /* Load the theme, and make a handle of it. theme_load does
   * a good job of error checking by itself, so no need
   * for that here
   * (besides, who could hope for a humble request handler to
   * understand a compiled theme file ;-)
   */

  e = theme_load(&h,owner,data,req->size);
  errorcheck;

  *ret = h;

  return success;
}

g_error rqh_mkfillstyle(int owner, struct pgrequest *req,
			void *data, unsigned long *ret, int *fatal) {
  char *buf;
  handle h;
  g_error e;

  /* FIXME: This should perform some sanity checks on the fillstyle,
     at least stack underflow/overflow and invalid opcodes. */

  e = g_malloc((void **) &buf,req->size+sizeof(unsigned long));
  errorcheck;
  *((unsigned long *)buf) = req->size;
  memcpy(buf+sizeof(unsigned long),data,req->size);

  e = mkhandle(&h,PG_TYPE_FILLSTYLE,owner,buf);
  errorcheck;

  *ret = h;
  return success;
}

g_error rqh_writeto(int owner, struct pgrequest *req,
		    void *data, unsigned long *ret, int *fatal) {
  union trigparam tp;
  struct widget *w;
  g_error e;
  reqarg(handlestruct);
  
  tp.stream.size = req->size - 4;
  tp.stream.data = ((unsigned char *)data) + 4;

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,owner,ntohl(arg->h));
  errorcheck;
  send_trigger(w,TRIGGER_STREAM,&tp);

  return success;
}

g_error rqh_updatepart(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  union trigparam tp;
  struct widget *w;
  g_error e;
  reqarg(handlestruct);
  
  e = rdhandle((void**) &w,PG_TYPE_WIDGET,owner,ntohl(arg->h));
  errorcheck;

  update(w->in,1);

  return success;
}

g_error rqh_getmode(int owner, struct pgrequest *req,
		    void *data, unsigned long *ret, int *fatal) {
  struct pgresponse_data rsp;
  struct pgmodeinfo mi;
  g_error e;

  /* Fill in the data structure */
  mi.flags = htonl(vid->flags);
  mi.xres  = htons(vid->xres);
  mi.yres  = htons(vid->yres);
  mi.lxres = htons(vid->lxres);
  mi.lyres = htons(vid->lyres);
  mi.bpp   = htons(vid->bpp);
  mi.dummy = 0;
   
  /* Send a PG_RESPONSE_DATA back */
  rsp.type = htons(PG_RESPONSE_DATA);
  rsp.id = htonl(req->id);
  rsp.size = htonl(sizeof(mi));
  
  *fatal |= send_response(owner,&rsp,sizeof(rsp));  
  *fatal |= send_response(owner,&mi,sizeof(mi));  
  return ERRT_NOREPLY;
}

g_error rqh_render(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  g_error e;
  hwrbitmap dest;
  struct groprender *rend;
  struct gropnode grop;
  int numparams,i;
  u32 *params;
  reqarg(render);

  /* First, validate the destination bitmap */
  if (!arg->dest) {
    /* The client wants to draw directly to vid->display.
     * To do this it must register for exclusive use of the display. */

    if (owner != display_owner)
      return mkerror(PG_ERRT_BUSY,9);   /* Not the display owner */
    dest = vid->display;
  }
  else {
    /* Validate the bitmap handle */

    e = rdhandle((void **) &dest,PG_TYPE_BITMAP,owner,ntohl(arg->dest));
    errorcheck;
  }

  /* Retrieve the groprender, allocating one if necessary */
  VID(bitmap_get_groprender)(dest,&rend);

  /* Construct a gropnode from the supplied parameters */
  memset(&grop,0,sizeof(grop));
  grop.type = ntohl(arg->groptype);
  numparams = (req->size - sizeof(struct pgreqd_render))>>2;
  params = (u32*) (((u8*)data) + sizeof(struct pgreqd_render));
  if (PG_GROP_IS_UNPOSITIONED(grop.type)) {
    if (numparams > NUMGROPPARAMS)
      numparams = NUMGROPPARAMS;
    for (i=0;i<numparams;i++)
      grop.param[i] = ntohl(params[i]);
  }
  else {
    if (numparams<4)
      return mkerror(PG_ERRT_BADPARAM,57);   /* not enough args */
    if (numparams > (NUMGROPPARAMS+4))
      numparams = NUMGROPPARAMS+4;
    grop.r.x = ntohl(params[0]);
    grop.r.y = ntohl(params[1]);
    grop.r.w = ntohl(params[2]);
    grop.r.h = ntohl(params[3]);
    for (i=4;i<numparams;i++)
      grop.param[i-4] = ntohl(params[i]);
  }

  /* Convert color */
  if (grop.type == PG_GROP_SETCOLOR ||
      grop.flags == PG_GROPF_COLORED)
    grop.param[0] = VID(color_pgtohwr)(grop.param[0]);

  /* Take care of nonvisual nodes */
  if (PG_GROP_IS_NONVISUAL(grop.type)) {
    gropnode_nonvisual(rend,&grop);
    return success;
  }

  /* Similar steps as the 'main' rendering loop in render.c */
  gropnode_map(rend,&grop);
  gropnode_clip(rend,&grop);
  gropnode_draw(rend,&grop);

  return success;
}

g_error rqh_newbitmap(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  hwrbitmap bmp;
  handle h;
  g_error e;
  reqarg(newbitmap);
  
  e = VID(bitmap_new) (&bmp,ntohs(arg->width),ntohs(arg->height));
  errorcheck;
  e = mkhandle(&h,PG_TYPE_BITMAP,owner,bmp);
  errorcheck;
  
  *ret = h;
  return success;
}

g_error rqh_thlookup(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  reqarg(thlookup);
  
  *ret = theme_lookup(ntohs(arg->object),ntohs(arg->property));
  return success;
}

g_error rqh_getinactive(int owner, struct pgrequest *req,
			void *data, unsigned long *ret, int *fatal) {
  *ret = inactivity_get();
  return success;
}

g_error rqh_setinactive(int owner, struct pgrequest *req,
			void *data, unsigned long *ret, int *fatal) {
  reqarg(setinactive);
  
  inactivity_set(ntohl(arg->time));
  return success;
}

g_error rqh_drivermsg(int owner, struct pgrequest *req,
		      void *data, unsigned long *ret, int *fatal) {
  reqarg(drivermsg);
  
#ifndef CONFIG_NOCLIENTDRIVERMSG
  drivermessage(ntohl(arg->message),ntohl(arg->param),ret);
#endif
  return success;
}

g_error rqh_loaddriver(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  char *buf;
  handle h;
  g_error e;
  struct inlib *i;

  buf = alloca(req->size+1);
  memcpy(buf,data,req->size);
  buf[req->size] = 0;  /* Null terminate it if it isn't already */

  /* Load the inlib */
  e = load_inlib(find_inputdriver(buf),&i);
  errorcheck;

  e = mkhandle(&h,PG_TYPE_DRIVER,owner,i);
  errorcheck;

  *ret = h;
  return success;
}

g_error rqh_dup(int owner, struct pgrequest *req,
		void *data, unsigned long *ret, int *fatal) {
  handle h;
  g_error e;
  reqarg(handlestruct);

  e = handle_dup(&h,owner,ntohl(arg->h));
  errorcheck;

  *ret = h;
  return success;
}

g_error rqh_chcontext(int owner, struct pgrequest *req,
		      void *data, unsigned long *ret, int *fatal) {
  g_error e;
  reqarg(chcontext);

  return handle_chcontext(ntohl(arg->handle),owner,ntohs(arg->delta));
}

g_error rqh_getfstyle(int owner, struct pgrequest *req,
		      void *data, unsigned long *ret, int *fatal) {
  struct pgresponse_data rsp;
  struct pgdata_getfstyle gfs;
  g_error e;
  int i;
  u16 fontrep;
  struct fontstyle_node *n;
  reqarg(getfstyle);
  memset(&gfs,0,sizeof(gfs));

  /* Iterate to the selected font style */
  for (i=ntohs(arg->index),n=fontstyles;i&&n;i--,n=n->next);

  /* If it's good, return the info. Otherwise, name[0] stays zero */
  if (n) {

    /* Put together representation flags */
    fontrep = 0;
    if (n->normal)
      fontrep |= PG_FR_BITMAP_NORMAL;
    if (n->bold)
      fontrep |= PG_FR_BITMAP_BOLD;
    if (n->italic)
      fontrep |= PG_FR_BITMAP_ITALIC;
    if (n->bolditalic)
      fontrep |= PG_FR_BITMAP_BOLDITALIC;

    strcpy(gfs.name,n->name);
    gfs.size = htons(n->size);
    gfs.fontrep = htons(fontrep);
    gfs.flags = htonl(n->flags);
  }

  /* Send a PG_RESPONSE_DATA back */
  rsp.type = htons(PG_RESPONSE_DATA);
  rsp.id = htonl(req->id);
  rsp.size = htonl(sizeof(gfs));
  *fatal |= send_response(owner,&rsp,sizeof(rsp));  
  *fatal |= send_response(owner,&gfs,sizeof(gfs));  
  return ERRT_NOREPLY;
}

/* Communication between rqh_findwidget and iterator */
handle rqh_findwidget_result;
const char *rqh_findwidget_string;
u32 rqh_findwidget_len;
/* Iterator function for rqh_findwidget() */
g_error rqh_findwidget_iterate(void **p) {
  struct widget *w = (struct widget *) (*p);
  const char *str;
  if (iserror(rdhandle((void**)&str,PG_TYPE_STRING,-1,w->name)) || !str)
    return success;
  if (!strncmp(rqh_findwidget_string,str,rqh_findwidget_len))
    rqh_findwidget_result = hlookup(w,NULL);
  return success;
}
/* Find a widget by name 
 *
 * FIXME: modify handle_iterate so we don't need to use global variables
 *        and hlookup() here.
 */
g_error rqh_findwidget(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  rqh_findwidget_result = 0;
  rqh_findwidget_string = (const char *) data;
  rqh_findwidget_len = req->size;
  handle_iterate(PG_TYPE_WIDGET,&rqh_findwidget_iterate);
  *ret = rqh_findwidget_result;
  return success;
}

g_error rqh_checkevent(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  *ret = check_event(owner);
  return success;
}

g_error rqh_sizebitmap(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  hwrbitmap bit;
  s16 w,h;
  g_error e;
  reqarg(handlestruct);
  
  e = rdhandle((void**) &bit,PG_TYPE_BITMAP,owner,ntohl(arg->h));
  errorcheck;

  e = VID(bitmap_getsize) (bit,&w,&h);
  errorcheck;
  
  /* Pack w and h into ret */
  *ret = (((u32)w)<<16) | h;

  return success;
}

g_error rqh_appmsg(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  g_error e;
  struct widget *w;
  reqarg(handlestruct);

  /* Everything after the handlestruct is data to send */
  data += sizeof(struct pgreqd_handlestruct);
  req->size -= sizeof(struct pgreqd_handlestruct);
  
  /* Is the handle good? */
  e = rdhandle((void **) &w, PG_TYPE_WIDGET,-1,ntohl(arg->h));
  errorcheck;

  /* Send it */
  if (req->size > 0)
    post_event(PG_WE_APPMSG,w,req->size,0,data);

  return success;
}

/* Byte-swap each entry in the array, and prepend the number of entries */
g_error rqh_mkarray(int owner, struct pgrequest *req,
		    void *data, unsigned long *ret, int *fatal) {
  u32 *buf;
  int i;
  handle h;
  g_error e;

  /* Truncate size to a multiple of 4 */
  req->size &= ~3;

  e = g_malloc((void **) &buf, req->size + 4);  /* Additional space for the
						   number of entries in the
						   array */
  errorcheck;
  buf[0] = req->size >> 2;       /* Number of entries */

  /* Swap the data as we copy it */
  for (i=1;i<=buf[0];i++,data+=4)
    buf[i] = ntohl(*(u32*)data);

  e = mkhandle(&h,PG_TYPE_ARRAY,owner,buf);
  errorcheck;

  *ret = h;
  return success;
}

g_error rqh_findthobj(int owner, struct pgrequest *req,
		      void *data, unsigned long *ret, int *fatal) {
  s16 id;

  if (find_named_thobj((const char *) data, &id))
    *ret = id;
  else
    *ret = 0;

  return success;
}

/* The End */








