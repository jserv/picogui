/* $Id$
 *
 * requests.c - Process the requests that form picogui's network
 *              protocol and several file formats.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/timer.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/input.h>
#include <pgserver/svrwt.h>
#include <pgserver/pgstring.h>
#include <pgserver/requests.h>

#include <stdlib.h>	/* alloca */
#include <string.h>	/* strncmp */


/******************************************************** Utilities */

#define DEF_REQHANDLER(n) g_error rqh_##n(struct request_data *r);
#define TAB_REQHANDLER(n) &rqh_##n ,

/* First bring in function prototypes for all handlers */
#define RQH DEF_REQHANDLER
#include "requests.inc"
#undef RQH

/* Then make a table of all defined requests */
#define RQH TAB_REQHANDLER
g_error (*rqhtab[])(struct request_data*) = {
#include "requests.inc"
};
#undef RQH

/* Macro for casting the arguments */
#define reqarg(x) \
  struct pgreqd_##x *arg = (struct pgreqd_##x *) r->in.data; \
  if (r->in.req->size < sizeof(struct pgreqd_##x)) \
     return mkerror(PG_ERRT_BADPARAM,57);   /* Request data too short */


/******************************************************** Request exec */

g_error request_exec(struct request_data *r) {
  g_error e;

  /* Convert request to host byte order */
  r->in.req->type = ntohs(r->in.req->type);
  r->in.req->size = ntohl(r->in.req->size);
  r->in.req->id   = ntohl(r->in.req->id);

  /* Undefined requests are undefined! */
  if (r->in.req->type > PGREQ_UNDEF)
    r->in.req->type = PGREQ_UNDEF;

  /* Dispatch to one of the handlers in the table */
  e = (*rqhtab[r->in.req->type])(r);
 
  /* If the handler didn't already set up a response,
   * generate either an error or return packet.
   */
  if (!r->out.has_response) {
    if (iserror(e)) {
      /* An error occured, set up an error packet */
      const char *errmsg;
      u32 errlen;
      errmsg = errortext(e);
      errlen = strlen(errmsg);
      r->out.response.err.type   = htons(PG_RESPONSE_ERR);
      r->out.response.err.id     = htonl(r->in.req->id);
      r->out.response.err.errt   = htons(errtype(e));
      r->out.response.err.msglen = htons(errlen);
      r->out.response_len = sizeof(r->out.response.err);
      r->out.response_data = errmsg;
      r->out.response_data_len = errlen;
      r->out.has_response = 1;
    }
    else {
      /* No error, send a return code packet */
      r->out.response.ret.type = htons(PG_RESPONSE_RET);
      r->out.response.ret.id   = htonl(r->in.req->id);
      r->out.response.ret.data = htonl(r->out.ret);
      r->out.response_len = sizeof(r->out.response.ret);
      r->out.has_response = 1;
    }
  }

  return e;
}


/******************************************************** Request handlers */

g_error rqh_ping(struct request_data *r) {
  return success;
}

g_error rqh_update(struct request_data *r) {
  /* Turn off DIVNODE_UNDERCONSTRUCTION flags for all
   * divnodes this app owns. This flag ensures that the
   * app's widgets won't actually be displayed until
   * that app calls pgUpdate().
   */
  activate_client_divnodes(r->in.owner);

  /* The layout engine's entry point */
  update(NULL,1);

  return success;
}

g_error rqh_mkwidget(struct request_data *r) {
  struct widget *w = NULL,*parent = NULL;
  handle h;
  handle xh;
  g_error e,etmp;
  reqarg(mkwidget);

  etmp = rdhandle((void**) &parent,PG_TYPE_WIDGET,r->in.owner,xh=ntohl(arg->parent));
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
  if (r->in.owner>=0 && parent->isroot && ntohs(arg->rship)!=PG_DERIVE_INSIDE)
    return mkerror(PG_ERRT_BADPARAM,60);

  e = widget_derive(&w,&h,ntohs(arg->type),parent,xh,ntohs(arg->rship),r->in.owner);
  errorcheck;
  
  r->out.ret = h;

  return success;
}

g_error rqh_createwidget(struct request_data *r) {
  struct widget *w;
  handle h;
  g_error e;
  reqarg(createwidget);

  e = widget_create(&w, &h, ntohs(arg->type), NULL, 0, r->in.owner);
  errorcheck;
  r->out.ret = h;

  return success;
}

g_error rqh_attachwidget(struct request_data *r) {
  struct widget *w,*parent;
  handle xh;
  g_error e,etmp;
  reqarg(attachwidget);

  etmp = rdhandle((void**) &parent,PG_TYPE_WIDGET,r->in.owner,xh=ntohl(arg->parent));
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

  /* Note that if 'parent' is null here, it signifies detaching the widget */

  /* Don't let an app put stuff outside its root widget */
  if (r->in.owner>=0 && parent && parent->isroot && ntohs(arg->rship)!=PG_DERIVE_INSIDE)
    return mkerror(PG_ERRT_BADPARAM,60);

  /* Get the handle to the widget */
  etmp = rdhandle((void **) &w, PG_TYPE_WIDGET, r->in.owner, ntohl(arg->widget));
  if ( iserror(etmp) )
     return etmp;

  if (!w)
    return mkerror(PG_ERRT_BADPARAM,117);   /* Null widget in attachwidget */

  /* Call widget_derive to actually do the attaching.  widget_derive will notice that the widget already
   * has been created and not create it again.  In this case, it will just do the attachment.
   */
  return widget_derive(&w,NULL,w->type,parent,xh,ntohs(arg->rship),r->in.owner);  

}

g_error rqh_mkbitmap(struct request_data *r) {
  hwrbitmap bmp;
  handle h;
  g_error e;
  
  e = VID(bitmap_load) (&bmp,r->in.data,r->in.req->size);
  errorcheck;
  e = mkhandle(&h,PG_TYPE_BITMAP,r->in.owner,bmp);
  errorcheck;
  
  r->out.ret = h;
  return success;
}

g_error rqh_mkfont(struct request_data *r) {
  handle h;
  g_error e;
  struct font_style fs;
  struct font_descriptor *fd;
  reqarg(mkfont);

  memset(&fs,0,sizeof(fs));
  fs.name = arg->name;
  fs.size = ntohs(arg->size);
  fs.style = ntohl(arg->style);

  e = font_descriptor_create(&fd,&fs);
  errorcheck;
  e = mkhandle(&h,PG_TYPE_FONTDESC,r->in.owner,fd);
  errorcheck;

  r->out.ret = h;
  return success;
}

g_error rqh_mkstring(struct request_data *r) {
  struct pgstring *str;
  handle h;
  g_error e;
  int encoding;
  int i;
  u8 *p;

  /* If there's no characters with the high bit set, we'll use
   * ASCII encoding for speed. Otherwise use the full UTF-8.
   */
  encoding = PGSTR_ENCODE_ASCII;
  for (i=0,p=(u8*)r->in.data;i<r->in.req->size;i++,p++)
    if (*p & 0x80) {
      encoding = PGSTR_ENCODE_UTF8;
      break;
    }

  e = pgstring_new(&str, encoding, r->in.req->size, r->in.data);
  errorcheck;
  e = mkhandle(&h,PG_TYPE_PGSTRING,r->in.owner,str);
  errorcheck;

  r->out.ret = h;
  return success;
}

g_error rqh_free(struct request_data *r) {
  reqarg(handlestruct);  
  return handle_free(r->in.owner,ntohl(arg->h));
}

g_error rqh_set(struct request_data *r) {
  struct widget *w;
  g_error e;
  reqarg(set);

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,r->in.owner,ntohl(arg->widget));
  errorcheck;

  /* They gave us a null widget? Weird... */
  if (!w)
    return success;
  
  return widget_set(w,ntohs(arg->property),ntohl(arg->glob));
}

g_error rqh_get(struct request_data *r) {
  struct widget *w;
  g_error e;
  reqarg(get);

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,r->in.owner,ntohl(arg->widget));
  errorcheck;

  r->out.ret = widget_get(w,ntohs(arg->property));

  return success;
}

g_error rqh_undef(struct request_data *r) {
  return mkerror(PG_ERRT_BADPARAM,62);
}

g_error rqh_wait(struct request_data *r) {
  struct event *q;

  /* Is there anything here already? */
  if ((q = get_event(r->in.owner,1))) {

    r->out.response.event.type  = htons(PG_RESPONSE_EVENT);
    r->out.response.event.event = htons(q->event);
    r->out.response.event.from  = htonl(q->from);
    r->out.response.event.param = htonl(q->param);
    r->out.has_response = 1;
    r->out.response_len = sizeof(r->out.response.event);

    if ((q->event & PG_EVENTCODINGMASK) == PG_EVENTCODING_DATA) {
      /* Transfer ownership of the data block over to our caller */
      r->out.response_data = q->data;
      r->out.response_data_len = q->param;
      r->out.free_response_data = 1;
      q->data = NULL;
    }
  }
  else {
    /* Nope. Off to the waiting list... */
    r->out.block = 1;
  }

  return success;
}

g_error rqh_register(struct request_data *r) {
  struct app_info i;
  g_error e;
  s16 *spec = (s16 *)(((char*)r->in.data)+sizeof(struct pgreqd_register));
  u32 remaining = r->in.req->size - sizeof(struct pgreqd_register);
  reqarg(register);

  memset(&i,0,sizeof(i));

  i.owner = r->in.owner;
  i.name = ntohl(arg->name);
  i.type = ntohs(arg->type);

  i.side = theme_lookup(i.type==PG_APP_TOOLBAR ? PGTH_O_TOOLBAR : PGTH_O_PANEL, PGTH_P_SIDE);
  i.sidemask = 0xFFFF;
  i.default_size.w = 0x7FFF;
  i.default_size.h = 0x7FFF;
  i.min_size.w = -1;
  i.max_size.w = -1;
  i.min_size.h = -1;
  i.max_size.h = -1;

  /* Process APPSPECs */
  for (;remaining >= 4;remaining -= 4) {
    s16 key,value;
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
      i.default_size.w = value;
      break;

    case PG_APPSPEC_HEIGHT:
      i.default_size.h = value;
      break;

    case PG_APPSPEC_MINWIDTH:
      i.min_size.w = value;
      break;

    case PG_APPSPEC_MAXWIDTH:
      i.max_size.w = value;
      break;

    case PG_APPSPEC_MINHEIGHT:
      i.min_size.h = value;
      break;

    case PG_APPSPEC_MAXHEIGHT:
      i.max_size.h = value;
      break;

    default:
      return mkerror(PG_ERRT_BADPARAM,79);  /*  Bad PG_APPSPEC_*  */

    }
  }

  e = appmgr_register(&i);

  r->out.ret = i.rootw;

  return e;
}

g_error rqh_sizetext(struct request_data *r) {
  struct font_descriptor *fd;
  struct pgstring *str;
  s16 w,h;
  g_error e;
  reqarg(sizetext);

  if (arg->font)
    e = rdhandle((void**) &fd,PG_TYPE_FONTDESC,r->in.owner,ntohl(arg->font));
  else
    e = rdhandle((void**) &fd,PG_TYPE_FONTDESC,-1,res[PGRES_DEFAULT_FONT]);
  errorcheck;

  e = rdhandle((void**) &str,PG_TYPE_PGSTRING,r->in.owner,ntohl(arg->text));
  errorcheck;

  fd->lib->measure_string(fd,str,0,&w,&h);

  /* Pack w and h into ret */
  r->out.ret = (((u32)w)<<16) | h;

  return success;
}

/* This accepts a packet that contains many individual request packets.
   This is for stacking together lots of commands (a batch!) that execute
   in sequence.  If any command fails, its error code is returned and the
   other commands are ignored.  Only the return value from the last
   command is saved.
*/
g_error rqh_batch(struct request_data *r) {
  s32 remaining = r->in.req->size;
  u8 *p = (u8 *) r->in.data;
  struct request_data subreq;
  g_error e;
  u32 datasize;
  s32 padding;
  memset(&subreq,0,sizeof(subreq));

  while (remaining) {
    /* Free the last request's data if we need to */
    if (subreq.out.free_response_data)
      g_free(subreq.out.response_data);
    memset(&subreq,0,sizeof(subreq));

    /* Extract a request header */
    subreq.in.req = (struct pgrequest *) p;
    p += sizeof(struct pgrequest);
    remaining -= sizeof(struct pgrequest);    
    if (remaining<0)
      return mkerror(PG_ERRT_BADPARAM,63);

    /* Extract the data */
    datasize = ntohl(subreq.in.req->size);
    subreq.in.data = (void *) p;
    subreq.in.owner = r->in.owner;
    p += datasize;
    remaining -= datasize;
    if (remaining<0)
      return mkerror(PG_ERRT_BADPARAM,64);    

    /* Exec this request */
    e = request_exec(&subreq);
    if (iserror(e))
      break;

    /* The next packet is padded to a 32-bit boundary */
    if (remaining) {
      padding = ((u32)(p - (u8*)subreq.in.data)) & 3;
      if (padding) {
	padding = 4-padding;
	p += padding;
	remaining -= padding;
	if (remaining<0)
	  remaining = 0;
      }
    }
  }

  /* use the return value from the last packet we got */
  memcpy(&r->out, &subreq.out, sizeof(r->out));

  return e;
}

g_error rqh_regowner(struct request_data *r) {
#ifdef CONFIG_NOEXCLUSIVE
   return mkerror(PG_ERRT_BADPARAM,105);
#else
   reqarg(regowner);
   
   switch (ntohs(arg->res)) {
      
    case PG_OWN_DISPLAY:
      if (display_owner)
	return mkerror(PG_ERRT_BUSY,10);
      display_owner = r->in.owner;
      break;
      
    default:
      return mkerror(PG_ERRT_BADPARAM,99);
      break;
   }
   return success;
#endif
}
      
g_error rqh_unregowner(struct request_data *r) {
#ifdef CONFIG_NOEXCLUSIVE
   return mkerror(PG_ERRT_BADPARAM,105);
#else
   reqarg(regowner);
   
   switch (ntohs(arg->res)) {
      
    case PG_OWN_DISPLAY:
      if (display_owner==r->in.owner) {
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
      
g_error rqh_setmode(struct request_data *r) {
   reqarg(setmode);
   
   /* Pass this on to the video subsystem */
   return video_setmode(ntohs(arg->xres),ntohs(arg->yres),
			ntohs(arg->bpp),ntohs(arg->flagmode),
			ntohl(arg->flags));
}

g_error rqh_mkcontext(struct request_data *r) {
  struct conbuf *cb = find_conbuf(r->in.owner);
  if (!cb) return mkerror(PG_ERRT_INTERNAL,69);

  /* FIXME: There's a theoretical possibility cb->context will wrap around.
   *        This won't happen when it's used as a stack, but when it's
   *        used for unique IDs it will eventually happen. This should
   *        check to make sure the context is unused before entering it.
   */

  cb->context++;
  r->out.ret = cb->context;

  return success;
}

g_error rqh_rmcontext(struct request_data *r) {
  struct conbuf *cb;
  struct pgreqd_rmcontext *arg = (struct pgreqd_rmcontext *) r->in.data;

  if (r->in.req->size < sizeof(struct pgreqd_rmcontext)) {
    /* We don't have a structure, follow the stack behavior */

    cb = find_conbuf(r->in.owner);
    if (!cb) return mkerror(PG_ERRT_INTERNAL,69);
    if (cb->context<=0) return mkerror(PG_ERRT_BADPARAM,70);
    handle_cleanup(r->in.owner,cb->context);
    cb->context--;
  }
  else {
    /* We do have a structure, only delete one context */

    handle_cleanup(r->in.owner,ntohl(arg->context));
  }
  
  return success;
}

g_error rqh_focus(struct request_data *r) {
  g_error e;
  struct widget *w;
  reqarg(handlestruct);

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,r->in.owner,ntohl(arg->h));
  errorcheck;
  
  request_focus(w);

  return success;
}

g_error rqh_getstring(struct request_data *r) {
  struct pgresponse_data rsp;
  struct pgstring *str, *tmpstr;
  u32 size;
  g_error e;
  reqarg(handlestruct);

  e = rdhandle((void**) &str,PG_TYPE_PGSTRING,r->in.owner,ntohl(arg->h));
  errorcheck;
  if (!str)
    return mkerror(PG_ERRT_HANDLE,93);   /* Null string in getstring */
  
  /* Send a PG_RESPONSE_DATA back */
  r->out.response.data.type = htons(PG_RESPONSE_DATA);
  r->out.response.data.id = htonl(r->in.req->id);
  r->out.response_len = sizeof(r->out.response.data);
  r->out.has_response = 1;

  r->out.response.data.size = htonl(str->num_bytes);
  r->out.response_data_len = str->num_bytes;
  r->out.response_data = str->buffer;
  
  return success;
}

g_error rqh_setpayload(struct request_data *r) {
  u32 *ppayload;
  g_error e;
  reqarg(setpayload);
  
  e = handle_payload(&ppayload,r->in.owner,ntohl(arg->h));
  errorcheck;
  
  *ppayload = ntohl(arg->payload);

  return success;
}

g_error rqh_getpayload(struct request_data *r) {
  u32 *ppayload;
  g_error e;
  reqarg(handlestruct);
  
  e = handle_payload(&ppayload,r->in.owner,ntohl(arg->h));
  errorcheck;
  r->out.ret = *ppayload;

  return success;
}

g_error rqh_mktheme(struct request_data *r) {
  handle h;
  g_error e;
  
  e = theme_load(&h,r->in.owner,r->in.data,r->in.req->size);
  errorcheck;
  r->out.ret = h;
  return success;
}

g_error rqh_mkfillstyle(struct request_data *r) {
  char *buf;
  handle h;
  g_error e;

  e = check_fillstyle(r->in.data, r->in.req->size);
  errorcheck;

  e = g_malloc((void **) &buf,r->in.req->size+sizeof(u32));
  errorcheck;
  *((u32 *)buf) = r->in.req->size;
  memcpy(buf+sizeof(u32),r->in.data,r->in.req->size);

  e = mkhandle(&h,PG_TYPE_FILLSTYLE,r->in.owner,buf);
  errorcheck;

  r->out.ret = h;
  return success;
}

g_error rqh_writecmd(struct request_data *r) {
  union trigparam tp;
  struct widget *w;
  g_error e;
  int i;
  u32 *data;
  u32 h;

  data = (u32 *) r->in.data;

  h = ntohl(data[0]);
  tp.command.command = ntohl(data[1]);
  tp.command.numparams = (r->in.req->size / sizeof(s32)) - 2;
  tp.command.data = data + 2;

  /* Convert parameters */
  for (i=0; i< tp.command.numparams; i++)
    tp.command.data[i] = ntohl(tp.command.data[i]);

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,r->in.owner,h);
  errorcheck;
  send_trigger(w,PG_TRIGGER_COMMAND,&tp);

  return success;
}

g_error rqh_writedata(struct request_data *r) {
  union trigparam tp;
  struct widget *w;
  g_error e;
  reqarg(handlestruct);
  
  tp.stream.size = r->in.req->size - 4;
  tp.stream.data = ((unsigned char *)r->in.data) + 4;

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,r->in.owner,ntohl(arg->h));
  errorcheck;
  send_trigger(w,PG_TRIGGER_STREAM,&tp);

  return success;
}

g_error rqh_updatepart(struct request_data *r) {
  struct widget *w;
  g_error e;
  reqarg(handlestruct);
  
  e = rdhandle((void**) &w,PG_TYPE_WIDGET,r->in.owner,ntohl(arg->h));
  errorcheck;

  update(w->in,1);

  return success;
}

g_error rqh_getmode(struct request_data *r) {
  struct pgmodeinfo *mi;
  g_error e;
  u32 flags = vid->flags;

  /* Let the client see rotation base too
   * (needed for touchscreen calibrators or other programs that access
   * input or video hardware at a low level)
   */
# if defined(CONFIG_ROTATIONBASE_90)
  flags |= PG_VID_ROTBASE90;
# elif defined(CONFIG_ROTATIONBASE_180)
  flags |= PG_VID_ROTBASE180;
# elif defined(CONFIG_ROTATIONBASE_270)
  flags |= PG_VID_ROTBASE270;
# endif

  e = g_malloc((void*)&mi, sizeof(*mi));
  errorcheck;

  /* Fill in the data structure */
  mi->flags = htonl(flags);
  mi->xres  = htons(vid->xres);
  mi->yres  = htons(vid->yres);
  mi->lxres = htons(vid->lxres);
  mi->lyres = htons(vid->lyres);
  mi->bpp   = htons(vid->bpp);
  mi->dummy = 0;
   
  /* Send a PG_RESPONSE_DATA back */
  r->out.response.data.type = htons(PG_RESPONSE_DATA);
  r->out.response.data.id   = htonl(r->in.req->id);
  r->out.response.data.size = htonl(sizeof(*mi));
  r->out.response_len = sizeof(r->out.response.data);
  r->out.response_data = mi;
  r->out.response_data_len = sizeof(*mi);
  r->out.has_response = 1;
  r->out.free_response_data = 1;

  return success;
}

g_error rqh_render(struct request_data *r) {
  g_error e;
  hwrbitmap dest;
  struct groprender *rend;
  struct gropnode grop;
  int numparams,i;
  u32 *params;
  reqarg(render);

  /* First, validate the destination bitmap */
  if (!arg->dest) {
    /* The client wants to draw directly to the screen.
     * To do this it must register for exclusive use of the display. */

    if (r->in.owner != display_owner)
      return mkerror(PG_ERRT_BUSY,9);   /* Not the display r->in.owner */
    dest = VID(window_fullscreen)();
    
    /* Keep quiet if output's disabled */
    if (disable_output)
      return success;
  }
  else {
    /* Validate the bitmap handle */

    e = rdhandle((void **) &dest,PG_TYPE_BITMAP,r->in.owner,ntohl(arg->dest));
    errorcheck;
  }

  /* Retrieve the groprender, allocating one if necessary */
  VID(bitmap_get_groprender)(dest,&rend);

  /* Construct a gropnode from the supplied parameters */
  memset(&grop,0,sizeof(grop));
  grop.type = ntohl(arg->groptype);
  numparams = (r->in.req->size - sizeof(struct pgreqd_render))>>2;
  params = (u32*) (((u8*)r->in.data) + sizeof(struct pgreqd_render));
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

g_error rqh_newbitmap(struct request_data *r) {
  hwrbitmap bmp;
  handle h;
  g_error e;
  reqarg(newbitmap);
  
  e = VID(bitmap_new) (&bmp,ntohs(arg->width),ntohs(arg->height),vid->bpp);
  errorcheck;
  e = mkhandle(&h,PG_TYPE_BITMAP,r->in.owner,bmp);
  errorcheck;
  
  r->out.ret = h;
  return success;
}

g_error rqh_thlookup(struct request_data *r) {
  reqarg(thlookup);
  
  r->out.ret = theme_lookup(ntohs(arg->object),ntohs(arg->property));
  return success;
}

g_error rqh_getinactive(struct request_data *r) {
  r->out.ret = inactivity_get();
  return success;
}

g_error rqh_setinactive(struct request_data *r) {
  reqarg(setinactive);
  
  inactivity_set(ntohl(arg->time));
  return success;
}

g_error rqh_drivermsg(struct request_data *r) {
  reqarg(drivermsg);
  
#ifndef CONFIG_NOCLIENTDRIVERMSG
  drivermessage(ntohl(arg->message),ntohl(arg->param),&r->out.ret);
#endif
  return success;
}

g_error rqh_loaddriver(struct request_data *r) {
  char *buf;
  handle h;
  g_error e;
  struct inlib *i;

  buf = alloca(r->in.req->size+1);
  memcpy(buf,r->in.data,r->in.req->size);
  buf[r->in.req->size] = 0;  /* Null terminate it if it isn't already */

  /* Load the inlib */
  e = load_inlib(find_inputdriver(buf),&i);
  errorcheck;

  e = mkhandle(&h,PG_TYPE_DRIVER,r->in.owner,i);
  errorcheck;

  r->out.ret = h;
  return success;
}

g_error rqh_dup(struct request_data *r) {
  handle h;
  g_error e;
  reqarg(handlestruct);

  e = handle_dup(&h,r->in.owner,ntohl(arg->h));
  errorcheck;

  r->out.ret = h;
  return success;
}

g_error rqh_chcontext(struct request_data *r) {
  reqarg(chcontext);

  return handle_chcontext(ntohl(arg->handle),r->in.owner,ntohs(arg->delta));
}

g_error rqh_getfstyle(struct request_data *r) {
  struct pgresponse_data rsp;
  struct pgdata_getfstyle *gfs;
  struct font_style fs;
  g_error e;
  reqarg(getfstyle);

  e = g_malloc((void**)&gfs, sizeof(*gfs));
  errorcheck;
  memset(gfs,0,sizeof(*gfs));

  font_getstyle(ntohs(arg->index),&fs);

  if (fs.name)
    strncpy(gfs->name,fs.name,sizeof(gfs->name)-1);
  gfs->size    = htons(fs.size);
  gfs->fontrep = htons(fs.representation);
  gfs->flags   = htonl(fs.style);

  /* Send a PG_RESPONSE_DATA back */
  r->out.response.data.type = htons(PG_RESPONSE_DATA);
  r->out.response.data.id   = htonl(r->in.req->id);
  r->out.response.data.size = htonl(sizeof(*gfs));
  r->out.response_len = sizeof(r->out.response.data);
  r->out.response_data = gfs;
  r->out.response_data_len = sizeof(*gfs);
  r->out.has_response = 1;
  r->out.free_response_data = 1;

  return success;
}

g_error rqh_findwidget(struct request_data *r) {
  /* FIXME: Is this unicode safe? */
  handle h = hlookup(widget_find(pgstring_tmpwrap(r->in.data)),NULL);
  r->out.ret = h;
  return success;
}

g_error rqh_checkevent(struct request_data *r) {
  r->out.ret = check_event(r->in.owner);
  return success;
}

g_error rqh_sizebitmap(struct request_data *r) {
  hwrbitmap bit;
  s16 w,h;
  g_error e;
  reqarg(handlestruct);
  
  e = rdhandle((void**) &bit,PG_TYPE_BITMAP,r->in.owner,ntohl(arg->h));
  errorcheck;

  if (bit) {
    
    e = VID(bitmap_getsize) (bit,&w,&h);
    errorcheck;
    
    /* Pack w and h into ret */
    r->out.ret = (((u32)w)<<16) | h;
    
  }
  else
    r->out.ret = 0;

  return success;
}

g_error rqh_appmsg(struct request_data *r) {
  g_error e;
  struct widget *w;
  const u8 *data;
  u32 datasize;
  reqarg(handlestruct);

  /* Everything after the handlestruct is data to send */
  data = (char*)(r->in.data) + sizeof(struct pgreqd_handlestruct);
  datasize = r->in.req->size - sizeof(struct pgreqd_handlestruct);
  
  /* Is the handle good? */
  e = rdhandle((void **) &w, PG_TYPE_WIDGET,-1,ntohl(arg->h));
  errorcheck;

  /* Send it */
  if (datasize > 0)
    post_event(PG_WE_APPMSG,w,datasize,0,data);

  return success;
}

/* Byte-swap each entry in the array, and prepend the number of entries */
g_error rqh_mkarray(struct request_data *r) {
  u32 *buf;
  int i;
  handle h;
  g_error e;
  const u32 *data = (const u32 *) r->in.data;
  u32 arraysize;

  /* Number of 32-bit elements */
  arraysize = r->in.req->size >> 2;

  e = g_malloc((void **) &buf, (arraysize+1) << 2);  /* Additional space for the
							number of entries in the
							array */
  errorcheck;
  buf[0] = arraysize;       /* Number of entries */
  
  /* Swap the data as we copy it */
  for (i=0;i<arraysize;i++)
    buf[i+1] = ntohl(data[i]);

  e = mkhandle(&h,PG_TYPE_ARRAY,r->in.owner,buf);
  errorcheck;

  r->out.ret = h;
  return success;
}

g_error rqh_findthobj(struct request_data *r) {
  s16 id;

  /* FIXME: r->in.data isn't necessarily null-terminated! */
  if (find_named_thobj(pgstring_tmpwrap(r->in.data), &id))
    r->out.ret = id;
  else
    r->out.ret = 0;

  return success;
}

g_error rqh_traversewgt(struct request_data *r) {
  g_error e;
  struct widget *w;
  reqarg(traversewgt);

  e = rdhandle((void**) &w,PG_TYPE_WIDGET,r->in.owner,ntohl(arg->widget));
  errorcheck;
  
  r->out.ret = hlookup(widget_traverse(w, ntohs(arg->direction), ntohs(arg->count)),NULL);
  return success;
}

g_error rqh_mktemplate(struct request_data *r) {
  handle h;
  g_error e;
  
  e = wt_load(&h,r->in.owner,r->in.data,r->in.req->size);
  errorcheck;

  r->out.ret = h;

  return success;
}

g_error rqh_getresource(struct request_data *r) {
  g_error e;
  reqarg(getresource);
  
  if (ntohl(arg->id) >= PGRES_NUM)
    return mkerror(PG_ERRT_BADPARAM, 118);  /* resource identifier out of range */

  r->out.ret = res[ntohl(arg->id)];

  return success;
}

g_error rqh_mkcursor(struct request_data *r) {
  handle h;
  g_error e;
  

#ifdef CONFIG_NOREMOTEINPUT
  return mkerror(PG_ERRT_BADPARAM,104);     /* Remote input devices have been disabled */
#else
  e = cursor_new(NULL,&h,r->in.owner);
  errorcheck;

  r->out.ret = h;

  return success;
#endif
}

g_error rqh_setcontext(struct request_data *r) {
  struct conbuf *cb = find_conbuf(r->in.owner);
  reqarg(setcontext);
  if (!cb) return mkerror(PG_ERRT_INTERNAL,69);

  cb->context = ntohl(arg->context);

  return success;
}

g_error rqh_getcontext(struct request_data *r) {
  struct conbuf *cb = find_conbuf(r->in.owner);
  if (!cb) return mkerror(PG_ERRT_INTERNAL,69);

  r->out.ret = cb->context;

  return success;
}

g_error rqh_mkinfilter(struct request_data *r) {
  g_error e;
  handle h;
  reqarg(mkinfilter);

#ifdef CONFIG_NOREMOTEINPUT
  return mkerror(PG_ERRT_BADPARAM,104);     /* Remote input devices have been disabled */
#else
  e = infilter_client_create(ntohl(arg->insert_after), ntohl(arg->accept_trigs),
			     ntohl(arg->absorb_trigs), &h, r->in.owner);
  errorcheck;
  r->out.ret = h;

  return success;
#endif
}

g_error rqh_infiltersend(struct request_data *r) {
  g_error e;
  reqarg(infiltersend);

#ifdef CONFIG_NOREMOTEINPUT
  return mkerror(PG_ERRT_BADPARAM,104);     /* Remote input devices have been disabled */
#else
  return infilter_client_send(&arg->trig);
#endif
}

g_error rqh_mkshmbitmap(struct request_data *r) {
  struct pgresponse_data rsp;
  hwrbitmap bmp;
  struct pgshmbitmap *shm;
  g_error e;
  reqarg(mkshmbitmap);

  e = rdhandle((void**) &bmp,PG_TYPE_BITMAP,r->in.owner,ntohl(arg->bitmap));
  errorcheck;
  if (!bmp)
    return mkerror(PG_ERRT_HANDLE,3);   /* Null bitmap in mkshmbitmap */

  /* This is implemented as a vidlib function since it depends 
   * on the bitmap format 
   */
  e = g_malloc((void**)&shm, sizeof(*shm));
  errorcheck;
  memset(shm,0,sizeof(*shm));
  e = VID(bitmap_getshm)(bmp, ntohl(arg->uid), shm);
  errorcheck;

  /* Send a PG_RESPONSE_DATA back */
  r->out.response.data.type = htons(PG_RESPONSE_DATA);
  r->out.response.data.id   = htonl(r->in.req->id);
  r->out.response.data.size = htonl(sizeof(*shm));
  r->out.response_len = sizeof(r->out.response.data);
  r->out.response_data = shm;
  r->out.response_data_len = sizeof(*shm);
  r->out.has_response = 1;
  r->out.free_response_data = 1;

  return success;
}

/* The End */








