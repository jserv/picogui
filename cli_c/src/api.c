/* $Id: api.c,v 1.2 2001/03/16 04:10:03 micahjd Exp $
 *
 * api.c - PicoGUI application-level functions not directly related
 *                 to the network. Mostly wrappers around the request packets
 *                 to provide the interface described in client_c.h
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
 * Contributors: 
 * Philippe Ney <philippe.ney@smartdata.ch>
 * Brandon Smith <lottabs2@yahoo.com>
 * 
 * 
 */

#include "clientlib.h"

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
  if (!obj.pointer) return 0;

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

/******* A little more complex ones, with args */

void pgSetPayload(pghandle object,unsigned long payload) {
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

void pgSendKeyInput(unsigned long type,unsigned short key,
		    unsigned short mods) {
  struct pgreqd_in_key arg;
  arg.type = htonl(type);
  arg.key  = htons(key);
  arg.mods = htons(mods);
  _pg_add_request(PGREQ_IN_KEY,&arg,sizeof(arg));
}

/* Also used by networked input devices, but to send pointing device events */
void pgSendPointerInput(unsigned long type,unsigned short x,unsigned short y,
			unsigned short btn) {
  struct pgreqd_in_point arg;
  arg.type = htonl(type);
  arg.x  = htons(x);
  arg.y  = htons(y);
  arg.btn = htons(btn);
  arg.dummy = 0;
  _pg_add_request(PGREQ_IN_POINT,&arg,sizeof(arg));
}

unsigned long pgGetPayload(pghandle object) {
  object = htonl(object);
  _pg_add_request(PGREQ_GETPAYLOAD,&object,sizeof(object));
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

  va_start(v,widget);
  for (;;) {
    i = va_arg(v,short);
    if (!i) break;
    arg.property = htons(i);
    arg.glob = htonl(va_arg(v,long));
    _pg_add_request(PGREQ_SET,&arg,sizeof(arg));
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
  if (!obj.pointer) return 0;

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
  while (!(ret = pgGetPayload(pgGetEvent()->from)));

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
    if (*p == '|') i++;
    p++;
  }
  if (!(handletab = alloca(4*i)))
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
  return pgGetPayload(pgGetEvent()->from);
}

/* Write data to a widget.
 * (for example, a terminal widget)
 */
void pgWriteData(pghandle widget,struct pgmemdata data) {
  unsigned long *buf;

  /* FIXME: Shouln't be recopying this... */

  if (!data.pointer) return;
  if (!(buf = _pg_malloc(data.size+4))) return;
  *buf = htonl(widget ? widget : _pgdefault_widget);
  memcpy(buf+1,data.pointer,data.size);

  _pg_add_request(PGREQ_WRITETO,buf,data.size+4);

  _pg_free_memdata(data);
  free(buf);
}

/* Wrapper around pgWriteData to send a command, for example
 * to a canvas widget. Widget, command, and param number must be followed
 * by the specified number of commands
 */
void pgWriteCmd(pghandle widget,short command,short numparams, ...) {
   struct pgcommand *hdr;
   signed long *params;
   unsigned long bufsize;
   char *buf;
   va_list v;
   
   bufsize = numparams * sizeof(signed long) + sizeof(struct pgcommand);
   buf = alloca(bufsize);
   hdr = (struct pgcommand *) buf;
   params = (signed long *) (buf + sizeof(struct pgcommand));
   
   hdr->command = htons(command);
   hdr->numparams = htons(numparams);
      
   va_start(v,numparams);
   for (;numparams;numparams--) {
      *params = htonl(va_arg(v,signed long));
      params++;
   }
   va_end(V);
   
   pgWriteData(widget,pgFromMemory(buf,bufsize));
}

/* The End */
