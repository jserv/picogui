/* $Id: dispatch.c,v 1.1 2000/11/06 00:31:36 micahjd Exp $
 *
 * dispatch.c - Processes and dispatches raw request packets to PicoGUI
 *              This is the layer of network-transparency between the app
 *              and the PicoGUI internal functions. Everything is packaged
 *              in network byte order, and a lot of parameter validation is
 *              performed.
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

#include <pgserver/pgnet.h>

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
DEF_REQHANDLER(mkpopup)
DEF_REQHANDLER(sizetext)
DEF_REQHANDLER(batch)
DEF_REQHANDLER(grabkbd)
DEF_REQHANDLER(grabpntr)
DEF_REQHANDLER(givekbd)
DEF_REQHANDLER(givepntr)
DEF_REQHANDLER(mkcontext)
DEF_REQHANDLER(rmcontext)
DEF_REQHANDLER(focus)
DEF_REQHANDLER(getstring)
DEF_REQHANDLER(restoretheme)
DEF_REQHANDLER(setpayload)
DEF_REQHANDLER(getpayload)
DEF_REQHANDLER(undef)
g_error (*rqhtab[])(int,struct pgrequest*,void*,unsigned long*,int*) = {
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
  TAB_REQHANDLER(mkpopup)
  TAB_REQHANDLER(sizetext)
  TAB_REQHANDLER(batch)
  TAB_REQHANDLER(grabkbd)
  TAB_REQHANDLER(grabpntr)
  TAB_REQHANDLER(givekbd)
  TAB_REQHANDLER(givepntr)
  TAB_REQHANDLER(mkcontext)
  TAB_REQHANDLER(rmcontext)
  TAB_REQHANDLER(focus)
  TAB_REQHANDLER(getstring)
  TAB_REQHANDLER(restoretheme)
  TAB_REQHANDLER(setpayload)
  TAB_REQHANDLER(getpayload)
  TAB_REQHANDLER(undef)
};

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
  update();
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
  struct bitmap *bmp;
  unsigned char *bits;
  long bitsz;
  handle h;
  g_error e;
  int w;
  reqarg(mkbitmap);
  
  bits = ((unsigned char *)data)+sizeof(struct pgreqd_mkbitmap);
  bitsz = req->size - sizeof(struct pgreqd_mkbitmap);

  if (arg->w && arg->h) {
    /* XBM */
    w = ntohs(arg->w);
    if (w%8)
      w = w/8 + 1;
    else
      w = w/8;
    if (bitsz < (w*ntohs(arg->h)))
      return mkerror(PG_ERRT_BADPARAM,61);
    e = (*vid->bitmap_loadxbm)(&bmp,bits,ntohs(arg->w),ntohs(arg->h),
			       (*vid->color_pgtohwr)(ntohl(arg->fg)),
			       (*vid->color_pgtohwr)(ntohl(arg->bg)));
  }
  else {
    /* PNM */
    e = (*vid->bitmap_loadpnm)(&bmp,bits,bitsz);
  }
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

g_error rqh_setbg(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  reqarg(handlestruct);
  return appmgr_setbg(owner,ntohl(arg->h));
}

g_error rqh_undef(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  return mkerror(PG_ERRT_BADPARAM,62);
}

g_error rqh_in_key(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  reqarg(in_key);
  dispatch_key(ntohl(arg->type),(int) ntohs(arg->key),ntohs(arg->mods));
  return sucess;
}

g_error rqh_in_point(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  reqarg(in_point);
  dispatch_pointing(ntohl(arg->type),ntohs(arg->x),ntohs(arg->y),
		    ntohs(arg->btn));
  return sucess;
}

g_error rqh_themeset(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  reqarg(themeset);

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

g_error rqh_in_direct(int owner, struct pgrequest *req,
		   void *data, unsigned long *ret, int *fatal) {
  reqarg(in_direct);
  dispatch_direct(((char*)arg)+sizeof(struct pgreqd_in_direct),
		  ntohl(arg->param));
  return sucess;
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
  }
  else {
    /* Nop. Off to the waiting list... */
  
    FD_SET(owner,&evtwait);
#ifdef DEBUG
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

g_error rqh_grabkbd(int owner, struct pgrequest *req,
		    void *data, unsigned long *ret, int *fatal) {
  if (keyboard_owner)
    return mkerror(PG_ERRT_BUSY,65);
  keyboard_owner = owner;
  return sucess;
}

g_error rqh_grabpntr(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  if (pointer_owner)
    return mkerror(PG_ERRT_BUSY,65);
  pointer_owner = owner;
  return sucess;
}

g_error rqh_givekbd(int owner, struct pgrequest *req,
		    void *data, unsigned long *ret, int *fatal) {
  if (keyboard_owner!=owner)
    return mkerror(PG_ERRT_BADPARAM,67);
  keyboard_owner = 0;
  return sucess;
}

g_error rqh_givepntr(int owner, struct pgrequest *req,
		     void *data, unsigned long *ret, int *fatal) {
  if (pointer_owner!=owner)
    return mkerror(PG_ERRT_BADPARAM,68);
  pointer_owner = 0;
  return sucess;
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

  /* Send a PG_RESPONSE_DATA back */
  rsp.type = htons(PG_RESPONSE_DATA);
  rsp.id = htons(req->id);

  size = strlen(string)+1;
  rsp.size = htonl(size);
  
  *fatal |= send_response(owner,&rsp,sizeof(rsp));  
  *fatal |= send_response(owner,string,size);  
  return ERRT_NOREPLY;
}

g_error rqh_restoretheme(int owner, struct pgrequest *req,
			 void *data, unsigned long *ret, int *fatal) {
  restoretheme();
  return sucess;
}

g_error rqh_setpayload(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  unsigned long *ppayload;
  g_error e;
  reqarg(setpayload);
  
  e = handle_payload(&ppayload,owner,arg->h);
  errorcheck;
  
  *ppayload = arg->payload;

  return sucess;
}

g_error rqh_getpayload(int owner, struct pgrequest *req,
		       void *data, unsigned long *ret, int *fatal) {
  unsigned long *ppayload;
  g_error e;
  reqarg(handlestruct);
  
  e = handle_payload(&ppayload,owner,arg->h);
  errorcheck;
  
  *ret = *ppayload;

  return sucess;
}

/* The End */








