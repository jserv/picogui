/* $Id: network.h,v 1.12 2000/12/12 00:51:47 micahjd Exp $
 *
 * picogui/network.h - Structures and constants needed by the PicoGUI client
 *                     library, but not by the application
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

#ifndef _H_PG_NETWORK
#define _H_PG_NETWORK

#define PG_REQUEST_PORT    30450
#define PG_PROTOCOL_VER    0x0002      /* Increment this whenever changes are made */
#define PG_REQUEST_MAGIC   0x31415926

/******* Packet structures */

/* Request, the only packet ever sent from client to server */
struct pgrequest {
  unsigned short type;
  unsigned short id;  /* Just to make sure requests match up with responses */
  unsigned long size; /* The request is followed by size bytes of data */
};  

/* various response packets, sent to the client after the 
   server processes a request packet */
 
#define PG_MAX_RESPONSE_SZ 12  /* in bytes */

#define PG_RESPONSE_ERR 1
struct pgresponse_err {
  unsigned short type;    /* RESPONSE_ERR - error code */
  unsigned short id;
  unsigned short errt;
  unsigned short msglen;  /* Length of following message */
};

#define PG_RESPONSE_RET 2
struct pgresponse_ret {
  unsigned short type;    /* RESPONSE_RET - return value */
  unsigned short id;
  unsigned long data;
};

#define PG_RESPONSE_EVENT 3
struct pgresponse_event {
  unsigned short type;    /* RESPONSE_EVENT */
  unsigned short event;
  unsigned long from;
  unsigned long param;
};

#define PG_RESPONSE_DATA 4
struct pgresponse_data {
  unsigned short type;    /* RESPONSE_DATA */
  unsigned short id;
  unsigned long size;
  /* 'size' bytes of data follow */
};

/* This is sent to the client after establishing a connection */
struct pghello {
  unsigned long  magic;
  unsigned short protover;
  unsigned short dummy;   /* padding */
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
#define PGREQ_GRABKBD      19  /* Become the keyboard owner      |  none */
#define PGREQ_GRABPNTR     20  /* Own the pointing device        |  none */
#define PGREQ_GIVEKBD      21  /* Give the keyboard back         |  none */
#define PGREQ_GIVEPNTR     22  /* Give the pointing device back  |  none */
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

#define PGREQ_UNDEF        33     /* types > this will be truncated. return error */

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
  unsigned long h;   /* for requests that just use a handle */
};

struct pgreqd_mkwidget {
  unsigned short rship;
  unsigned short type;
  unsigned long parent;
};
struct pgreqd_mkbitmap {
  unsigned short w;       /* If these are 0, the following data is a */
  unsigned short h;       /* pnm bitmap.  Otherwise, these are the dimensions
			     of xbm data following it. */
  unsigned long fg;       /* Foreground and background colors if this is a */
  unsigned long bg;       /* xbm bitmap. */
};
struct pgreqd_mkfont {
  char name[40];
  unsigned long style;
  unsigned short size;
  unsigned short dummy;
};
struct pgreqd_set {
  unsigned long widget;
  unsigned long glob;
  unsigned short property;
  unsigned short dummy;
};
struct pgreqd_get {
  unsigned long widget;
  unsigned short property;
  unsigned short dummy;
};
struct pgreqd_in_key {
  unsigned long type;   /* A TRIGGER_* constant */
  unsigned short key;
  unsigned short mods;
};
struct pgreqd_in_point {
  unsigned long type;   /* A TRIGGER_* constant */
  unsigned short x;
  unsigned short y;
  unsigned short btn;  /* button bitmask */
  unsigned short dummy;
};
struct pgreqd_in_direct {
  unsigned long param;   /* The arbitrary parameter */
  /* The rest of the packet is read as a string */
};
struct pgreqd_themeset {
  unsigned long value;
  unsigned short element;
  unsigned short state;
  unsigned short param;
  unsigned short dummy;
};
struct pgreqd_register {
  /* This is just a subset of app_info, organized for network
     transmission */

  unsigned long name;  /* string handle */
  unsigned short type;
  unsigned short dummy;

  /* Followed by optional APPSPECs */
};
struct pgreqd_mkpopup {
  unsigned short x; /* can be a PG_POPUP_* constant */
  unsigned short y; 
  unsigned short w;
  unsigned short h;
};
struct pgreqd_sizetext {
  unsigned long text;  /* Handle to text and to font */
  unsigned long font;
};
struct pgreqd_setpayload {
  unsigned long h;  /* Any handle */
  unsigned long payload;  /* 32-bits of data to store with
			     the handle'd object */
};
struct pgreqd_mkmsgdlg {
  unsigned long title;
  unsigned long text;
  unsigned long flags;
};

#endif /* __H_PG_NETWORK */
/* The End */
