/* $Id: dispatch.c,v 1.16 2000/08/27 05:54:27 micahjd Exp $
 *
 * dispatch.c - Processes and dispatches raw request packets to PicoGUI
 *              This is the layer of network-transparency between the app
 *              and the PicoGUI internal functions. Everything is packaged
 *              in network byte order, and a lot of parameter validation is
 *              performed.
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
  TAB_REQHANDLER(undef)
};

/* Process an incoming packet, and if applicable generate a response packet.
 * Response packets are generated with call(s) to send_response. Returns
 * nonzero if the connection with the client is to be closed.
 */
int dispatch_packet(int from,struct uipkt_request *req,void *data) {
  int fatal=0;
  unsigned long ret_data=0;
  g_error e;

  /* No invalid pointers for you! */
  if (req->type>RQH_UNDEF) req->type = RQH_UNDEF;
  
  /* Dispatch to one of the handlers in the table */
  e = (*rqhtab[req->type])(from,req,data,&ret_data,&fatal);
  
  if (errtype(e) == ERRT_NONE) {
    /* No error, send a return code packet */

    struct response_ret rsp;
    
    rsp.type = htons(RESPONSE_RET);
    rsp.id = htons(req->id);
    rsp.data = htonl(ret_data);
    
    /* Send the return packet */
    fatal |= send_response(from,&rsp,sizeof(rsp));
  }
  else if (e != ERRT_NOREPLY) {
    /* If we need a reply, send error message */

    int errlen;
    struct response_err rsp;
    const char *errmsg;

    errlen = strlen(errmsg = errortext(e));
    
    rsp.type = htons(RESPONSE_ERR);
    rsp.id = htons(req->id);
    rsp.errt = htons(neterrtype(e));
    rsp.msglen = htons(errlen);

    /* Send the error */
    fatal |= send_response(from,&rsp,sizeof(rsp)) | 
             send_response(from,errmsg,errlen);
  }

  return fatal;
}

/***************** Request handlers *******/

g_error rqh_ping(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  return sucess;
}

g_error rqh_update(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  update();
  return sucess;
}

g_error rqh_mkwidget(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_mkwidget *arg = (struct rqhd_mkwidget *) data;
  struct widget *w,*parent;
  handle h;
  handle xh;
  g_error e;

  if (req->size < sizeof(struct rqhd_mkwidget)) 
    return mkerror(ERRT_BADPARAM,57);

  /* Don't allow direct creation of 'special' widgets that must
     be created by other means (app registration, popup boxes)
  */
  switch (ntohs(arg->type)) {
  case WIDGET_PANEL:
  case WIDGET_POPUP:
    return mkerror(ERRT_BADPARAM,58);
  }

  e = rdhandle((void**) &parent,TYPE_WIDGET,owner,xh=ntohl(arg->parent));
  errorcheck;
  if (!parent) return mkerror(ERRT_BADPARAM,59);

  /* Don't let an app put stuff outside its root widget */
  if (owner>=0 && parent->isroot && ntohs(arg->rship)!=DERIVE_INSIDE)
    return mkerror(ERRT_BADPARAM,60);

  e = widget_derive(&w,ntohs(arg->type),parent,xh,ntohs(arg->rship),owner);
  errorcheck;

  e = mkhandle(&h,TYPE_WIDGET,owner,w);
  errorcheck;
  
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
    return mkerror(ERRT_BADPARAM,57);

  bits = ((unsigned char *)data)+sizeof(struct rqhd_mkbitmap);
  bitsz = req->size - sizeof(struct rqhd_mkbitmap);

  if (arg->w && arg->h) {
    /* XBM */
    w = ntohs(arg->w);
    if (w%8)
      w = w/8 + 1;
    else
      w = w/8;
    if (bitsz < (w*ntohs(arg->h)))
      return mkerror(ERRT_BADPARAM,61);
    e = (*vid->bitmap_loadxbm)(&bmp,bits,ntohs(arg->w),ntohs(arg->h),
			       (*vid->color_pgtohwr)(ntohl(arg->fg)),
			       (*vid->color_pgtohwr)(ntohl(arg->bg)));
  }
  else {
    /* PNM */
    e = (*vid->bitmap_loadpnm)(&bmp,bits,bitsz);
  }
  errorcheck;

  e = mkhandle(&h,TYPE_BITMAP,owner,bmp);
  errorcheck;
  
  *ret = h;

  return sucess;
}

g_error rqh_mkfont(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_mkfont *arg = (struct rqhd_mkfont *) data;
  handle h;
  g_error e;

  if (req->size <= sizeof(struct rqhd_mkfont)) 
    return mkerror(ERRT_BADPARAM,57);

  e = findfont(&h,owner,arg->name,ntohs(arg->size),ntohl(arg->style));
  errorcheck;

  *ret = h;
  return sucess;
}

g_error rqh_mkstring(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  char *buf;
  handle h;
  g_error e;

  e = g_malloc((void **) &buf, req->size+1);
  errorcheck;
  memcpy(buf,data,req->size);
  buf[req->size] = 0;  /* Null terminate it if it isn't already */

  e = mkhandle(&h,TYPE_STRING,owner,buf);
  errorcheck;

  *ret = h;
  return sucess;
}

g_error rqh_free(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_free *arg = (struct rqhd_free *) data;
  if (req->size < sizeof(struct rqhd_free)) 
    return mkerror(ERRT_BADPARAM,57);
  
  return handle_free(owner,ntohl(arg->h));
}

g_error rqh_set(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_set *arg = (struct rqhd_set *) data;
  struct widget *w;
  g_error e;

  if (req->size < sizeof(struct rqhd_set)) 
    return mkerror(ERRT_BADPARAM,57);
  e = rdhandle((void**) &w,TYPE_WIDGET,owner,ntohl(arg->widget));
  errorcheck;

  return widget_set(w,ntohs(arg->property),ntohl(arg->glob));
}

g_error rqh_get(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_get *arg = (struct rqhd_get *) data;
  struct widget *w;
  g_error e;

  if (req->size < sizeof(struct rqhd_get)) 
    return mkerror(ERRT_BADPARAM,57);
  e = rdhandle((void**) &w,TYPE_WIDGET,owner,ntohl(arg->widget));
  errorcheck;

  *ret = widget_get(w,ntohs(arg->property));

  return sucess;
}

g_error rqh_setbg(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_setbg *arg = (struct rqhd_setbg *) data;
  if (req->size < sizeof(struct rqhd_setbg)) 
    return mkerror(ERRT_BADPARAM,57);
  
  return appmgr_setbg(owner,ntohl(arg->h));
}

g_error rqh_undef(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  return mkerror(ERRT_BADPARAM,62);
}

g_error rqh_in_key(int owner, struct uipkt_request *req,
		   void *data, unsigned long *ret, int *fatal) {
  struct rqhd_in_key *arg = (struct rqhd_in_key *) data;
  if (req->size < sizeof(struct rqhd_in_key)) 
    return mkerror(ERRT_BADPARAM,57);
  dispatch_key(ntohl(arg->type),(int) ntohs(arg->key),ntohs(arg->mods));
  return sucess;
}

g_error rqh_in_point(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct rqhd_in_point *arg = (struct rqhd_in_point *) data;
  if (req->size < sizeof(struct rqhd_in_point)) 
    return mkerror(ERRT_BADPARAM,57);
  dispatch_pointing(ntohl(arg->type),ntohs(arg->x),ntohs(arg->y),
		    ntohs(arg->btn));
  return sucess;
}

g_error rqh_themeset(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct rqhd_themeset *arg = (struct rqhd_themeset *) data;
  if (req->size < sizeof(struct rqhd_themeset)) 
    return mkerror(ERRT_BADPARAM,57);

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
    return mkerror(ERRT_BADPARAM,57);
  dispatch_direct(((char*)arg)+sizeof(struct rqhd_in_direct),
		  ntohl(arg->param));
  return sucess;
}

g_error rqh_wait(int owner, struct uipkt_request *req,
		 void *data, unsigned long *ret, int *fatal) {
  struct event *q;

  /* Is there anything here already? */
  if (q = get_event(owner,1)) {
    struct response_event rsp;

    rsp.type = htons(RESPONSE_EVENT);
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

g_error rqh_register(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct rqhd_register *arg = (struct rqhd_register *) data;
  struct app_info i;
  g_error e;
  memset(&i,0,sizeof(i));
  if (req->size < (sizeof(struct rqhd_register))) 
    return mkerror(ERRT_BADPARAM,57);

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

g_error rqh_mkpopup(int owner, struct uipkt_request *req,
		  void *data, unsigned long *ret, int *fatal) {
  struct rqhd_mkpopup *arg = (struct rqhd_mkpopup *) data;
  struct widget *w;
  handle h;
  g_error e;

  if (req->size < (sizeof(struct rqhd_mkpopup))) 
    return mkerror(ERRT_BADPARAM,57);

  e = create_popup(ntohs(arg->x),ntohs(arg->y),ntohs(arg->w),ntohs(arg->h),&w,owner);
  errorcheck;

  e = mkhandle(&h,TYPE_WIDGET,owner,w);
  errorcheck;
  
  *ret = h;

  return sucess;
}

g_error rqh_sizetext(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct rqhd_sizetext *arg = (struct rqhd_sizetext *) data;
  struct fontdesc *fd;
  char *txt;
  int w,h;
  g_error e;

  if (req->size < (sizeof(struct rqhd_sizetext))) 
    return mkerror(ERRT_BADPARAM,57);

  if (arg->font)
    e = rdhandle((void**) &fd,TYPE_FONTDESC,owner,ntohl(arg->font));
  else
    e = rdhandle((void**) &fd,TYPE_FONTDESC,-1,defaultfont);
  errorcheck;

  e = rdhandle((void**) &txt,TYPE_STRING,owner,ntohl(arg->text));
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
g_error rqh_batch(int owner, struct uipkt_request *req,
		  void *data, unsigned long *ret, int *fatal) {
  int remaining = req->size;
  unsigned char *p = (unsigned char *) data;
  struct uipkt_request *subreq;
  void *subdata;
  unsigned char null_save;
  g_error e;

  while (remaining) {
    /* Extract a request header */
    subreq = (struct uipkt_request *) p;
    p += sizeof(struct uipkt_request);
    remaining -= sizeof(struct uipkt_request);    
    if (remaining<0)
      return mkerror(ERRT_BADPARAM,63);

    /* Reorder the bytes in the header */
    subreq->type = ntohs(subreq->type);
    subreq->id = ntohs(subreq->id);
    subreq->size = ntohl(subreq->size);

    /* Extract the data */
    subdata = (void *) p;
    p += subreq->size;
    remaining -= subreq->size;
    if (remaining<0)
      return mkerror(ERRT_BADPARAM,64);    

    /* _temporarily_ insert a null terminator (writing over the next
       request header, then putting it back)
    */
    null_save = *p;
    *p = 0;
    
    /* No invalid pointers for you! */
    if (subreq->type>RQH_UNDEF) subreq->type = RQH_UNDEF;
    
    /* Dispatch to one of the handlers in the table */
    e = (*rqhtab[subreq->type])(owner,subreq,subdata,ret,fatal);
    errorcheck;

    /* Undo the null terminator */
    *p = null_save;
  }

  return e;
}

g_error rqh_grabkbd(int owner, struct uipkt_request *req,
		    void *data, unsigned long *ret, int *fatal) {
  if (keyboard_owner)
    return mkerror(ERRT_BUSY,65);
  keyboard_owner = owner;
  return sucess;
}

g_error rqh_grabpntr(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  if (pointer_owner)
    return mkerror(ERRT_BUSY,65);
  pointer_owner = owner;
  return sucess;
}

g_error rqh_givekbd(int owner, struct uipkt_request *req,
		    void *data, unsigned long *ret, int *fatal) {
  if (keyboard_owner!=owner)
    return mkerror(ERRT_BADPARAM,67);
  keyboard_owner = 0;
  return sucess;
}

g_error rqh_givepntr(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  if (pointer_owner!=owner)
    return mkerror(ERRT_BADPARAM,68);
  pointer_owner = 0;
  return sucess;
}

g_error rqh_mkcontext(int owner, struct uipkt_request *req,
		      void *data, unsigned long *ret, int *fatal) {
  struct conbuf *cb = find_conbuf(owner);
  if (!cb) return mkerror(ERRT_INTERNAL,69);

  cb->context++;

  return sucess;
}

g_error rqh_rmcontext(int owner, struct uipkt_request *req,
		      void *data, unsigned long *ret, int *fatal) {
  struct conbuf *cb = find_conbuf(owner);
  if (!cb) return mkerror(ERRT_INTERNAL,69);
  if (cb->context<=0) return mkerror(ERRT_BADPARAM,70);

  handle_cleanup(owner,cb->context);
  cb->context--;
  
  return sucess;
}

g_error rqh_focus(int owner, struct uipkt_request *req,
		  void *data, unsigned long *ret, int *fatal) {
  struct rqhd_focus *arg = (struct rqhd_focus *) data;
  g_error e;
  struct widget *w;

  if (req->size < (sizeof(struct rqhd_focus))) 
    return mkerror(ERRT_BADPARAM,57);

  e = rdhandle((void**) &w,TYPE_WIDGET,owner,ntohl(arg->h));
  errorcheck;
  
  request_focus(w);

  return sucess;
}

g_error rqh_getstring(int owner, struct uipkt_request *req,
		      void *data, unsigned long *ret, int *fatal) {
  struct response_data rsp;
  char *string;
  unsigned long size;
  struct rqhd_getstring *arg = (struct rqhd_getstring *) data;
  g_error e;

  if (req->size < (sizeof(struct rqhd_getstring))) 
    return mkerror(ERRT_BADPARAM,57);

  e = rdhandle((void**) &string,TYPE_STRING,owner,ntohl(arg->h));
  errorcheck;

  /* Send a RESPONSE_DATA back */
  rsp.type = htons(RESPONSE_DATA);
  rsp.id = htons(req->id);

  size = strlen(string)+1;
  rsp.size = htonl(size);
  
  *fatal |= send_response(owner,&rsp,sizeof(rsp));  
  *fatal |= send_response(owner,string,size);  
  return ERRT_NOREPLY;
}

g_error rqh_restoretheme(int owner, struct uipkt_request *req,
			 void *data, unsigned long *ret, int *fatal) {
  restoretheme();
  return sucess;
}

/* The End */








