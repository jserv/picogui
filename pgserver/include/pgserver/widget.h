/* $Id: widget.h,v 1.16 2001/02/07 08:45:07 micahjd Exp $
 *
 * widget.h - defines the standard widget interface used by widgets
 * This is an abstract widget framework that loosely follows the
 * Property Method Event model.
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

struct blob;
struct widgetdef;
struct widget;

/******* Structures */

/* A data type to represent anything */
typedef long glob;

/* Constants for a trigger type. One of these constants is used to identify
   a trigger when it happens, and they are combined to form a trigger mask */
#define TRIGGER_TIMER      (1<<0)  /* Timer event from install_timer */
#define TRIGGER_HOTKEY     (1<<1)  /* The registered 'hotkey' was pressed */
#define TRIGGER_DIRECT     (1<<2)  /* A trigger sent explicitely */
#define TRIGGER_ACTIVATE   (1<<3)  /* Sent when it receives focus */
#define TRIGGER_DEACTIVATE (1<<4)  /* Losing focus */
#define TRIGGER_KEYUP      (1<<5)  /* Ignores autorepeat, etc. Raw key codes*/
#define TRIGGER_KEYDOWN    (1<<6)  /* Ditto. */
#define TRIGGER_RELEASE    (1<<7)  /* Mouse up (see note) */
#define TRIGGER_UP         (1<<8)  /* Mouse up in specified divnode */
#define TRIGGER_DOWN       (1<<9)  /* Mouse down in divnode */
#define TRIGGER_MOVE       (1<<10) /* Triggers on any mouse movement in node */
#define TRIGGER_ENTER      (1<<11) /* Mouse moves inside widget */
#define TRIGGER_LEAVE      (1<<12) /* Mouse moves outside widget */
#define TRIGGER_DRAG       (1<<13) /* Mouse move when captured */
#define TRIGGER_CHAR       (1<<14) /* A processed ASCII/Unicode character */
#define TRIGGER_STREAM     (1<<15) /* Incoming packet (from WRITETO) */

/* Note on TRIGGER_RELEASE:  This is when the mouse was pressed inside
   the widget, then released elsewhere.  */

/* Trigger param union */
union trigparam {
  struct {
    int x,y,btn;  /* Current mouse status */
    int chbtn;    /* Changed buttons */

    /* Relative to a particular divnode */
    struct divnode *div;
    int divx,divy;
  } mouse;
  struct {
    int key;
    int mods;
  } kbd;
  struct {
    unsigned long size;
    unsigned char *data;
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
  void (*trigger)(struct widget *self,long type,union trigparam *param);

  /* Setting/getting properties */
  g_error (*set)(struct widget *self, int property, glob data);
  glob (*get)(struct widget *self, int property);
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

  /***** 16/8-bit packed values */
   
  /* Defines the type of widget */
  int type;

  /* Connection that created the widget.  Any handles the widget make
   * take on this owner
   */
  int owner;


  /* If not null, the widget is contained within this widget (a toolbar,
   * panel, etc...
   */
  handle container;


  /***** 32-bit values */
   
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

  /* This is called whenever sizing parameters have changed, such
     as when a theme is loaded */
  void (*resize)(struct widget *self);
  
  /* Widget's private data (Properties) */
  void *data;

  /* Used for management of triggers: */
 
  /*
   For any trigger to be accepted, it must be or'ed into 'trigger_mask'

   A direct trigger is caused by a match with direct_trigger, and a hotkey
   is triggered by a match with 'hotkey'   
  */
 
  /* widget sets this to accept triggers.  TRIGGER_* constants or'ed
     together. */
  long trigger_mask;

  /* Name of an active direct trigger */
  char *direct_trigger;

  /* Active hotkey */
  long hotkey;
  /* The widgets with assigned hotkeys are stored in a linked list */
  struct widget *hknext;

  /* Time (in ticks) for a TRIGGER_TIMER */
  unsigned long time;
  /* Widgets with timers are in a linked list */
  struct widget *tnext;
};

/* Macros to help define widgets */
#ifdef RUNTIME_FUNCPTR   /* If we can't use function pointers at compile-time */

# define DEF_WIDGET_TABLE(n) \
  p->install = &n##_install; p->remove = &n##_remove; p->trigger = &n##_trigger; p->set = &n##_set; p->get = &n##_get; p++;
# define DEF_STATICWIDGET_TABLE(n) \
  p->install = &n##_install; p->remove = &n##_remove; p->trigger = NULL; p->set = &n##_set; p->get = &n##_get; p++;

#else  /* ! RUNTIME_FUNCPTR */

# define DEF_WIDGET_TABLE(n) \
  n##_install, n##_remove, n##_trigger, n##_set, n##_get,
#define DEF_STATICWIDGET_TABLE(n) \
  n##_install, n##_remove, NULL, n##_set, n##_get,

#endif  /* RUNTIME_FUNCPTR */
#define DEF_WIDGET_PROTO(n) \
  g_error n##_install(struct widget *self); \
  void n##_remove(struct widget *self); \
  void n##_trigger(struct widget *self,long type,union trigparam *param); \
  g_error n##_set(struct widget *self, int property, glob data); \
  glob n##_get(struct widget *self, int property);

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
			  
/* Set to the client # if a client has taken over the system resource */
extern int keyboard_owner;
extern int pointer_owner;
extern int sysevent_owner;

/******* These functions define the 'public' methods for widgets */
     
/* Special function to generate a popup root widget */
g_error create_popup(int x,int y,int w,int h,struct widget **wgt,int owner);

g_error widget_create(struct widget **w,int type,
		      struct divtree *dt,struct divnode **where,
		      handle container,int owner);
g_error widget_derive(struct widget **w,int type,struct widget *parent,
		      handle hparent,int rship,int owner);
void widget_remove(struct widget *w);
g_error widget_set(struct widget *w, int property, glob data);
glob widget_get(struct widget *w, int property);

/* This is used in transparent widgets - it propagates a redraw through
   the container the widget is in, in order to redraw the background
*/
void redraw_bg(struct widget *self);

/* 
   This function should be called by an interactive widget to allocate
   a hotkey.  Its value does not need to be displayed unless there is
   no current pointing device, but it finds the first available acceptable
   hotkey.
*/
long find_hotkey(void);

/* 
   Set the current hotkey (from find_hotkey or otherwise)
   Sets the 'hotkey' widget member, and adds to the hkwidgets list
   if necessary

   If hotkey==0, the hotkey is unset
*/
void install_hotkey(struct widget *self,long hotkey);

/*
   Set a timer.  At the time, in ticks, specified by 'time',
   the widget will recieve a TRIGGER_TIMER
*/
void install_timer(struct widget *self,unsigned long interval);

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
void reset_pointer(void);

/* This is a pointing device event.
   The trigger parameter is a struct pointing_trigger.
   x and y here are absolute coords. The divtree is traversed to find the
   node at those coords.

   btn specifies the buttons down on the pointing device. Bit 0 is the
   first mouse button, bit 1 is the second, etc.
   Pointing device events should only be sent here for MOVE, UP, DOWN.
*/
void dispatch_pointing(long type,int x,int y,int btn);

/* This dispatches a key.  It is first checked against the global
   key owner table, and if not found there it is sent to the currently 
   focused widget.  The key is passed as the trigger param.
*/
void dispatch_key(long type,int key,int mods);
  
/* Dispatch a direct trigger to the widget with a matching direct_trigger.
   The param is passed directly
 */
void dispatch_direct(char *name,long param);

/* The divnode currently occupied by the pointing device */
extern struct divnode *div_under_crsr;

/* Other status variables */
/* These are needed to determine which widget is under the pointing
   device, keep track of status */
extern struct widget *under;
extern struct widget *capture;
extern struct widget *kbdfocus;

/* Customizes the button's appearance
   (used by other widgets that embed buttons in themeselves) */
void customize_button(struct widget *self,int state,int state_on,int state_hilight,
		      void *extra, void (*event)(struct widget *extra,struct widget *button));

#endif /* __WIDGET_H */

/* The End */







