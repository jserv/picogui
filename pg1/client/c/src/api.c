/* $Id$
 *
 * api.c - PicoGUI application-level functions not directly related
 *                 to the network. Mostly wrappers around the request packets
 *                 to provide the interface described in client_c.h
 *
 *    This library is in need of an overhaul... (thread support, multiple pgserver..)
 *
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
 * Lalo Martins <lalo@laranja.org>
 * 
 */

#include "clientlib.h"       
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DBG(fmt, args...) printf( "%s: " fmt, __FUNCTION__ , ## args); fflush(stdout)

/******* The simple functions that don't need args or return values */

void pgUpdate(void) {
  _pg_add_request(PGREQ_UPDATE,NULL,0);
  /* Update forces a buffer flush */
  pgFlushRequests();
}

int pgEnterContext(void) {
  _pg_add_request(PGREQ_MKCONTEXT,NULL,0);
  pgFlushRequests();
  return _pg_return.e.retdata;
}

int pgGetContext(void) {
  _pg_add_request(PGREQ_GETCONTEXT,NULL,0);
  pgFlushRequests();
  return _pg_return.e.retdata;
}

void pgLeaveContext(void) {
  _pg_add_request(PGREQ_RMCONTEXT,NULL,0);
}  

void pgSetContext(int id) {
	struct pgreqd_setcontext arg;
	arg.context = htonl(id);

	_pg_add_request(PGREQ_SETCONTEXT, &arg, sizeof(arg));
}

void pgDeleteHandleContext(int id) {
  struct pgreqd_rmcontext arg;
  arg.context = htonl(id);

  _pg_add_request(PGREQ_RMCONTEXT,&arg,sizeof(arg));
}  

pghandle pgDataString(struct pgmemdata obj) {

  /* Error */
  if (!obj.pointer) return 0;

  /* FIXME: I should probably find a way to do this that
     doesn't involve copying the data- probably flushing any
     pending packets, then writing the mmap'd file data directly
     to the socket.

     The current method is memory hungry when dealing with larger files.
  */
  _pg_add_request(PGREQ_MKSTRING,obj.pointer,obj.size);
  _pg_free_memdata(obj);

  pgFlushRequests();
  return _pg_return.e.retdata;
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

  pgFlushRequests();
  return _pg_return.e.retdata;
}

pghandle pgLoadWidgetTemplate(struct pgmemdata obj) {

  /* Error */
  if (!obj.pointer) return 0;

  /* FIXME: I should probably find a way to do this that
     doesn't involve copying the data- probably flushing any
     pending packets, then writing the mmap'd file data directly
     to the socket.

     The current method is memory hungry when dealing with larger files.
  */
  _pg_add_request(PGREQ_MKTEMPLATE,obj.pointer,obj.size);
  _pg_free_memdata(obj);

  pgFlushRequests();
  return _pg_return.e.retdata;
}

u32 pgThemeLookup(s16 object, s16 property) {
  struct pgreqd_thlookup arg;

  arg.object = htons(object);
  arg.property = htons(property);

  _pg_add_request(PGREQ_THLOOKUP,&arg,sizeof(arg));
  pgFlushRequests();
  return _pg_return.e.retdata;
}  

/******* Data loading */

/* Data already loaded in memory */
struct pgmemdata pgFromMemory(void *data,u32 length) {
  static struct pgmemdata x;    /* Maybe make something like this
				   global to use less memory? */
  x.pointer = data;
  x.size = length;
  x.flags = 0;
  return x;
}

/* Data already loaded in memory, need to free it */
struct pgmemdata pgFromTempMemory(void *data,u32 length) {
  static struct pgmemdata x;    /* Maybe make something like this
				   global to use less memory? */
  x.pointer = data;
  x.size = length;
  x.flags = PGMEMDAT_NEED_FREE;;
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

/* Load from an already-opened stream */
struct pgmemdata pgFromStream(FILE *f, u32 length) {
   static struct pgmemdata x;
   
   x.size = length;
   if (!(x.pointer = _pg_malloc(x.size))) {
      x.pointer = NULL;
      return x;
   }
   x.flags = PGMEMDAT_NEED_FREE;
   fread(x.pointer,x.size,1,f);
   
   return x;
}

/******* A little more complex ones, with args */

void pgSetPayload(pghandle object,u32 payload) {
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

void pgSetVideoMode(u16 xres, u16 yres,
		    u16 bpp, u16 flagmode,
		    u32 flags) {
  struct pgreqd_setmode arg;
  arg.xres     = htons(xres);
  arg.yres     = htons(yres);
  arg.bpp      = htons(bpp);
  arg.flagmode = htons(flagmode);
  arg.flags    = htonl(flags);
  _pg_add_request(PGREQ_SETMODE,&arg,sizeof(arg));
}

u32 pgGetPayload(pghandle object) {
  object = htonl(object);
  _pg_add_request(PGREQ_GETPAYLOAD,&object,sizeof(object));
  pgFlushRequests();
  return _pg_return.e.retdata;
}

pghandle pgGetServerRes(u32 res) {
  struct pgreqd_getresource arg;
  arg.id = htonl(res);
  _pg_add_request(PGREQ_GETRESOURCE,&arg,sizeof(arg));
  pgFlushRequests();
  return _pg_return.e.retdata;
}

void pgSetInactivity(u32 time) {
  struct pgreqd_setinactive arg;
  arg.time = htonl(time);
  _pg_add_request(PGREQ_SETINACTIVE,&arg,sizeof(arg));
}

u32 pgGetInactivity(void) {
  _pg_add_request(PGREQ_GETINACTIVE,NULL,0);
  pgFlushRequests();
  return _pg_return.e.retdata;
}

pghandle pgNewCursor(void) {
  _pg_add_request(PGREQ_MKCURSOR,NULL,0);
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
  if (_pghandlerlist) {
     if (_pghandlerlist->widgetkey == object) {
	condemn = _pghandlerlist;
	_pghandlerlist = condemn->next;
	free(condemn);
     }
     n = _pghandlerlist;
     while (n && n->next) {
	if (n->next->widgetkey == object) {
	   condemn = n->next;
	   n->next = condemn->next;
	   free(condemn);
	}
	n = n->next;
     }
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
pghandle pgRegisterApp(s16 type,const char *name, ...) {
  va_list v;
  struct pgreqd_register *arg;
  s16 *spec;
  int numspecs,i;
  pghandle ret;

  /* Are we going to use an applet instead? */
  if (_pg_appletbox) {
    ret = pgNewWidget(PG_WIDGET_BOX,PG_DERIVE_INSIDE,_pg_appletbox);
  }
  else {
    /* Normal app */
   
    /* First just count the number of APPSPECs we have */
    for (va_start(v,name),numspecs=0;va_arg(v,s32);
	 va_arg(v,s32),numspecs++);
    va_end(v);
    #define PGDM_INPUT_RAW        8   //!< Send PG_NWE_PNTR_RAW from the specified widget

    /* Allocate */
    if (!(arg = alloca(sizeof(struct pgreqd_register)+numspecs*4)))
      return;
    /* Move pointer */
    spec = (s16 *)(((char*)arg)+sizeof(struct pgreqd_register));
    
    /* Fill in the required params */
    arg->name = htonl(pgNewString(name));
    arg->type = htons(type);
    arg->dummy = 0;
    
    /* Fill in the optional APPSPEC params */
    for (va_start(v,name),i=numspecs<<1;i;
	 i--,*(spec++)=htons(va_arg(v,s32)));
    va_end(v);

    _pg_add_request(PGREQ_REGISTER,arg,
		    sizeof(struct pgreqd_register)+numspecs*4);
    
    /* Because we need a result now, flush the buffer */
    pgFlushRequests();
    ret = _pg_return.e.retdata;
  }

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_INSIDE;
  _pgdefault_widget = ret;
  
  /* Return the new handle */
  return ret;
}

void  pgSetWidget(pghandle widget, ...) {
  va_list v;
  struct pgreqd_set arg;
  s16 *spec;
  int numspecs,i;

  /* Set defaults values */
  arg.widget = htonl(widget ? widget : _pgdefault_widget);
  arg.dummy = 0;

  va_start(v,widget);
  for (;;) {
    i = (int) va_arg(v,s32);
    if (!i) break;
    arg.property = htons(i);
    arg.glob = htonl(va_arg(v,s32));
    _pg_add_request(PGREQ_SET,&arg,sizeof(arg));
  }
  va_end(v);
}

pghandle pgCreateWidget(s16 type) {
   struct pgreqd_createwidget arg;

   arg.type = htons(type);

  _pg_add_request(PGREQ_CREATEWIDGET,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_AFTER;
  _pgdefault_widget = _pg_return.e.retdata;
  
  /* Return the new handle */
  return _pg_return.e.retdata;
}

pghandle pgNewInFilter(pghandle insert_after, u32 accept_trigs, u32 absorb_trigs) {
   struct pgreqd_mkinfilter arg;

   arg.insert_after = htonl(insert_after);
   arg.accept_trigs = htonl(accept_trigs);
   arg.absorb_trigs = htonl(absorb_trigs);

  _pg_add_request(PGREQ_MKINFILTER,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_AFTER;
  _pgdefault_widget = _pg_return.e.retdata;
  
  /* Return the new handle */
  return _pg_return.e.retdata;
}

void pgAttachWidget(pghandle parent, s16 rship, pghandle widget) {

   struct pgreqd_attachwidget arg;

   arg.widget = htonl(widget);
   arg.parent = htonl(parent);   
   arg.rship  = htons(rship  ? rship  : _pgdefault_rship);
   
  _pg_add_request(PGREQ_ATTACHWIDGET,&arg,sizeof(arg));
}


void pgInFilterSend(union pg_client_trigger *trig) {
  struct pgreqd_infiltersend arg;
  int i;

  /* Convert byte order */
  for (i=0;i<(sizeof(arg.trig.array)/sizeof(arg.trig.array[0]));i++)
    arg.trig.array[i] = htonl(trig->array[i]);

  _pg_add_request(PGREQ_INFILTERSEND,&arg,sizeof(arg));
}


pghandle pgNewWidget(s16 type, s16 rship,pghandle parent) {
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
  pghandle h;

  h = pgCreateWidget(PG_WIDGET_POPUP);
  pgSetWidget(h,
	      PG_WP_ABSOLUTEX, x,
	      PG_WP_ABSOLUTEY, y,
	      PG_WP_WIDTH, width,
	      PG_WP_HEIGHT, height,
	      0);

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_INSIDE;
  _pgdefault_widget = _pg_return.e.retdata;
  
  return h;
}

pghandle pgNewFont(const char *name, s16 size, u32 style) {
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

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  _pg_free_memdata(obj);

  /* Return the property */
  return _pg_return.e.retdata;
}

pghandle pgCreateBitmap(s16 width, s16 height) {
  struct pgreqd_newbitmap arg;

  arg.width = htons(width);
  arg.height = htons(height);

  _pg_add_request(PGREQ_NEWBITMAP,&arg,sizeof(arg));

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

/* Works just like pgNewString :) */
pghandle pgFindWidget(const char* str) {
  if (!str) return 0;

  /* Passing the NULL terminator to the server is redundant.
   * no need for a +1 on that strlen...
   */
  _pg_add_request(PGREQ_FINDWIDGET,(void *) str,strlen(str)+1);

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
}

/* Works just like pgNewString :) */
int pgFindThemeObject(const char* str) {
  if (!str) return 0;

  /* Passing the NULL terminator to the server is redundant.
   * no need for a +1 on that strlen...
   */
  _pg_add_request(PGREQ_FINDTHOBJ,(void *) str,strlen(str));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
}

pghandle pgNewArray(const s32* dat, u16 size) {  
  u16 i;
  s32 *swapped;
  
  /* Swap each entry first */
  swapped = alloca(size * sizeof(s32));
  for (i=0;i<size;i++)
    swapped[i] = htonl(dat[i]);

  _pg_add_request(PGREQ_MKARRAY,(void *) swapped, size * sizeof(s32));  
  pgFlushRequests();  
  return _pg_return.e.retdata;
}  
  
pghandle pgEvalRequest(s16 reqtype, void *data, u32 datasize) {
  _pg_add_request(reqtype,data,datasize);
  pgFlushRequests();
  return _pg_return.e.retdata;
}

s32 pgGetWidget(pghandle widget, s16 property) {
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

pghandle pgTraverseWidget(pghandle widget, int direction, int count) {
  struct pgreqd_traversewgt arg;
  arg.widget = htonl(widget);
  arg.direction = htons(direction);
  arg.count = htons(count);

  _pg_add_request(PGREQ_TRAVERSEWGT,&arg,sizeof(arg));

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

void pgSizeBitmap(int *w, int *h, pghandle bitmap) {
  bitmap = htonl(bitmap);

  _pg_add_request(PGREQ_SIZEBITMAP,&bitmap,sizeof(pghandle));
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

struct pgshmbitmap *pgMakeSHMBitmap(pghandle bitmap) {
  struct pgshmbitmap *s;
  struct pgreqd_mkshmbitmap arg;

  arg.bitmap = htonl(bitmap);
  arg.uid = htonl(getuid());

  _pg_add_request(PGREQ_MKSHMBITMAP,&arg,sizeof(arg));
  pgFlushRequests();
  s = (struct pgshmbitmap *) _pg_return.e.data.data;

  /* Convert to host byte order */
  s->shm_key = ntohl(s->shm_key);
  s->shm_length = ntohl(s->shm_length);
  s->format = ntohl(s->format);
  s->palette = ntohl(s->palette);
  s->width = ntohs(s->width);
  s->height = ntohs(s->height);
  s->bpp = ntohs(s->bpp);
  s->pitch = ntohs(s->pitch);
  s->red_mask = ntohl(s->red_mask);
  s->green_mask = ntohl(s->green_mask);
  s->blue_mask = ntohl(s->blue_mask);
  s->alpha_mask = ntohl(s->alpha_mask);
  s->red_shift = ntohs(s->red_shift);
  s->green_shift = ntohs(s->green_shift);
  s->blue_shift = ntohs(s->blue_shift);
  s->alpha_shift = ntohs(s->alpha_shift);
  s->red_length = ntohs(s->red_length);
  s->green_length = ntohs(s->green_length);
  s->blue_length = ntohs(s->blue_length);
  s->alpha_length = ntohs(s->alpha_length);

  return s;
}

int pgGetFontStyle(s16 index, char *name, u16 *size,
		   u16 *fontrep, u32 *flags) {
  struct pgreqd_getfstyle arg;
  struct pgdata_getfstyle *gfs;

  arg.index = htons(index);
  arg.dummy = 0;
  _pg_add_request(PGREQ_GETFSTYLE,&arg,sizeof(arg));
  pgFlushRequests();
  gfs = (struct pgdata_getfstyle *) _pg_return.e.data.data;
  
  if (name)
    strcpy(name,gfs->name);
  if (size)
    *size = ntohs(gfs->size);
  if (fontrep)
    *fontrep = ntohs(gfs->fontrep);
  if (flags)
    *flags = ntohl(gfs->flags);

  return name[0]!=0;
}

/* Get video mode data */
struct pgmodeinfo *pgGetVideoMode(void) {
  struct pgmodeinfo *mi;
  _pg_add_request(PGREQ_GETMODE,NULL,0);
  pgFlushRequests();
  mi = (struct pgmodeinfo *) _pg_return.e.data.data;   
  
  /* Convert byte order */
  mi->flags = ntohl(mi->flags);
  mi->xres  = ntohs(mi->xres);
  mi->yres  = ntohs(mi->yres);
  mi->lxres = ntohs(mi->lxres);
  mi->lyres = ntohs(mi->lyres);
  mi->bpp   = ntohs(mi->bpp);
   
  return mi;
}

void pgDriverMessage(u32 message, u32 param) {
  struct pgreqd_drivermsg arg;
  arg.message = htonl(message);
  arg.param = htonl(param);
  _pg_add_request(PGREQ_DRIVERMSG,&arg,sizeof(arg));
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

/* Write data to a widget.
 * (for example, a terminal widget)
 */
void pgWriteData(pghandle widget,struct pgmemdata data) {
  u32 *buf;

  /* FIXME: Shouln't be recopying this... */

  if (!data.pointer) return;
  if (!(buf = _pg_malloc(data.size+4))) return;
  *buf = htonl(widget ? widget : _pgdefault_widget);
  memcpy(buf+1,data.pointer,data.size);
  _pg_add_request(PGREQ_WRITEDATA,buf,data.size+4);
  _pg_free_memdata(data);
  free(buf);
}

/* Wrapper around pgWriteData to send a command, for example
 * to a canvas widget. Widget, command, and param number must be followed
 * by the specified number of commands
 */
void pgWriteCmd(pghandle widget, s32 command, s16 numparams, ...) {
   s32 *params;
   s32 *buf;
   u32 bufsize;
   va_list v;
   
   bufsize = (numparams + 2) * sizeof(s32);
   buf = params = _pg_malloc(bufsize);
   if (!buf) return;
   
   *params = htonl(widget ? widget : _pgdefault_widget);
   params++;
   *params = htonl(command);
   params++;
      
   va_start(v,numparams);
   for (;numparams;numparams--) {
      *params = htonl(va_arg(v,s32));
      params++;
   }
   va_end(v);
   
  _pg_add_request(PGREQ_WRITECMD,buf,bufsize);
  free(buf);
}

void pgRender(pghandle bitmap, s16 groptype, ...) {
  struct pgreqd_render *arg;
  u32 *params;
  int size;
  int numparams;
  va_list v;

  /* Allocate the packet on the stack */
  numparams = PG_GROPPARAMS(groptype);
  if (!PG_GROP_IS_UNPOSITIONED(groptype))
    numparams += 4;
  size =  sizeof(struct pgreqd_render) + 4*numparams;
  arg = alloca(size);
  params = (u32 *) (((unsigned char *)arg) + 
			      sizeof(struct pgreqd_render));

  /* Transcribe arguments */
  arg->dest = htonl(bitmap);
  arg->groptype = htonl(groptype);

  va_start(v,groptype);
  for (;numparams;numparams--) {
    *params = htonl(va_arg(v,s32));
    params++;
  }
  va_end(v);
  _pg_add_request(PGREQ_RENDER,arg,size);
}

pghandle pgLoadDriver(const char *name) {
  if (!name) return 0;

  /* Passing the NULL terminator to the server is redundant.
   * no need for a +1 on that strlen...
   */
  _pg_add_request(PGREQ_LOADDRIVER,(void *) name,strlen(name));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
}

pghandle pgDup(pghandle object) {
  object = htonl(object);
  _pg_add_request(PGREQ_DUP,&object,sizeof(object));
  pgFlushRequests();
  return _pg_return.e.retdata;
}

void pgChangeContext(pghandle object, s16 delta) {
  struct pgreqd_chcontext arg;
  arg.handle = htonl(object);
  arg.delta  = htons(delta);
  arg.dummy  = 0;
  _pg_add_request(PGREQ_CHCONTEXT,&arg,sizeof(arg));
}

int pgCheckEvent(void) {
  _pg_add_request(PGREQ_CHECKEVENT,NULL,0);
  pgFlushRequests();
  return _pg_return.e.retdata;
}

void pgEventPoll(void) {
  /* This is like pgEventLoop, but completely nonblocking */
  
  while (pgCheckEvent()) {
    struct pgEvent evt = *pgGetEvent();
    pgDispatchEvent(&evt);
  }
}

/* This is almost exacly like pgWriteData */
void pgAppMessage(pghandle dest, struct pgmemdata data) {
  u32 *buf;

  /* FIXME: Shouln't be recopying this... */

  if (!data.pointer) return;
  if (!(buf = _pg_malloc(data.size+4))) return;
  *buf = htonl(dest);
  memcpy(buf+1,data.pointer,data.size);
  
  _pg_add_request(PGREQ_APPMSG,buf,data.size+4);
  
  _pg_free_memdata(data);
  free(buf);
}

/*
 * Handler for the answer messages sent in response to a 
 * pgSyncAppMessage call.
 * When a widget is bound to this handler, the extra parameter
 * given is the address of a pointer where the answer
 * message must be copied.
 */
static int syncmsg_handler (struct pgEvent * evt)
{
  void ** panswer = (void **) evt->extra;

#ifdef DEBUG  
  DBG ("received APPMSG from 0x%08x\n", evt->from);
  DBG ("extra = 0x%08x\n", evt->extra);
#endif

  if (evt->e.data.size > 0) {
    * panswer = _pg_malloc (evt->e.data.size);
    if (* panswer == NULL) return 0;
    memcpy (* panswer, evt->e.data.pointer, evt->e.data.size);
  }
  
  return 1;
}

void * pgSyncAppMessage (pghandle dest, struct pgmemdata data)
{
  pghandle source;
  void * answer;

  if (data.size < sizeof (pghandle)) return NULL;
  
  source = pgCreateWidget (PG_WIDGET_LABEL);

#ifdef DEBUG  
  DBG ("binding 0x%08x with extra = 0x%08x\n", source, & answer);
#endif
  pgBind (source, PG_WE_APPMSG, syncmsg_handler, & answer);
      
  * ((pghandle *) data.pointer) = htonl (source);

  pgAppMessage (dest, data);
  
  answer = NULL;
  do {
    struct pgEvent evt = * pgGetEvent ();
    pgDispatchEvent (& evt);
  } while (! answer);

  pgDelete (source);

  return answer;
}


/******* Getting key names, or getting the value from the name */

extern char *_pgKeyNames[];

char *pgKeyName (u32 key)
{
  if (key < PGKEY_MAX)
    return _pgKeyNames[key];
  else
    return NULL;
}

/* warning: this is MUCH more expensive, don't abuse it */
u32 pgKeyByName (const char *name)
{
  u32 i;

  for (i = 8; i <= PGKEY_MAX; i++)
    if (strcasecmp (_pgKeyNames[i], name) == 0)
      return i;

  return 0;
}

/* The End */
