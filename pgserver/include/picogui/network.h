/* $Id: network.h,v 1.32 2001/07/12 00:17:18 micahjd Exp $
 *
 * picogui/network.h - Structures and constants needed by the PicoGUI client
 *                     library, but not by the application
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
 * 
 * 
 * 
 */

#ifndef _H_PG_NETWORK
#define _H_PG_NETWORK

#define PG_REQUEST_PORT    30450
#define PG_PROTOCOL_VER    0x0005      /* Increment this whenever changes are made */
#define PG_REQUEST_MAGIC   0x31415926

#ifndef PGSERVER
typedef unsigned short u16;
typedef unsigned long  u32;
#endif

/******* Packet structures */

/* Request, the only packet ever sent from client to server */
struct pgrequest {
  u16 type;
  u16 id;  /* Just to make sure requests match up with responses */
  u32 size; /* The request is followed by size bytes of data */
};  

/* various response packets, sent to the client after the 
   server processes a request packet */
 
#define PG_MAX_RESPONSE_SZ 12  /* in bytes */

#define PG_RESPONSE_ERR 1
struct pgresponse_err {
  u16 type;    /* RESPONSE_ERR - error code */
  u16 id;
  u16 errt;
  u16 msglen;  /* Length of following message */
};

#define PG_RESPONSE_RET 2
struct pgresponse_ret {
  u16 type;    /* RESPONSE_RET - return value */
  u16 id;
  u32 data;
};

#define PG_RESPONSE_EVENT 3
struct pgresponse_event {
  u16 type;    /* RESPONSE_EVENT */
  u16 event;
  u32 from;
  u32 param;
  /* If event == PG_WE_DATA, 'param' bytes of data follow */
};

#define PG_RESPONSE_DATA 4
struct pgresponse_data {
  u16 type;    /* RESPONSE_DATA */
  u16 id;
  u32 size;
  /* 'size' bytes of data follow */
};

/* This is sent to the client after establishing a connection */
struct pghello {
  u32  magic;
  u16 protover;
  u16 dummy;   /* padding */
};

/******* Request handlers */

/* Constants for request handlers                                 args  */
#define PGREQ_PING         0   /* Simply returns if server is ok |   none  */
#define PGREQ_UPDATE       1   /* Call update()                  |   none  */
#define PGREQ_MKWIDGET     2   /* Makes a widget, returns handle |  struct */
#define PGREQ_MKBITMAP     3   /* Makes a bitmap, returns handle |  data   */
#define PGREQ_MKFONT       4   /* Makes a fontdesc, ret's handle |  struct */
#define PGREQ_MKSTRING     5   /* Makes a string, returns handle |  chars  */
#define PGREQ_FREE         6   /* Frees a handle                 |  handle */
#define PGREQ_SET          7   /* Set a widget param             |  struct */
#define PGREQ_GET          8   /* Get a widget param, return it  |  struct */
#define PGREQ_MKTHEME      9   /* Load a compiled theme          |  theme  */
#define PGREQ_IN_KEY       10  /* Dispatch keyboard input        |  struct */
#define PGREQ_IN_POINT     11  /* Dispatch pointing device input |  struct */
#define PGREQ_IN_DIRECT    12  /* Dispatch direct input          |  struct */
#define PGREQ_WAIT         13  /* Wait for an event              |  none   */
#define PGREQ_MKFILLSTYLE  14  /* Load a fill style,return handle|  fillstyle */
#define PGREQ_REGISTER     15  /* Register a new application     |  struct */
#define PGREQ_MKPOPUP      16  /* Create a popup root widget     |  struct */
#define PGREQ_SIZETEXT     17  /* Find the size of text          |  struct */
#define PGREQ_BATCH        18  /* Executes many requests         |  requests */
#define PGREQ_REGOWNER     19  /* Get exclusive privileges       |  struct */
#define PGREQ_UNREGOWNER   20  /* Give up exclusive privileges   |  struct */
#define PGREQ_SETMODE      21  /* Sets video mode/depth/rotation |  struct */
#define PGREQ_GETMODE      22  /* Returns a modeinfo struct      |  none */
#define PGREQ_MKCONTEXT    23  /* Enters a new context           |  none */
#define PGREQ_RMCONTEXT    24  /* Cleans up and kills the context|  none */
#define PGREQ_FOCUS        25  /* Force focus to specified widget|  handle */
#define PGREQ_GETSTRING    26  /* Returns a RESPONSE_DATA        |  handle */
#define PGREQ_MKMSGDLG     27  /* Creates a message dialog box   |  struct */
#define PGREQ_SETPAYLOAD   28  /* Sets an object's payload       |  struct */
#define PGREQ_GETPAYLOAD   29  /* Gets an object's payload       |  handle */
#define PGREQ_MKMENU       30  /* Creates a simple popup menu    |  handle[] */
#define PGREQ_WRITETO      31  /* Stream data to a widget        |  handle + data */
#define PGREQ_UPDATEPART   32  /* Updates subtree defined by wgt |  handle */
#define PGREQ_MKARRAY      33  /* Makes a array, returns handle  |    data */  
#define PGREQ_RENDER       34  /* Render gropnode(s) to a bitmap |  struct */
#define PGREQ_NEWBITMAP    35  /* Creates a blank bitmap         |  struct */
#define PGREQ_THLOOKUP     36  /* Perform a theme lookup         |  struct */
#define PGREQ_GETINACTIVE  37  /* get milliseconds of inactivity |    none */
#define PGREQ_SETINACTIVE  38  /* set milliseconds of inactivity |  struct */
#define PGREQ_DRIVERMSG    39  /* Send a message to all drivers  |  struct */
#define PGREQ_LOADDRIVER   40  /* Load input/misc (not video)    |   chars */

#define PGREQ_UNDEF        41  /* types > this will be truncated. return error */

/******* Request data structures */

/* Structures passed to request handlers as 'data'.
 * Dummy variables pad it to a multiple of 4 bytes (compiler likes it?)
 *
 * All numerical values here are in network byte order (converted with
 * ntohl/ntohs/htonl/htons)
 *
 * All values referring to objects (text,widget,font...) are handles
 * (also in network order)
 *
 */

struct pgreqd_handlestruct {
  u32 h;   /* for requests that just use a handle */
};

struct pgreqd_mkwidget {
  u16 rship;
  u16 type;
  u32 parent;
};
struct pgreqd_mkfont {
  char name[40];
  u32 style;
  u16 size;
  u16 dummy;
};
struct pgreqd_set {
  u32 widget;
  u32 glob;
  u16 property;
  u16 dummy;
};
struct pgreqd_get {
  u32 widget;
  u16 property;
  u16 dummy;
};
struct pgreqd_in_key {
  u32 type;   /* A TRIGGER_* constant */
  u16 key;
  u16 mods;
};
struct pgreqd_in_point {
  u32 type;   /* A TRIGGER_* constant */
  u16 x;
  u16 y;
  u16 btn;  /* button bitmask */
  u16 dummy;
};
struct pgreqd_in_direct {
  u32 param;   /* The arbitrary parameter */
  /* The rest of the packet is read as a string */
};
struct pgreqd_themeset {
  u32 value;
  u16 element;
  u16 state;
  u16 param;
  u16 dummy;
};
struct pgreqd_register {
  /* This is just a subset of app_info, organized for network
     transmission */

  u32 name;  /* string handle */
  u16 type;
  u16 dummy;

  /* Followed by optional APPSPECs */
};
struct pgreqd_setmode {
  u16 xres;      /* If these are zero, mode is not changed */
  u16 yres;
  u16 bpp;       /* Zero to leave alone */
  u16 flagmode;  /* A PG_FM_* constant */
  u32 flags;     /* Merged with existing flags according to flagmode */
};
struct pgreqd_mkpopup {
  u16 x; /* can be a PG_POPUP_* constant */
  u16 y; 
  u16 w;
  u16 h;
};
struct pgreqd_sizetext {
  u32 text;  /* Handle to text and to font */
  u32 font;
};
struct pgreqd_setpayload {
  u32 h;        /* Any handle */
  u32 payload;  /* 32-bits of data to store with
			     the handle'd object */
};
struct pgreqd_mkmsgdlg {
  u32 title;
  u32 text;
  u32 flags;
};
struct pgreqd_regowner {
  u16 res;     /* A resource to own: PG_OWN_* */
};
struct pgreqd_render {
  /* Handle of a bitmap to render to. If this is null,
   * _and_ the client has been registered as owning the display,
   * the destination will be vid->display
   */
  u32 dest;
  u32 groptype;    /* PG_GROP_* constant */

  /* Followed by a number of 32-bit parameters.
   * like PGCANVAS_GROP, the first four must be x,y,w,h unless it is an
   * unpositioned gropnode. The rest are treated as gropnode parameters.
   */
};
struct pgreqd_newbitmap {
  u16 width;
  u16 height;
};
struct pgreqd_thlookup {
  u16 object;
  u16 property;
};
struct pgreqd_setinactive {
  u32 time;
};
struct pgreqd_drivermsg {
  u32 message;
  u32 param;
};

/* A structure for encapsulating commands, for example in canvas, within
 * a RQH_WRITETO */
struct pgcommand {
   u16 command;
   u16 numparams;
   /* Followed by numparams * signed long */
};

/* Returned by rqh_getmode */
struct pgmodeinfo {
   u32 flags;
   u16 xres;     /* Physical resolution */
   u16 yres;
   u16 lxres;    /* Logical resolution */
   u16 lyres;
   u16 bpp;  
   u16 dummy;
};
   
#endif /* __H_PG_NETWORK */
/* The End */
