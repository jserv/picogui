/* $Id: dispatch.c,v 1.6 2000/06/10 01:15:45 micahjd Exp $
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
  
  /* Send an error packet if there was an error */
  if (e.type != ERRT_NONE) {
    int errlen;
    struct response_err rsp;
    errlen = strlen(e.msg);
    
    rsp.type = htons(RESPONSE_ERR);
    rsp.id = htons(req->id);
    rsp.errt = htons(e.type);
    rsp.msglen = htons(errlen);

    /* Send the error */
    fatal |= send_response(from,&rsp,sizeof(rsp)) | 
             send_response(from,e.msg,errlen);
  }
  else if (req->type != RQH_WAIT) {  /*WAIT packet gets no response yet*/
    /* Send a normal response packet */
    struct response_ret rsp;
    
    rsp.type = htons(RESPONSE_RET);
    rsp.id = htons(req->id);
    rsp.data = htonl(ret_data);
    
    /* Send the return packet */
    fatal |= send_response(from,&rsp,sizeof(rsp));
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
    return mkerror(ERRT_BADPARAM,"rqhd_mkwidget too small");

  /* Don't allow direct creation of 'special' widgets that must
     be created by other means (app registration, popup boxes)
  */
  switch (ntohs(arg->type)) {
  case WIDGET_PANEL:
  case WIDGET_POPUP:
    return mkerror(ERRT_BADPARAM,"Cannot create special widget with mkwidget");
  }

  e = rdhandle((void**) &parent,TYPE_WIDGET,owner,xh=ntohl(arg->parent));
  if (e.type != ERRT_NONE) return e;
  if (!parent) return mkerror(ERRT_BADPARAM,"NULL parent widget");

  /* Don't let an app put stuff outside its root widget */
  if (owner>=0 && parent->isroot && ntohs(arg->rship)!=DERIVE_INSIDE)
    return mkerror(ERRT_BADPARAM,
		   "App attempted to derive before or after a root widget");

  e = widget_derive(&w,ntohs(arg->type),parent,xh,ntohs(arg->rship));
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
  dispatch_key(ntohl(arg->type),(int) ntohs(arg->key),ntohs(arg->mods));
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
#ifdef DEBUG
    printf("Client (#%d) added to waiting list\n",owner);
#endif
  return sucess;
}

g_error rqh_register(int owner, struct uipkt_request *req,
		     void *data, unsigned long *ret, int *fatal) {
  struct rqhd_register *arg = (struct rqhd_register *) data;
  struct app_info i;
  g_error e;
  memset(&i,0,sizeof(i));
  if (req->size < (sizeof(struct rqhd_register))) 
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

g_error rqh_mkpopup(int owner, struct uipkt_request *req,
		  void *data, unsigned long *ret, int *fatal) {
  struct rqhd_mkpopup *arg = (struct rqhd_mkpopup *) data;
  struct widget *w;
  handle h;
  g_error e;

  if (req->size < (sizeof(struct rqhd_mkpopup))) 
    return mkerror(ERRT_BADPARAM,"rqhd_mkpopup too small");

  e = create_popup(ntohs(arg->x),ntohs(arg->y),ntohs(arg->w),ntohs(arg->h),&w);
  if (e.type != ERRT_NONE) return e;

  e = mkhandle(&h,TYPE_WIDGET,owner,w);
  if (e.type != ERRT_NONE) return e;
  
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
    return mkerror(ERRT_BADPARAM,"rqhd_sizetext too small");

  if (arg->font)
    e = rdhandle((void**) &fd,TYPE_FONTDESC,owner,ntohl(arg->font));
  else
    e = rdhandle((void**) &fd,TYPE_FONTDESC,-1,defaultfont);
  if (e.type != ERRT_NONE) return e;

  e = rdhandle((void**) &txt,TYPE_STRING,owner,ntohl(arg->text));
  if (e.type != ERRT_NONE) return e;

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
      return mkerror(ERRT_BADPARAM,"Partial request header in batch");

    /* Reorder the bytes in the header */
    subreq->type = ntohs(subreq->type);
    subreq->id = ntohs(subreq->id);
    subreq->size = ntohl(subreq->size);

    /* Extract the data */
    subdata = (void *) p;
    p += subreq->size;
    remaining -= subreq->size;
    if (remaining<0)
      return mkerror(ERRT_BADPARAM,"Partial request data in batch");    

    /* _temporarily_ insert a null terminator (writing over the next
       request header, then putting it back)
    */
    null_save = *p;
    *p = 0;
    
    /* No invalid pointers for you! */
    if (subreq->type>RQH_UNDEF) subreq->type = RQH_UNDEF;
    
    /* Dispatch to one of the handlers in the table */
    e = (*rqhtab[subreq->type])(owner,subreq,subdata,ret,fatal);
    if (e.type!=ERRT_NONE) break;

    /* Undo the null terminator */
    *p = null_save;
  }

  return e;
}

/* The End */








