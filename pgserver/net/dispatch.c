/* $Id: dispatch.c,v 1.30 2001/02/23 05:21:24 micahjd Exp $
 *
 * dispatch.c - Processes and dispatches raw request packets to PicoGUI
 *              This is the layer of network-transparency between the app
 *              and the PicoGUI internal functions. Everything is packaged
 *              in network byte order, and a lot of parameter validation is
 *              performed.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
    rsp.id = htons(req->id);
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
    rsp.id = htons(req->id);
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
  return sucess;
}

g_error rqh_update(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  update(NULL,1);
  return sucess;
}

g_error rqh_mkwidget(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct widget *w,*parent;
  handle h;
  handle xh;
  g_error e;
  reqarg(mkwidget);

  /* Don't allow direct creation of 'special' widgets that must
     be created by other means (app registration, popup boxes)
  */
  switch (ntohs(arg->type)) {
  case PG_WIDGET_PANEL:
  case PG_WIDGET_POPUP:
  case PG_WIDGET_BACKGROUND:
    return mkerror(PG_ERRT_BADPARAM,58);
  }

  e = rdhandle((void**) &parent,PG_TYPE_WIDGET,owner,xh=ntohl(arg->parent));
  errorcheck;
  if (!parent) return mkerror(PG_ERRT_BADPARAM,59);

  /* Don't let an app put stuff outside its root widget */
  if (owner>=0 && parent->isroot && ntohs(arg->rship)!=PG_DERIVE_INSIDE)
    return mkerror(PG_ERRT_BADPARAM,60);

  e = widget_derive(&w,ntohs(arg->type),parent,xh,ntohs(arg->rship),owner);
  errorcheck;

  e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
  errorcheck;
  
  *ret = h;

  return sucess;
}

g_error rqh_mkbitmap(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  hwrbitmap bmp;
  handle h;
  g_error e;
  int w;
  
  /* The file format is autodetected. Formats that can't be detected
     will have seperate functions for loading them (XBM for example) */

  /* So far the only available type is PNM :) */

  /* PNM */
  e = (*vid->bitmap_loadpnm)(&bmp,data,req->size);
  errorcheck;

  e = mkhandle(&h,PG_TYPE_BITMAP,owner,bmp);
  errorcheck;
  
  *ret = h;
  return sucess;
}

g_error rqh_mkfont(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  handle h;
  g_error e;
  reqarg(mkfont);

  e = findfont(&h,owner,arg->name,ntohs(arg->size),ntohl(arg->style));
  errorcheck;

  *ret = h;
  return sucess;
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
  return sucess;
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

  return sucess;
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
  return sucess;
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
  return sucess;
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
  return sucess;
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
    if (q->event==PG_WE_DATA)
      send(owner,q->data,q->param,0);
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

  e = create_popup(ntohs(arg->x),ntohs(arg->y),ntohs(arg->w),ntohs(arg->h),&w,owner);
  errorcheck;

  e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
  errorcheck;
  
  *ret = h;

  return sucess;
}

g_error rqh_sizetext(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct fontdesc *fd;
  char *txt;
  int w,h;
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
  *ret = w<<16 | h;

  return sucess;
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

  while (remaining) {
    /* Extract a request header */
    subreq = (struct pgrequest *) p;
    p += sizeof(struct pgrequest);
    remaining -= sizeof(struct pgrequest);    
    if (remaining<0)
      return mkerror(PG_ERRT_BADPARAM,63);

    /* Reorder the bytes in the header */
    subreq->type = ntohs(subreq->type);
    subreq->id = ntohs(subreq->id);
    subreq->size = ntohl(subreq->size);

    /* Extract the data */
    subdata = (void *) p;
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
	return mkerror(PG_ERRT_BUSY,65);
      pointer_owner = owner;
      break;
      
    case PG_OWN_SYSEVENTS:
      if (sysevent_owner)
	return mkerror(PG_ERRT_BUSY,98);
      sysevent_owner = owner;
      break;
      
    default:
      return mkerror(PG_ERRT_BADPARAM,99);
      break;
   }
   return sucess;
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
   }
   return sucess;
#endif
}
      
g_error rqh_mkcontext(int owner, struct pgrequest *req,
		      void *data, unsigned long *ret, int *fatal) {
  struct conbuf *cb = find_conbuf(owner);
  if (!cb) return mkerror(PG_ERRT_INTERNAL,69);

  cb->context++;

  return sucess;
}

g_error rqh_rmcontext(int owner, struct pgrequest *req,
		      void *data, unsigned long *ret, int *fatal) {
  struct conbuf *cb = find_conbuf(owner);
  if (!cb) return mkerror(PG_ERRT_INTERNAL,69);
  if (cb->context<=0) return mkerror(PG_ERRT_BADPARAM,70);

  handle_cleanup(owner,cb->context);
  cb->context--;
  
  return sucess;
}

g_error rqh_focus(int owner, struct pgrequest *req,
		  void *data, unsigned long *ret, int *fatal) {
  g_error e;
  struct widget *w;
  reqarg(handlestruct);

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,owner,ntohl(arg->h));
  errorcheck;
  
  request_focus(w);

  return sucess;
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
  rsp.id = htons(req->id);

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

  return sucess;
}

g_error rqh_getpayload(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  unsigned long *ppayload;
  g_error e;
  reqarg(handlestruct);
  
  e = handle_payload(&ppayload,owner,ntohl(arg->h));
  errorcheck;
  
  *ret = *ppayload;

  return sucess;
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

  return sucess;
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
  return sucess;
}

/* Little internal helper function for mkmsgdlg */
g_error dlgbtn(int owner, struct widget *tb, handle htb,
	       unsigned long payload, int textproperty, int key) {
  g_error e;
  handle h;
  unsigned long *ppayload;
  struct widget *w;

  e = widget_derive(&w,PG_WIDGET_BUTTON,tb,htb,PG_DERIVE_INSIDE,owner);
  errorcheck;
  e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
  errorcheck;
  e = handle_payload(&ppayload,owner,h);
  errorcheck;
  *ppayload = payload;
  e = widget_set(w,PG_WP_TEXT,theme_lookup(PGTH_O_POPUP_MESSAGEDLG,textproperty));
  errorcheck;
  e = widget_set(w,PG_WP_SIDE,PG_S_RIGHT);
  errorcheck;
  e = widget_set(w,PG_WP_HOTKEY,theme_lookup(PGTH_O_POPUP_MESSAGEDLG,key));
  errorcheck;
}

g_error rqh_mkmsgdlg(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  g_error e;
  handle h,htb;
  struct widget *w,*tb;
  unsigned long flags;
  int bw=0,bh=0;
  struct fontdesc *fd;
  char *str;
  reqarg(mkmsgdlg);

  /* If no flags were specified, use the defaults */
  flags = ntohl(arg->flags);
  if (!flags) flags = PG_MSGBTN_OK;

  /*** Sorta size the popup box... This could definitely be improved :) */
  /* Start with the body text size */
  if (!iserror(rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,
			theme_lookup(PGTH_O_LABEL_DLGTEXT,PGTH_P_FONT)))
      && fd) 
    if (!iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,ntohl(arg->text)))
	&& str)
      sizetext(fd,&bw,&bh,str);
  /* Account for doohickeys */
  bh += theme_lookup(PGTH_O_TOOLBAR,PGTH_P_HEIGHT) << 1;
  /* Any additions the theme may have */
  bw += theme_lookup(PGTH_O_POPUP_MESSAGEDLG,PGTH_P_WIDTH);
  bh += theme_lookup(PGTH_O_POPUP_MESSAGEDLG,PGTH_P_HEIGHT);

  /* The popup box itself */
  e = create_popup(PG_POPUP_CENTER,PG_POPUP_CENTER,bw,bh,&w,owner);
  errorcheck;
  w->in->div->state = PGTH_O_POPUP_MESSAGEDLG;
  e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
  errorcheck;
  *ret = h;

  /* Button toolbar */
  e = widget_derive(&tb,PG_WIDGET_TOOLBAR,w,h,PG_DERIVE_INSIDE,owner);
  errorcheck;
  e = mkhandle(&htb,PG_TYPE_WIDGET,owner,tb);
  errorcheck;
  e = widget_set(tb,PG_WP_SIDE,PG_S_BOTTOM);
  errorcheck;

  /* Text */
  e = widget_derive(&w,PG_WIDGET_LABEL,tb,htb,PG_DERIVE_AFTER,owner);
  errorcheck;
  w->in->div->state = PGTH_O_LABEL_DLGTEXT;
  e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
  errorcheck;
  e = widget_set(w,PG_WP_TEXT,ntohl(arg->text));
  errorcheck;
  e = widget_set(w,PG_WP_SIDE,PG_S_ALL);
  errorcheck;

  /* Title */
  e = widget_derive(&w,PG_WIDGET_LABEL,tb,htb,PG_DERIVE_AFTER,owner);
  errorcheck;
  w->in->div->state = PGTH_O_LABEL_DLGTITLE;
  e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
  errorcheck;
  e = widget_set(w,PG_WP_TRANSPARENT,0);
  errorcheck;
  e = widget_set(w,PG_WP_TEXT,ntohl(arg->title));
  errorcheck;

  /* Buttons */
  if (flags & PG_MSGBTN_YES)
    dlgbtn(owner,tb,htb,PG_MSGBTN_YES,PGTH_P_STRING_YES,PGTH_P_HOTKEY_YES);
  if (flags & PG_MSGBTN_NO)
    dlgbtn(owner,tb,htb,PG_MSGBTN_NO,PGTH_P_STRING_NO,PGTH_P_HOTKEY_NO);
  if (flags & PG_MSGBTN_OK)
    dlgbtn(owner,tb,htb,PG_MSGBTN_OK,PGTH_P_STRING_OK,PGTH_P_HOTKEY_OK);
  if (flags & PG_MSGBTN_CANCEL)
    dlgbtn(owner,tb,htb,PG_MSGBTN_CANCEL,PGTH_P_STRING_CANCEL,PGTH_P_HOTKEY_CANCEL);

  return sucess;
}

/* Create a simple (just menuitems) menu.
 * Accepts an array of string handles as the items,
 * and returns the number of the selected item
 * (numbered starting at 1, 0 is reserved for cancellation)
 */
g_error rqh_mkmenu(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  g_error e;
  char *str;
  struct fontdesc *fd;
  struct widget *w,*wbox;
  handle h,hbox;
  int bhmin,bw,bh,maxw = 0,ttlh = 0;
  int i,num = req->size / 4;
  unsigned long *items = data;
  unsigned long *ppayload;

  /*** Find the maximum width and total height (and convert byte order) */
  e = rdhandle((void **)&fd,PG_TYPE_FONTDESC,-1,
	       theme_lookup(PGTH_O_LABEL_DLGTEXT,PGTH_P_FONT));
  errorcheck;
  bhmin = theme_lookup(PGTH_O_MENUITEM,PGTH_P_HEIGHT);
  for (i=0;i<num;i++) {
    e = rdhandle((void **)&str,PG_TYPE_STRING,-1,items[i] = ntohl(items[i]));
    errorcheck;
    sizetext(fd,&bw,&bh,str);
    if (bw>maxw) maxw = bw;
    if (bhmin>bh) bh = bhmin;
    ttlh += bh;
  }

  /* The popup box itself */
  e = create_popup(PG_POPUP_ATCURSOR,PG_POPUP_ATCURSOR,
		   maxw,ttlh,&wbox,owner);
  errorcheck;
  e = mkhandle(&hbox,PG_TYPE_WIDGET,owner,wbox);
  errorcheck;
  *ret = h;

  /* Items */
  for (i=num-1;i>=0;i--) {
    e = widget_derive(&w,PG_WIDGET_MENUITEM,wbox,hbox,PG_DERIVE_INSIDE,owner);
    errorcheck;
    e = mkhandle(&h,PG_TYPE_WIDGET,owner,w);
    errorcheck;
    e = widget_set(w,PG_WP_TEXT,items[i]);
    errorcheck;

    /* Set payload to it's item number (1-based) */  
    e = handle_payload(&ppayload,owner,h);
    errorcheck;
    *ppayload = i+1;
  }

  return sucess;
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

  return sucess;
}

g_error rqh_updatepart(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  union trigparam tp;
  struct widget *w;
  g_error e;
  reqarg(handlestruct);
  
  e = rdhandle((void**) &w,PG_TYPE_WIDGET,owner,ntohl(arg->h));
  errorcheck;

  /* If the divtree is hidden, don't even bother */
  if (w->dt != dts->top) return sucess;

  update(w->in->div,1);

  return sucess;
}

/* The End */








