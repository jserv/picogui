/* $Id: widget.h,v 1.59 2002/03/26 17:07:06 instinc Exp $
 *
 * widget.h - defines the standard widget interface used by widgets
 * This is an abstract widget framework that loosely follows the
 * Property Method Event model.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 * pgCreateWidget & pgAttachWidget functionality added by RidgeRun Inc.
 * Copyright (C) 2001 RidgeRun, Inc.  All rights reserved.   
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

#ifndef __WIDGET_H
#define __WIDGET_H

#include <picogui/constants.h>

#include <pgserver/divtree.h>
#include <pgserver/font.h>
#include <pgserver/video.h>
#include <pgserver/g_error.h>
#include <pgserver/g_malloc.h>
#include <pgserver/pgmain.h>
#include <pgserver/svrtheme.h>
#include <pgserver/pgnet.h>
#include <pgserver/render.h>

struct blob;
struct widgetdef;
struct widget;

/******* Structures */

/* A data type to represent anything */
typedef long glob;

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

/* Note on TRIGGER_RELEASE:  This is when the mouse was pressed inside
   the widget, then released elsewhere.  */

/* Trigger param union */
union trigparam {
  struct {
    s16 x,y,btn;  /* Current mouse status */
    s16 chbtn;    /* Changed buttons */

    /* Relative to a particular divnode */
    struct divnode *div;
    s16 divx,divy;
  } mouse;
  struct {
    s16 key;      /* PGKEY_* constant */
    s16 mods;     /* PGMOD_* constant */
    u16 flags;    /* PG_KF_* constants */
    u16 consume;  /* Increment this to consume the key event */
  } kbd;
  struct {
    u32 size;
    u8 *data;
  } stream;
};

/* This contains function pointers for the widget methods that define
   how a widget acts and what it does.
*/
struct widgetdef {
  /* Things every widget should have */
  g_error (*install)(struct widget *self);
  void (*remove)(struct widget *self);

  /* Optional, only for interactive controls */
  void (*trigger)(struct widget *self,s32 type,union trigparam *param);

  /* Setting/getting properties */
  g_error (*set)(struct widget *self, int property, glob data);
  glob (*get)(struct widget *self, int property);

  /* This function should recalculate the preferred size for all
   * applicable divnodes. The widget can call it when a parameter has
   * changed that would change the widget's size, the system can call it
   * when something globally changes, like the theme. */
  void (*resize)(struct widget *self);
};

/* The table of widgetdef's, indexed by type */
extern struct widgetdef widgettab[];

/* Finally, and actual widget structure */
struct widget {

  /***** Bits */
   
  /* Omit the usual automatic clear and update done in div_rebuild */
  unsigned int rawbuild : 1;
   
  /* If this is a root widget, an unprivelidged app can only derive
     widgets inside it, not before or after it */
  unsigned int isroot : 1;

  /* If this flag is set, other applications can derive widgets inside
   * this container. (They cannot modify it in any other way
   */
  unsigned int publicbox : 1;

  /* When this widget's orientation (horizontal/vertical), set the
   * orientation of all children to the opposite. Useful for toolbars
   */
  unsigned int auto_orientation : 1;

  /***** 16/8-bit packed values */
   
  /* Defines the type of widget */
  u8 type;

  /* If not null, the widget is contained within this widget (a toolbar,
   * panel, etc...
   */
  handle container;

  /* The currently active mutually exclusive widget. Only applies if this
   * is a container.
   */
  handle activemutex;

  /***** 32-bit values */

  /* Connection that created the widget.  Any handles the widget make
   * take on this owner
   */
  int owner;
   
  struct widgetdef *def;  /* (Methods) */
   
  /* These pointers indicate the input, output, and sub of the
   * local divtree */
  struct divnode *in;
  struct divnode **out;
  struct divnode **sub;
  /* This is where it was added (so it can be deleted by setting this
     back to the output of this widget */
  struct divnode **where;
   
  /* The divtree this widget is part of */
  struct divtree *dt;

  /* Widget's private data (Properties) */
  void *data;

  /* The widget's optional name (string handle) */
  handle name;

  /* This is the scroll bar bound to us, set with the PG_WP_BIND property. */
  handle scrollbind;

  /* widget sets this to accept triggers.  TRIGGER_* constants or'ed
     together. */
  u32 trigger_mask;

  /* Time (in ticks) for a TRIGGER_TIMER */
  u32 time;
  /* Widgets with timers are in a linked list */
  struct widget *tnext;

  /* Other widgets in pgserver may set up a callback to recieve events
   * from the widget. If the callback returns false, the widget is passed
   * on to the client normally, if it returns true the event is absorbed.
   */
  int (*callback)(int event, struct widget *from, s32 param, int owner, char *data);
};

/* Macros to help define widgets */
#ifdef RUNTIME_FUNCPTR   /* If we can't use function pointers at compile-time */

# define DEF_WIDGET_TABLE(n) \
  p->install = &n##_install; p->remove = &n##_remove; p->trigger = &n##_trigger; p->set = &n##_set; p->get = &n##_get; p->resize = &n##_resize; p++;
# define DEF_HYBRIDWIDGET_TABLE(n,m) \
  p->install = &n##_install; p->remove = &m##_remove; p->trigger = &m##_trigger; p->set = &m##_set; p->get = &m##_get; p->resize = &m##_resize; p++;
# define DEF_STATICWIDGET_TABLE(n) \
  p->install = &n##_install; p->remove = &n##_remove; p->trigger = NULL; p->set = &n##_set; p->get = &n##_get; p->resize = &n##_resize; p++;
# define DEF_ERRORWIDGET_TABLE(s) \
  p->install = NULL; p->remove = (void *) s; p->trigger = NULL; p->set = NULL; p->get = NULL; p->resize = NULL; p++;

#else  /* ! RUNTIME_FUNCPTR */

# define DEF_WIDGET_TABLE(n) \
  {n##_install, n##_remove, n##_trigger, n##_set, n##_get, n##_resize},
# define DEF_HYBRIDWIDGET_TABLE(n,m) \
  {n##_install, m##_remove, m##_trigger, m##_set, m##_get, m##_resize},
# define DEF_STATICWIDGET_TABLE(n) \
  {n##_install, n##_remove, NULL, n##_set, n##_get, n##_resize},
# define DEF_ERRORWIDGET_TABLE(s) \
  {NULL, (void *) s, NULL, NULL, NULL, NULL},

#endif  /* RUNTIME_FUNCPTR */
#define DEF_WIDGET_PROTO(n) \
  g_error n##_install(struct widget *self); \
  void n##_remove(struct widget *self); \
  void n##_trigger(struct widget *self,s32 type,union trigparam *param); \
  g_error n##_set(struct widget *self, int property, glob data); \
  glob n##_get(struct widget *self, int property); \
  void n##_resize(struct widget *self);

/* Widget prototypes */
DEF_WIDGET_PROTO(toolbar)      /* A container for buttons */
DEF_WIDGET_PROTO(scroll)
DEF_WIDGET_PROTO(label)        /* Text, resizes to fit */
DEF_WIDGET_PROTO(indicator)
DEF_WIDGET_PROTO(bitmap)
DEF_WIDGET_PROTO(button)
DEF_WIDGET_PROTO(panel)
DEF_WIDGET_PROTO(popup)
DEF_WIDGET_PROTO(box)
DEF_WIDGET_PROTO(field)
DEF_WIDGET_PROTO(background)
DEF_WIDGET_PROTO(menuitem)
DEF_WIDGET_PROTO(terminal)
DEF_WIDGET_PROTO(canvas)
DEF_WIDGET_PROTO(checkbox)
DEF_WIDGET_PROTO(flatbutton)
DEF_WIDGET_PROTO(listitem)
DEF_WIDGET_PROTO(submenuitem)
DEF_WIDGET_PROTO(radiobutton)
DEF_WIDGET_PROTO(textbox)
DEF_WIDGET_PROTO(list)
DEF_WIDGET_PROTO(panelbar)
DEF_WIDGET_PROTO(simplemenu)
    
/* Set to the client # if a client has taken over the system resource */
extern int keyboard_owner;
extern int pointer_owner;
extern int sysevent_owner;

/******* These functions define the 'public' methods for widgets */
     
/* Special function to generate a popup root widget */
g_error create_popup(int x,int y,int w,int h,struct widget **wgt,int owner);

g_error widget_derive(struct widget **w, int type,struct widget *parent, handle hparent,int rship,int owner);
g_error widget_attach(struct widget *w, struct divtree *dt,struct divnode **where, handle container, int owner);
g_error widget_create(struct widget **w, int type, struct divtree *dt, handle container, int owner);

void widget_remove(struct widget *w);
g_error widget_set(struct widget *w, int property, glob data);
glob widget_get(struct widget *w, int property);

/* This is used in transparent widgets - it propagates a redraw through
   the container the widget is in, in order to redraw the background
*/
void redraw_bg(struct widget *self);

/*
   Set a timer.  At the time, in ticks, specified by 'time',
   the widget will recieve a TRIGGER_TIMER
*/
void install_timer(struct widget *self,u32 interval);

/* This is called by the timer subsystem.  It triggers the
   timer and uninstalls it from the linked list of timers.

   It is assumed that this is triggering the timer at the
   beginning of the timerwidgets list
*/
void trigger_timer(void);

/*
  Request focus for a widget.  Usually called in response to a click, or 
  a hotkey.  Sends TRIGGER_ACTIVATE and TRIGGER_DEACTIVATE, and sets kbdfocus
*/
void request_focus(struct widget *self);

/**** These are entry points for the various input drivers. */

/* Reset pointers for the pointing device when a layer is popped */
void reset_widget_pointers(void);

/* This is a pointing device event.
   The trigger parameter is a struct pointing_trigger.
   x and y here are absolute coords. The divtree is traversed to find the
   node at those coords.

   btn specifies the buttons down on the pointing device. Bit 0 is the
   first mouse button, bit 1 is the second, etc.
   Pointing device events should only be sent here for MOVE, UP, DOWN.
*/
void dispatch_pointing(u32 type,s16 x,s16 y,s16 btn);

/* Update which widget/divnode the mouse is inside, etc, when the divtree changes */
void update_pointing(void);

/* This dispatches a key.  It is first checked against the global
   key owner table, and if not found there it is sent to the currently 
   focused widget.  The key is passed as the trigger param.
*/
void dispatch_key(u32 type,s16 key,s16 mods);
  
/* The divnode currently occupied by the pointing device - this will always
 * be a visible divnode owned by an interactive widget */
extern struct divnode *div_under_crsr;
/* This will be the deepest (in the divtree) divnode containing the
 * pointer, even if it is not visible or owned by an interactive widget */
extern struct divnode *deepest_div_under_crsr;

/* Other status variables */
/* These are needed to determine which widget is under the pointing
   device, keep track of status */
extern struct widget *under;
extern struct widget *lastclicked;    /* Most recently clicked widget */
extern struct widget *capture;
extern struct widget *kbdfocus;

/* Clips a popup's main divnode to fit on the screen,
 * used when changing video modes */
void clip_popup(struct divnode *div);

/* Resizes an individual widget and calculates a new split value if necessary.
 * This is called by widgets whenever key parameters have changed, such as 
 * widget text or font size. Recursively calculates the child divnode sizes, 
 * and sets split parameters.
 */
void resizewidget(struct widget *w);

/* Call the resizewidget() function on all widgets with handles */
void resizeall(void);

/* Check for global hotkeys, like those used to move between widgets.
 * Widgets that can be focused (this includes widgets with hotspots)
 * should send unused keys here.
 */
void global_hotkey(u16 key,u16 mods, u32 type);

/* Reloads global hotkey settings from the theme when it changes */
void reload_hotkeys(void);
extern u16 hotkey_left, hotkey_right, hotkey_up, hotkey_down;
extern u16 hotkey_activate, hotkey_next;

/* Traverse to other widgets in a given direction (PG_TRAVERSE_*) */
struct widget *widget_traverse(struct widget *w, int direction, int count);

/* sends a trigger to a widget,
 * returns nonzero if the trigger was accepted.
 */
int send_trigger(struct widget *w, s32 type, union trigparam *param);

/* Sends a trigger to all of a widget's children,
 * stopping if *stop > 0. Always traverses to the child and the panelbar,
 * only traverses forward for the top level if 'forward' is nonzero.
 */
void r_send_trigger(struct widget *w, s32 type, union trigparam *param,
		    u16 *stop, int forward);

/* Invokes the spirits of guru() and stdout for debuggativity */
void magic_button(s16 key);

/* Set flags to rebuild a widget on the next update,
 * Assumes that w->in->div is the visible divnode.
 */
void set_widget_rebuild(struct widget *w);

#endif /* __WIDGET_H */

/* The End */

