/* $Id$
 *
 * input.h - Abstract input driver interface
 *
 *    There are two layers to this interface- the input drivers
 *    provide callbacks that actually gather input from devices.
 *    Input filters then form a chain that processes the events
 *    and dispatches them to interested parties.
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
#ifdef __MINGW32__
#include <winsock2.h>
#endif
#define EAGAIN WSAEWOULDBLOCK
#define ioctl ioctlsocket
#else
#include <sys/time.h>    /* For timeval */
#include <sys/types.h>   /* For fd_set */
#endif

#include <pgserver/g_error.h>
#include <pgserver/handle.h>
#include <pgserver/video.h>
#include <pgserver/divtree.h>

/********************************************** Cursors */

/* There may be any number of cursors on the screen at once:
 *  - one for each input driver that needs one
 *  - one for each divtree, used by the hotspot system
 */

struct cursor {
  /**** Public data: */

  int prev_buttons;              /* Used during preprocessing to detect button press/release */

  /**** Public read-only data: */

  struct {
    handle widget_last_clicked;  /* The widget most recently clicked by the cursor */
    handle widget_under;         /* The widget under the cursor */
    handle widget_capture;       /* When the mouse is clicked in a widget, that widget
				  * captures MOVE and RELEASE events */

    int capture_btn;             /* The button that the widget_capture was obtained with */

    /* WARNING: These pointers may be invalid if the widget or divnodes have been deleted
     * since they were clicked or moved over. These pointers should only be dereferenced
     * by the widget pointed to by widget_under, in all other cases use the widget handles.
     */
    struct divnode *div_under;   /* The interactive divnode immediately under the cursor */
    struct divnode *deepest_div; /* The deepest (in the divtree) node under the cursor
				  * regardless of its interactive status or ownership. */
  } ctx;                         /* Contextual info on the pointer */

  int thobj;                     /* Theme object, as set with cursor_set_theme() */

  /**** Private data: */

  struct sprite *sprite;
  int hotspot_x, hotspot_y;      /* Location of hotspot on the sprite */
  int x,y;
  handle divtree;

  struct cursor *next;
};

/* Create the cursor itself, assigning it the default theme.
 * Either crsr or h may be null.
 */
g_error cursor_new(struct cursor **crsr, handle *h, int owner);

/* Drivers should use pointer_free instead of cursor_delete, so they
 * delete it via the handle system. This is only needed by object_free
 */
void cursor_delete(struct cursor *crsr);

void cursor_shutdown(void);

/* This (re)assigns the cursor a sprite, without
 * affecting its visibility. If this is the first time
 * it is called on a new sprite, the sprite will remain
 * invisible until cursor_show() is called.
 */
g_error cursor_set_theme(struct cursor *crsr, int thobj);

/* Accessor functions for getting/setting the position.
 * If the caller doesn't have a specific cursor to get the position of,
 * passing NULL to cursor_getposition will return the position of the
 * default cursor.
 */
void cursor_move(struct cursor *crsr, int x, int y, struct divtree *dt);
void cursor_getposition(struct cursor *crsr, int *x, int *y, struct divtree **dt);

/* Hide and show the cursor sprite, while also managing enter/leave events
 */
void cursor_hide(struct cursor *crsr);
void cursor_show(struct cursor *crsr);

/* Return the "Default" cursor, which is currently defined
 * as the one most recently interacted with or created.
 * If there is no cursor, this returns NULL.
 */
struct cursor *cursor_get_default(void);

/* Re-evaluate the widget under every cursor and send out events
 */
void cursor_update_hover(void);

/* Reload all cursor theme objects if necessary
 */
g_error cursor_retheme(void);

/********************************************** Triggers */

/* NOTE: Trigger types are now defined in constants.h */

/* Trigger param union */
union trigparam {

  struct trigparam_mouse {
    int x,y,btn;  /* Current mouse status */
    int chbtn;    /* Changed buttons */
    int pressure;

    /* This is used to distinguish between multiple mice attached.
     * It can be NULL to indicate that the event didn't come from a device
     * with an associated cursor. A NULL mouse cursor implies that no enter/leave
     * events should be generated.
     * Mouse events can not be dispatched without an attached cursor!
     * The pntr_normalize filter should give cursorless events an invisible cursor
     * to handle collecting the contextual information about the mouse location.
     */
    struct cursor *cursor;

    /* Usually mouse events will come in with physical coordinates and be
     * translated to logical coordinates during the normalization infilter.
     * However, sometimes events will come in to the filter chain in 
     * logical coordinates.. This makes tracking the whole mess easier.
     * This is nonzero if the input is in logical coordinates.
     */
    int is_logical;

    /* This is a handle to the touchscreen calibration string to use.
     * If it is zero or invalid, this uses the one set with PG_TRIGGER_TS_SETCAL
     * or read in from the configuration file.
     */
    handle ts_calibration;

    /* The handle to a divtree to dispatch this to, or 0 to send it to
     * the top divtree.
     */
    handle divtree;

  } mouse;

  struct trigparam_kbd {
    int key;      /* PGKEY_* constant */
    int mods;     /* PGMOD_* constant */
    int flags;    /* PG_KF_* constants */
    int consume;  /* Increment this to consume the key event */

    /* The handle to a divtree to dispatch this to, or 0 to send it to
     * the top divtree.
     */
    handle divtree;

  } kbd;

  /* Data from a motion tracking device-
   * Values unsupported by the device should be zero.
   * Coordinates are in device-dependent physical units, but should
   * be normalized between 0 and 0x7FFFFFF. If possible, the client
   * library should interpret this as a floating point number
   * between 0.0 and 1.0
   */
  struct trigparam_motion {
    u32 position[3];
    u32 orientation[3];
    u32 bodyPart;          /* A PG_BODYPART_* constant indicating what this motion data is for */
  } motion;

  struct trigparam_stream {
    u32 size;
    u8 *data;
  } stream;

  struct trigparam_command {
    u32 command;
    u32 numparams;
    s32 *data;
  } command;
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

/* Load all input drivers specified in the config database */
g_error input_init(void);

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
g_error vncinput_regfunc(struct inlib *i);
g_error palmaxts_regfunc(struct inlib *i);
g_error gpm_regfunc(struct inlib *i);
g_error mgl2input_regfunc(struct inlib *i);
g_error adc7843_regfunc(struct inlib *i);
g_error zaurus_regfunc(struct inlib *i);
g_error ericsson_cb_regfunc(struct inlib *i);
g_error ps2mouse_regfunc(struct inlib *i);
g_error jsdev_regfunc(struct inlib *i);
g_error rawttykb_regfunc(struct inlib *i);
g_error btkey_regfunc(struct inlib *i);
g_error directfbinput_regfunc(struct inlib *i);

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

struct infilter;

typedef void (*infilter_handler)(struct infilter *self, u32 trigger, union trigparam *param);

struct infilter {
  u32 accept_trigs;    /* Mask of trigger types to process here */
  u32 absorb_trigs;    /* Mask of trigger types to _not_ automatically pass on */
  infilter_handler handler;

  int owner;           /* Client to send events to, used by the if_client_adaptor filter */

  struct infilter *next;
};

/* Head of the input filter list */
extern struct infilter *infilter_list;

/* Pass an event to the next input filter, given a source filter.
 * Events coming directly from drivers should give NULL as the source.
 */
void infilter_send(struct infilter *from, u32 trigger, union trigparam *param);

/* Shortcuts for sending particular trigger types via infilter_send,
 * to make driver-writing easier.
 */
void infilter_send_key(u32 trigger, int key, int mods);
void infilter_send_pointing(u32 trigger, int x, int y,
			    int btn, struct cursor *cursor);
void infilter_send_touchscreen(int x, int y, int pressure, int btn);

/* Management functions */
g_error infilter_insert(struct infilter **insertion, handle *h, int owner,
			struct infilter *def);
void infilter_delete(struct infilter *node);
g_error infilter_init(void);
g_error touchscreen_init(void);

/********************************************** Input filter client-side adaptor */

union pg_client_trigger;

/* Create an input filter to act as an adaptor between pgserver and
 * client-side input filters.
 */
g_error infilter_client_create(handle insertion, u32 accept_trigs,
			       u32 absorb_trigs, handle *h, int owner);

/* Given a pg_client_trigger *in network byte order* this will reconstruct
 * the trigparam union from it, and dispatch it to the proper input filter.
 */
g_error infilter_client_send(union pg_client_trigger *client_trig);

/********************************************** Misc input utilities */

/* sends a trigger to a widget,
 * returns nonzero if the trigger was accepted.
 */
int send_trigger(struct widget *w, s32 type, union trigparam *param);

/* Sends a trigger to all of a widget's children,
 * stopping if *stop > 0. Always traverses to the child and the panelbar,
 * only traverses forward for the top level if 'forward' is nonzero.
 */
void r_send_trigger(struct widget *w, s32 type, union trigparam *param,
		    int *stop, int forward);

/* Send a trigger that propagates to a widget's container until it's accepted.
 * Returns nonzero if the event is accepted
 */
int send_propagating_trigger(struct widget *w, s32 type, union trigparam *param);

/*
 * Request focus for a widget.  Usually called in response to a click, or 
 * a hotkey.  Sends PG_TRIGGER_ACTIVATE and PG_TRIGGER_DEACTIVATE, and sets kbdfocus
 */
void request_focus(struct widget *self);

#endif /* __H_INPUT */
/* The End */










