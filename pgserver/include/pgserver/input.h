/* $Id: input.h,v 1.40 2002/05/22 00:09:58 micahjd Exp $
 *
 * input.h - Abstract input driver interface
 *
 *    There are two layers to this interface- the input drivers
 *    provide callbacks that actually gather input from devices.
 *    Input filters then form a chain that processes the events
 *    and dispatches them to interested parties.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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

#ifndef __H_INPUT
#define __H_INPUT

#if defined(__WIN32__) || defined(WIN32)
#ifndef WINDOWS
#define WINDOWS
#endif
#include <winsock2.h>
#define EAGAIN WSAEWOULDBLOCK
#define ioctl ioctlsocket
#else
#include <sys/time.h>    /* For timeval */
#include <sys/types.h>   /* For fd_set */
#endif

#include <pgserver/g_error.h>

/********************************************** Trigger constants */

/* Constants for a trigger type. One of these constants is used to identify
   a trigger when it happens, and they are combined to form a trigger mask */
#define TRIGGER_TIMER         (1<<0)  /* Timer event from install_timer */
#define TRIGGER_UNUSED_1      (1<<1)
#define TRIGGER_DIRECT        (1<<2)  /* A trigger sent explicitely */
#define TRIGGER_ACTIVATE      (1<<3)  /* Sent when it receives focus */
#define TRIGGER_DEACTIVATE    (1<<4)  /* Losing focus */
#define TRIGGER_KEYUP         (1<<5)  /* Ignores autorepeat, etc. Raw key codes*/
#define TRIGGER_KEYDOWN       (1<<6)  /* Ditto. */
#define TRIGGER_RELEASE       (1<<7)  /* Mouse up (see note) */
#define TRIGGER_UP            (1<<8)  /* Mouse up in specified divnode */
#define TRIGGER_DOWN          (1<<9)  /* Mouse down in divnode */
#define TRIGGER_MOVE          (1<<10) /* Triggers on any mouse movement in node */
#define TRIGGER_ENTER         (1<<11) /* Mouse moves inside widget */
#define TRIGGER_LEAVE         (1<<12) /* Mouse moves outside widget */
#define TRIGGER_DRAG          (1<<13) /* Mouse move when captured */
#define TRIGGER_CHAR          (1<<14) /* A processed ASCII/Unicode character */
#define TRIGGER_STREAM        (1<<15) /* Incoming packet (from WRITETO) */
#define TRIGGER_KEY_START     (1<<16) /* Sent at the beginning of key propagation */
#define TRIGGER_NONTOOLBAR    (1<<17) /* Not really a trigger, but widgets can put this
				       * in their trigger mask to request placement in
				       * the nontoolbar area when applicable */
#define TRIGGER_PNTR_STATUS   (1<<18) /* A driver can send this trigger with the current
				       * status of the mouse to have the input filters
				       * automatically extrapolate other events. */
#define TRIGGER_KEY           (1<<19) /* A driver can send this with a key code when
				       * the exact state of the key is unknown, to have
				       * KEYUP, KEYDOWN, and CHAR events generated. */

/* Note on TRIGGER_RELEASE:  This is when the mouse was pressed inside
   the widget, then released elsewhere.  */

/* Trigger param union */
union trigparam {

  struct {
    int x,y,btn;  /* Current mouse status */
    int chbtn;    /* Changed buttons */
    int pressure;

    /* Relative to a particular divnode */
    struct divnode *div;
    int divx,divy;
  } mouse;

  struct {
    int key;      /* PGKEY_* constant */
    int mods;     /* PGMOD_* constant */
    int flags;    /* PG_KF_* constants */
    int consume;  /* Increment this to consume the key event */
  } kbd;

  struct {
    u32 size;
    u8 *data;
  } stream;
};


/********************************************** Input driver interface */

/* Just like the video lib, use a structure of pointers
   to functions. These aren't called by much though, and
   defaults don't make as much sense as for video drivers.
   So, unused functions are set to null and simply not
   called. */

struct inlib {
  
  /* Called upon loading the input driver. This inits
     the hardware or underlying library, and optionally
     starts an input thread.
  */
  g_error (*init)(void);

  /* Upon unloading... */
  void (*close)(void);

  /* This method gives the driver control when activity
     on filedescriptors is detected.  fd_init() is called
     prior to entering the select loop, to set the
     necessary bits. After the select loop, if the 
     network code doesn't need the fd it is sent to
     fd_activate(). If the fd is owned by the driver,
     process it and return a 1. Else, return a 0 and
     the fd will go to the next driver. (More efficient
     on average, considering that normally only 1 input
     driver will be loaded)
  */
  /* Look familiar? See select(2). These are already
     initialized, but the input driver gets a chance
     to modify them.
  */
  void (*fd_init)(int *n,fd_set *readfds,struct timeval *timeout);
  int (*fd_activate)(int fd);

  /* This is called after every iteration through the select
     loop no matter what. Note that the default select
     timeout is a few seconds, so if you rely on this for input
     you should define fd_init to lower timeout to something ok
     for the driver
  */
  void (*poll)(void);

  /* If the input device queues events, this should return nonzero
   * when the queue is nonempty. */
  int (*ispending)(void);

  /* For recieving driver messages */
  void (*message)(u32 message, u32 param, u32 *ret);
   
  /* Do not touch (drivers) */
  g_error (*regfunc)(struct inlib *i);  /* For avoiding duplicates */
  struct inlib *next;
};

/* Head of the inlib list. */
extern struct inlib *inlib_list;

/* The main driver - that which the vidlib loads
   and unloads automatically. This can be NULL if the
   vidlib doesn't need an input driver. 
*/
extern struct inlib *inlib_main;

/* Loads an input driver, and puts a pointer to it in
   ppinlib.
*/
g_error load_inlib(g_error (*regfunc)(struct inlib *i),
		   struct inlib **inl);

/* Unload a specific driver */
void unload_inlib(struct inlib *inl);

/* Check whether any drivers have a backlog of events.
 * This calls the driver's ispending function */
int events_pending(void);

/* Unload all drivers */
void cleanup_inlib(void);

/* Registration functions */
g_error sdlinput_regfunc(struct inlib *i);
g_error svgainput_regfunc(struct inlib *i);
g_error ncursesinput_regfunc(struct inlib *i);
g_error chipslicets_regfunc(struct inlib *i);
g_error eventbroker_regfunc(struct inlib *i);
g_error r3912ts_regfunc(struct inlib *i);
g_error rrdsc21_gpio_regfunc(struct inlib *i);
g_error rrsim_regfunc(struct inlib *i);
g_error rrts_regfunc(struct inlib *i);
g_error vr3ts_regfunc(struct inlib *i);
g_error h3600ts_regfunc(struct inlib *i);
g_error tuxts_regfunc(struct inlib *i);
g_error ucb1x00_regfunc(struct inlib *i);
g_error sc3_regfunc(struct inlib *i);
g_error ttykb_regfunc(struct inlib *i);
g_error remorakb_regfunc(struct inlib *i);
g_error serialmouse_regfunc(struct inlib *i);
g_error x11input_regfunc(struct inlib *i);
g_error palmaxts_regfunc(struct inlib *i);
g_error gpm_regfunc(struct inlib *i);
g_error mgl2input_regfunc(struct inlib *i);
g_error adc7843_regfunc(struct inlib *i);
g_error zaurus_regfunc(struct inlib *i);
g_error ericsson_cb_regfunc(struct inlib *i);
g_error ps2mouse_regfunc(struct inlib *i);
g_error jsdev_regfunc(struct inlib *i);
g_error rawttykb_regfunc(struct inlib *i);

/* List of installed input drivers */
struct inputinfo {
  char *name;
  g_error (*regfunc)(struct inlib *i);
};
extern struct inputinfo inputdrivers[];

g_error (*find_inputdriver(const char *name))(struct inlib *i);

/* If this is 1, no user input is taken. */
extern int disable_input;

/********************************************** Input filter interface */

struct infilter {
  u32 accept_trigs;    /* Mask of trigger types to process here */
  u32 absorb_trigs;    /* Mask of trigger types to _not_ automatically pass on */

  void (*handler)(u32 trigger, union trigparam *param);

  struct infilter *next;
};

/* Head of the input filter list */
struct infilter *infilter_list;

/* The built-in input filters.
 */
struct infilter *infilter_touchscreen;                               /* Transformation and filtering for touchscreens */
struct infilter *infilter_key_preprocess, *infilter_pntr_preprocess; /* Transformation and munging                    */
struct infilter *infilter_key_magic;                                 /* Handling magic CTRL-ALT-* keys                */
struct infilter *infilter_key_dispatch, *infilter_pntr_dispatch;     /* Send the input where it needs to go           */

/* Pass an event to the next input filter, given a source filter.
 * Events coming directly from drivers should give NULL as the source.
 */
void infilter_send(struct infilter *from, u32 trigger, union trigparam *param);

/* Management functions */
struct infilter *infilter_create(void);
void infilter_insert(struct infilter *node, struct infilter **where);
void infilter_delete(struct infilter *node);

#endif /* __H_INPUT */
/* The End */










