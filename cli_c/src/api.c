/* $Id: api.c,v 1.40 2002/04/08 23:43:56 micahjd Exp $
 *
 * api.c - PicoGUI application-level functions not directly related
 *                 to the network. Mostly wrappers around the request packets
 *                 to provide the interface described in client_c.h
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 * Thread-safe code added by RidgeRun Inc.
 * Copyright (C) 2001 RidgeRun, Inc.  All rights reserved.
 * pgCreateWidget & pgAttachWidget functionality added by RidgeRun Inc.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DBG(fmt, args...) printf( "%s: " fmt, __FUNCTION__ , ## args); fflush(stdout)

/******* The simple functions that don't need args or return values */

void pgUpdate(void) {
#ifdef ENABLE_THREADING_SUPPORT
   _pg_add_request(PGREQ_UPDATE,NULL,0, -1, 1);
#else   
  _pg_add_request(PGREQ_UPDATE,NULL,0);
  /* Update forces a buffer flush */
  pgFlushRequests();
#endif  
}

int pgEnterContext(void) {
#ifdef ENABLE_THREADING_SUPPORT   
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_MKCONTEXT,NULL,0, -1, 0);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
#else   
  _pg_add_request(PGREQ_MKCONTEXT,NULL,0);
  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
}

void pgLeaveContext(void) {
#ifdef ENABLE_THREADING_SUPPORT   
   _pg_add_request(PGREQ_RMCONTEXT,NULL,0, -1, 0);
#else   
  _pg_add_request(PGREQ_RMCONTEXT,NULL,0);
#endif  
}  

void pgDeleteHandleContext(int id) {
  struct pgreqd_rmcontext arg;
  arg.context = htonl(id);

#ifdef ENABLE_THREADING_SUPPORT   
   _pg_add_request(PGREQ_RMCONTEXT,&arg,sizeof(arg), -1, 0);
#else   
  _pg_add_request(PGREQ_RMCONTEXT,&arg,sizeof(arg));
#endif  
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
#ifdef ENABLE_THREADING_SUPPORT  
  {
    pgClientReturnData retData;
    sem_init(&retData.sem, 0, 0);
    _pg_add_request(PGREQ_MKSTRING,obj.pointer,obj.size, 
		    (unsigned int)&retData, 1);
    _pg_free_memdata(obj);
    sem_wait(&retData.sem);
    return retData.ret.e.retdata;
  }
#else  
  _pg_add_request(PGREQ_MKSTRING,obj.pointer,obj.size);
  _pg_free_memdata(obj);

  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
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
#ifdef ENABLE_THREADING_SUPPORT  
  {
    pgClientReturnData retData;
    sem_init(&retData.sem, 0, 0);
    _pg_add_request(PGREQ_MKTHEME,obj.pointer,obj.size, 
		    (unsigned int)&retData, 1);
    _pg_free_memdata(obj);
    sem_wait(&retData.sem);
    return retData.ret.e.retdata;
  }
#else  
  _pg_add_request(PGREQ_MKTHEME,obj.pointer,obj.size);
  _pg_free_memdata(obj);

  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
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
#ifdef ENABLE_THREADING_SUPPORT  
  {
    pgClientReturnData retData;
    sem_init(&retData.sem, 0, 0);
    _pg_add_request(PGREQ_MKTEMPLATE,obj.pointer,obj.size, 
		    (unsigned int)&retData, 1);
    _pg_free_memdata(obj);
    sem_wait(&retData.sem);
    return retData.ret.e.retdata;
  }
#else  
  _pg_add_request(PGREQ_MKTEMPLATE,obj.pointer,obj.size);
  _pg_free_memdata(obj);

  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
}

u32 pgThemeLookup(s16 object, s16 property) {
  struct pgreqd_thlookup arg;

  arg.object = htons(object);
  arg.property = htons(property);

#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_THLOOKUP,&arg,sizeof(arg), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else 
  _pg_add_request(PGREQ_THLOOKUP,&arg,sizeof(arg));
  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
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

#if 0                         /* RESOURCE STUFF IS UNDECIDED SO FAR */
/* Load a resource file 
 *
 * If program is non-null, load from the resource associated with that
 * binary. Otherwise load from ourselves.
 * 
 * name is the name of a resource, like icon/32
 * 
 * Currently this simply corresponds to a file in a .res directory, but
 * this allows for expansion, and possibly some form of virtual file system.
 */
struct pgmemdata pgFromResource(const char *program,const char *name) {
  char *realname;
   
  if (!program)
     program = _pg_appname;
   
  /* Get the app's real name */
  if (lstat(program,&st)<0) {
    /* FIXME: Better error message / a way for the app to catch this error */
    clienterr("Error opening program file in pgFromResource()");
    x.pointer = NULL;
    return x;
  }
  if (S_ISLNK(st.st_mode)) {
     realname = alloca(200);
     tmp = readlink(_pg_appname,realname,200);
     if (tmp<0) {
	_pg_free(realname);
	x.pointer = NULL;    /* This is unlikely */
	return x;
     }
     realname[tmp] = 0;

     /* If that was a relative link, we need to make it absolute */
     if (realname[0] != '/') {
	char *nstr;
	nstr = malloc(strlen(_pg_appname)+strlen(realname)+1);
	strcpy(nstr,_pg_appname);
	*(strrchr(nstr,'/')+1) = 0;
	strcat(nstr,realname);
	free(realname);
	realname = nstr;
     }
  }
   else
     realname = _pg_appname;
}
#endif

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
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_SETPAYLOAD,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_SETPAYLOAD,&arg,sizeof(arg));
#endif  
}

void pgRegisterOwner(int resource) {
  struct pgreqd_regowner arg;
  arg.res = htons(resource);
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_REGOWNER,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_REGOWNER,&arg,sizeof(arg));
#endif  
}

void pgUnregisterOwner(int resource) {
  struct pgreqd_regowner arg;
  arg.res = htons(resource);
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_UNREGOWNER,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_UNREGOWNER,&arg,sizeof(arg));
#endif  
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
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_SETMODE,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_SETMODE,&arg,sizeof(arg));
#endif  
}

void pgSendKeyInput(u32 type, u16 key,
		    u16 mods) {
  struct pgreqd_in_key arg;
  arg.type = htonl(type);
  arg.key  = htons(key);
  arg.mods = htons(mods);
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_IN_KEY,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_IN_KEY,&arg,sizeof(arg));
#endif  
}

/* Also used by networked input devices, but to send pointing device events */
void pgSendPointerInput(u32 type, u16 x, u16 y,
			u16 btn) {
  struct pgreqd_in_point arg;
  arg.type = htonl(type);
  arg.x  = htons(x);
  arg.y  = htons(y);
  arg.btn = htons(btn);
  arg.dummy = 0;
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_IN_POINT,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_IN_POINT,&arg,sizeof(arg));
#endif  
}

u32 pgGetPayload(pghandle object) {
  object = htonl(object);
#ifdef ENABLE_THREADING_SUPPORT
{
  pgClientReturnData retData;
  sem_init(&retData.sem, 0, 0);
  _pg_add_request(PGREQ_GETPAYLOAD,&object,sizeof(object), (unsigned int)&retData, 1);
  sem_wait(&retData.sem);
  return retData.ret.e.retdata;
}
#else 
  _pg_add_request(PGREQ_GETPAYLOAD,&object,sizeof(object));
  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
}

void pgSetInactivity(u32 time) {
  struct pgreqd_setinactive arg;
  arg.time = htonl(time);
#ifdef ENABLE_THREADING_SUPPORT  
  _pg_add_request(PGREQ_SETINACTIVE,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_SETINACTIVE,&arg,sizeof(arg));
#endif  
}

u32 pgGetInactivity(void) {
#ifdef ENABLE_THREADING_SUPPORT
{
  pgClientReturnData retData;
  sem_init(&retData.sem, 0, 0);
  _pg_add_request(PGREQ_GETINACTIVE,NULL,0, (unsigned int)&retData, 1);
  sem_wait(&retData.sem);
  return retData.ret.e.retdata;
}
#else
  _pg_add_request(PGREQ_GETINACTIVE,NULL,0);
  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
}

void pgSubUpdate(pghandle widget) {
  struct pgreqd_handlestruct arg;
  arg.h = htonl(widget ? widget : _pgdefault_widget);
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_UPDATEPART,&arg,sizeof(arg), -1, 1);
#else  
  _pg_add_request(PGREQ_UPDATEPART,&arg,sizeof(arg));
  pgFlushRequests();  
#endif  
}

void pgFocus(pghandle widget) {
  struct pgreqd_handlestruct arg;
  arg.h = htonl(widget ? widget : _pgdefault_widget);
#ifdef ENABLE_THREADING_SUPPORT
   _pg_add_request(PGREQ_FOCUS,&arg,sizeof(arg), -1, 0);
#else   
  _pg_add_request(PGREQ_FOCUS,&arg,sizeof(arg));
#endif  
}

void pgDelete(pghandle object) {
  struct pgreqd_handlestruct arg;
  struct _pghandlernode *n,*condemn = NULL;
  
  /* Ignore if the object is 0 */
  if (!object) return;

  arg.h = htonl(object);
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_FREE,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_FREE,&arg,sizeof(arg));
#endif  

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

#ifdef ENABLE_THREADING_SUPPORT
{
    pgClientReturnData retData;
    sem_init(&retData.sem, 0, 0);
    _pg_add_request(PGREQ_REGISTER,arg, sizeof(struct pgreqd_register)+numspecs*4, (unsigned int)&retData, 1);
    sem_wait(&retData.sem);
    ret = retData.ret.e.retdata;
}
#else 
    _pg_add_request(PGREQ_REGISTER,arg,
		    sizeof(struct pgreqd_register)+numspecs*4);
    
    /* Because we need a result now, flush the buffer */
    pgFlushRequests();
    ret = _pg_return.e.retdata;
#endif
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
#ifdef ENABLE_THREADING_SUPPORT    
    _pg_add_request(PGREQ_SET,&arg,sizeof(arg), -1, 0);
#else    
    _pg_add_request(PGREQ_SET,&arg,sizeof(arg));
#endif    
  }
  va_end(v);
}

pghandle pgCreateWidget(s16 type) {
   struct pgreqd_createwidget arg;

   arg.type = htons(type);

#ifdef ENABLE_THREADING_SUPPORT  
{
  pgClientReturnData retData;
  sem_init(&retData.sem, 0, 0);
  _pg_add_request(PGREQ_CREATEWIDGET,&arg,sizeof(arg), (unsigned int)&retData, 1);
  sem_wait(&retData.sem);
  _pgdefault_rship = PG_DERIVE_AFTER;
  _pgdefault_widget = retData.ret.e.retdata;
  return retData.ret.e.retdata;
}
#else
  _pg_add_request(PGREQ_CREATEWIDGET,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_AFTER;
  _pgdefault_widget = _pg_return.e.retdata;
  
  /* Return the new handle */
  return _pg_return.e.retdata;
#endif  
}

void pgAttachWidget(pghandle parent, s16 rship, pghandle widget) {

   struct pgreqd_attachwidget arg;

   arg.widget = htonl(widget);
   arg.parent = htonl(parent ? parent : _pgdefault_widget);   
   arg.rship  = htons(rship  ? rship  : _pgdefault_rship);
   
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_ATTACHWIDGET,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_ATTACHWIDGET,&arg,sizeof(arg));
#endif  
   
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

#ifdef ENABLE_THREADING_SUPPORT  
{
  pgClientReturnData retData;
  sem_init(&retData.sem, 0, 0);
  _pg_add_request(PGREQ_MKWIDGET,&arg,sizeof(arg), (unsigned int)&retData, 1);
  sem_wait(&retData.sem);
  _pgdefault_rship = PG_DERIVE_AFTER;
  _pgdefault_widget = retData.ret.e.retdata;
  return retData.ret.e.retdata;
}
 
#else
  _pg_add_request(PGREQ_MKWIDGET,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_AFTER;
  _pgdefault_widget = _pg_return.e.retdata;
  
  /* Return the new handle */
  return _pg_return.e.retdata;
#endif  
}

pghandle pgNewPopupAt(int x,int y,int width,int height) {
  struct pgreqd_mkpopup arg;
  arg.x = htons(x);
  arg.y = htons(y);
  arg.w = htons(width);
  arg.h = htons(height);
#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_MKPOPUP,&arg,sizeof(arg), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
    _pgdefault_rship = PG_DERIVE_INSIDE;
   _pgdefault_widget = retData.ret.e.retdata;
   return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_MKPOPUP,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Default is inside this widget */
  _pgdefault_rship = PG_DERIVE_INSIDE;
  _pgdefault_widget = _pg_return.e.retdata;
  
  /* Return the new handle */
  return _pg_return.e.retdata;
#endif  
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
#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_MKFONT,&arg,sizeof(arg), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_MKFONT,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
#endif  
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
#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_MKBITMAP,obj.pointer,obj.size, (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_MKBITMAP,obj.pointer,obj.size);

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the property */
  return _pg_return.e.retdata;
#endif
  
  _pg_free_memdata(obj);
  
}

pghandle pgCreateBitmap(s16 width, s16 height) {
  struct pgreqd_newbitmap arg;

  arg.width = htons(width);
  arg.height = htons(height);

#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_NEWBITMAP,&arg,sizeof(arg), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_NEWBITMAP,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the property */
  return _pg_return.e.retdata;
#endif  
}

pghandle pgNewString(const char* str) {
  if (!str) return 0;

  /* Passing the NULL terminator to the server is redundant.
   * no need for a +1 on that strlen...
   */
#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_MKSTRING,(void *) str,strlen(str), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_MKSTRING,(void *) str,strlen(str));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
#endif  
}

/* Works just like pgNewString :) */
pghandle pgFindWidget(const char* str) {
  if (!str) return 0;

  /* Passing the NULL terminator to the server is redundant.
   * no need for a +1 on that strlen...
   */
#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_FINDWIDGET,(void *) str,strlen(str), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else
  _pg_add_request(PGREQ_FINDWIDGET,(void *) str,strlen(str));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
#endif  
}

/* Works just like pgNewString :) */
int pgFindThemeObject(const char* str) {
  if (!str) return 0;

  /* Passing the NULL terminator to the server is redundant.
   * no need for a +1 on that strlen...
   */
#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_FINDTHOBJ,(void *) str,strlen(str), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else
  _pg_add_request(PGREQ_FINDTHOBJ,(void *) str,strlen(str));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
#endif  
}

pghandle pgNewArray(const s32* dat, u16 size) {  
  u16 i;
  s32 *swapped;
  
  /* Swap each entry first */
  swapped = alloca(size * sizeof(s32));
  for (i=0;i<size;i++)
    swapped[i] = htonl(dat[i]);

#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_MKARRAY,(void *) swapped, size * sizeof(s32), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
    return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_MKARRAY,(void *) swapped, size * sizeof(s32));  
  pgFlushRequests();  
  return _pg_return.e.retdata;
#endif  
}  
  
pghandle pgEvalRequest(s16 reqtype, void *data, u32 datasize) {
#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(reqtype,data,datasize, (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else   
  _pg_add_request(reqtype,data,datasize);
  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
}

s32 pgGetWidget(pghandle widget, s16 property) {
  struct pgreqd_get arg;
  arg.widget = htonl(widget ? widget : _pgdefault_widget);
  arg.property = htons(property);
  arg.dummy = 0;
#ifdef ENABLE_THREADING_SUPPORT
{
  pgClientReturnData retData;
  sem_init(&retData.sem, 0, 0);
  _pg_add_request(PGREQ_GET,&arg,sizeof(arg), (unsigned int)&retData, 1);
  sem_wait(&retData.sem);
  return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_GET,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the property */
  return _pg_return.e.retdata;
#endif  
}

pghandle pgTraverseWidget(pghandle widget, int direction, int count) {
  struct pgreqd_traversewgt arg;
  arg.widget = htonl(widget ? widget : _pgdefault_widget);
  arg.direction = htons(direction);
  arg.count = htons(count);
#ifdef ENABLE_THREADING_SUPPORT
{
  pgClientReturnData retData;
  sem_init(&retData.sem, 0, 0);
  _pg_add_request(PGREQ_TRAVERSEWGT,&arg,sizeof(arg), (unsigned int)&retData, 1);
  sem_wait(&retData.sem);
  return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_TRAVERSEWGT,&arg,sizeof(arg));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the property */
  return _pg_return.e.retdata;
#endif
}

/* Measure a piece of text in a font, in pixels */
void pgSizeText(int *w,int *h,pghandle font,pghandle text) {
  struct pgreqd_sizetext arg;
  arg.text = htonl(text);
  arg.font = htonl(font);
#ifdef ENABLE_THREADING_SUPPORT  
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_SIZETEXT,&arg,sizeof(arg), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   if (w) *w = retData.ret.e.retdata >> 16;
   if (h) *h = retData.ret.e.retdata & 0xFFFF;
}
#else  
  _pg_add_request(PGREQ_SIZETEXT,&arg,sizeof(arg));
   
  /* Get the return value */
  pgFlushRequests();
  if (w) *w = _pg_return.e.retdata >> 16;
  if (h) *h = _pg_return.e.retdata & 0xFFFF;
#endif
  
}

void pgSizeBitmap(int *w, int *h, pghandle bitmap) {
  bitmap = htonl(bitmap);
#ifdef ENABLE_THREADING_SUPPORT    
{
  pgClientReturnData retData;
  sem_init(&retData.sem, 0, 0);
  _pg_add_request(PGREQ_SIZEBITMAP,&bitmap,sizeof(pghandle), (unsigned int)&retData, 1);
  sem_wait(&retData.sem);
  if (w) *w = retData.ret.e.retdata >> 16;
  if (h) *h = retData.ret.e.retdata & 0xFFFF;
}
#else  
  _pg_add_request(PGREQ_SIZEBITMAP,&bitmap,sizeof(pghandle));
  /* Get the return value */
  pgFlushRequests();
  if (w) *w = _pg_return.e.retdata >> 16;
  if (h) *h = _pg_return.e.retdata & 0xFFFF;
#endif  
}
   
/* Get the contents of a string handle. */
char *pgGetString(pghandle string) {
  string = htonl(string);
#ifdef ENABLE_THREADING_SUPPORT      
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_GETSTRING,&string,sizeof(pghandle), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.data.data;
}
#else
  _pg_add_request(PGREQ_GETSTRING,&string,sizeof(pghandle));
  pgFlushRequests();
  return _pg_return.e.data.data;
#endif  
}

int pgGetFontStyle(s16 index, char *name, u16 *size,
		   u16 *fontrep, u32 *flags) {
  struct pgreqd_getfstyle arg;
  struct pgdata_getfstyle *gfs;

  arg.index = htons(index);
  arg.dummy = 0;
#ifdef ENABLE_THREADING_SUPPORT        
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_GETFSTYLE,&arg,sizeof(arg), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   gfs = (struct pgdata_getfstyle *) retData.ret.e.data.data;
}
#else  
  _pg_add_request(PGREQ_GETFSTYLE,&arg,sizeof(arg));
  pgFlushRequests();
  gfs = (struct pgdata_getfstyle *) _pg_return.e.data.data;
#endif
  
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
#ifdef ENABLE_THREADING_SUPPORT          
{
    pgClientReturnData retData;
    sem_init(&retData.sem, 0, 0);
    _pg_add_request(PGREQ_GETMODE,NULL,0, (unsigned int)&retData, 1);
    sem_wait(&retData.sem);
    mi = (struct pgmodeinfo *) retData.ret.e.data.data;
}
#else
  _pg_add_request(PGREQ_GETMODE,NULL,0);
  pgFlushRequests();
  mi = (struct pgmodeinfo *) _pg_return.e.data.data;   
#endif
  
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
#ifdef ENABLE_THREADING_SUPPORT            
  _pg_add_request(PGREQ_DRIVERMSG,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_DRIVERMSG,&arg,sizeof(arg));
#endif  
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
#ifdef ENABLE_THREADING_SUPPORT              
  _pg_add_request(PGREQ_WRITETO,buf,data.size+4, -1, 0);
#else  
  _pg_add_request(PGREQ_WRITETO,buf,data.size+4);
#endif
  _pg_free_memdata(data);
  free(buf);
}

/* Wrapper around pgWriteData to send a command, for example
 * to a canvas widget. Widget, command, and param number must be followed
 * by the specified number of commands
 */
void pgWriteCmd(pghandle widget,s16 command, s16 numparams, ...) {
   struct pgcommand *hdr;
   s32 *params;
   u32 bufsize;
   char *buf;
   va_list v;
   
   bufsize = numparams * sizeof(s32) + sizeof(struct pgcommand);
   buf = alloca(bufsize);
   hdr = (struct pgcommand *) buf;
   params = (s32 *) (buf + sizeof(struct pgcommand));
   
   hdr->command = htons(command);
   hdr->numparams = htons(numparams);
      
   va_start(v,numparams);
   for (;numparams;numparams--) {
      *params = htonl(va_arg(v,s32));
      params++;
   }
   va_end(v);
   
   pgWriteData(widget,pgFromMemory(buf,bufsize));
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
#ifdef ENABLE_THREADING_SUPPORT                
  _pg_add_request(PGREQ_RENDER,arg,size, -1, 0);
#else  
  _pg_add_request(PGREQ_RENDER,arg,size);
#endif  
}

pghandle pgLoadDriver(const char *name) {
  if (!name) return 0;

  /* Passing the NULL terminator to the server is redundant.
   * no need for a +1 on that strlen...
   */
#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_LOADDRIVER,(void *) name,strlen(name), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_LOADDRIVER,(void *) name,strlen(name));

  /* Because we need a result now, flush the buffer */
  pgFlushRequests();

  /* Return the new handle */
  return _pg_return.e.retdata;
#endif  
}

pghandle pgDup(pghandle object) {
  object = htonl(object);
#ifdef ENABLE_THREADING_SUPPORT
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_DUP,&object,sizeof(object), (unsigned int)&retData, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else  
  _pg_add_request(PGREQ_DUP,&object,sizeof(object));
  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
}

void pgChangeContext(pghandle object, s16 delta) {
  struct pgreqd_chcontext arg;
  arg.handle = htonl(object);
  arg.delta  = htons(delta);
  arg.dummy  = 0;
#ifdef ENABLE_THREADING_SUPPORT
  _pg_add_request(PGREQ_CHCONTEXT,&arg,sizeof(arg), -1, 0);
#else  
  _pg_add_request(PGREQ_CHCONTEXT,&arg,sizeof(arg));
#endif  
}

int pgCheckEvent(void) {
#ifdef ENABLE_THREADING_SUPPORT   
{
   pgClientReturnData retData;
   sem_init(&retData.sem, 0, 0);
   _pg_add_request(PGREQ_CHECKEVENT,NULL,0, -1, 1);
   sem_wait(&retData.sem);
   return retData.ret.e.retdata;
}
#else 
  _pg_add_request(PGREQ_CHECKEVENT,NULL,0);
  pgFlushRequests();
  return _pg_return.e.retdata;
#endif  
}

#ifndef ENABLE_THREADING_SUPPORT   
void pgEventPoll(void) {
  /* This is like pgEventLoop, but completely nonblocking */
  
  while (pgCheckEvent()) {
    struct pgEvent evt = *pgGetEvent();
    pgDispatchEvent(&evt);
  }
}
#endif

/* This is almost exacly like pgWriteData */
void pgAppMessage(pghandle dest, struct pgmemdata data) {
  u32 *buf;

  /* FIXME: Shouln't be recopying this... */

  if (!data.pointer) return;
  if (!(buf = _pg_malloc(data.size+4))) return;
  *buf = htonl(dest);
  memcpy(buf+1,data.pointer,data.size);
  
#ifdef ENABLE_THREADING_SUPPORT   
  _pg_add_request(PGREQ_APPMSG,buf,data.size+4, -1, 1);
#else  
  _pg_add_request(PGREQ_APPMSG,buf,data.size+4);
#endif
  
  _pg_free_memdata(data);
  free(buf);
}

/* The End */
