/* $Id: widget.h,v 1.14 2000/06/01 23:11:42 micahjd Exp $
 *
 * widget.h - defines the standard widget interface used by widgets
 * This is an abstract widget framework that loosely follows the
 * Property Method Event model.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <divtree.h>
#include <font.h>
#include <video.h>
#include <g_error.h>
#include <g_malloc.h>

struct blob;
struct widgetdef;
struct widget;

/******* Structures */

/* A data type to represent anything */
typedef long glob;

/* Constants used for rship, the relationship between a widget and its
   parent */
#define DERIVE_BEFORE 0
#define DERIVE_AFTER  1
#define DERIVE_INSIDE 2

/* Constants for a trigger type. One of these constants is used to identify
   a trigger when it happens, and they are combined to form a trigger mask */
#define TRIGGER_KEY        (1<<0)  /* Any key, while focused */
#define TRIGGER_GLOBALKEY  (1<<1)  /* A registered global key */
#define TRIGGER_DIRECT     (1<<2)  /* A trigger sent explicitely */
#define TRIGGER_ACTIVATE   (1<<3)  /* Sent when it receives focus */
#define TRIGGER_DEACTIVATE (1<<4)  /* Losing focus */
#define TRIGGER_KEYUP      (1<<5)  /* Ignores autorepeat, etc. */
#define TRIGGER_KEYDOWN    (1<<6)  /* Ditto. */
#define TRIGGER_RELEASE    (1<<7)  /* Mouse up (see note) */
#define TRIGGER_UP         (1<<8)  /* Mouse up in specified divnode */
#define TRIGGER_DOWN       (1<<9)  /* Mouse down in divnode */
#define TRIGGER_MOVE       (1<<10) /* Triggers on any mouse movement in node */
#define TRIGGER_ENTER      (1<<11) /* Mouse moves inside widget */
#define TRIGGER_LEAVE      (1<<12) /* Mouse moves outside widget */
#define TRIGGER_DRAG       (1<<13) /* Mouse move when captured */

/* Note on TRIGGER_RELEASE:  This is when the mouse was pressed inside
   the widget, then released elsewhere.  */

/* Trigger param union */
union trigparam {
  struct {
    int x,y,btn;  /* Current mouse status */
    int chbtn;    /* Changed buttons */
  } mouse;
  struct {
    int key;
  } kbd;
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
  /* Defines the type of widget */
  int type;
  struct widgetdef *def;  /* (Methods) */
   
  /* These pointers indicate the input, output, and sub of the
   * local divtree */
  struct divnode *in;
  struct divnode **out;
  struct divnode **sub;
  /* This is where it was added (so it can be deleted by setting this
     back to the output of this widget */
  struct divnode **where;
   
  /* The divtree and stack this widget is part of */
  struct dtstack *ds;
  struct divtree *dt;
  
  /* If this is a root widget, an unprivelidged app can only derive
     widgets inside it, not before or after it */
  int isroot;

  /* Widget's private data (Properties) */
  void *data;

  /**** Used for management of triggers */
  
  /* widget sets this to accept triggers.  TRIGGER_* constants or'ed
     together. */
  long trigger_mask;
  /* Name of an active direct trigger */
  char *direct_trigger;
};

/* Macros to help define widgets */
#define DEF_WIDGET_TABLE(n) \
  n##_install, n##_remove, n##_trigger, n##_set, n##_get,
#define DEF_STATICWIDGET_TABLE(n) \
  n##_install, n##_remove, NULL, n##_set, n##_get,
#define DEF_WIDGET_PROTO(n) \
  g_error n##_install(struct widget *self); \
  void n##_remove(struct widget *self); \
  void n##_trigger(struct widget *self,long type,union trigparam *param); \
  g_error n##_set(struct widget *self, int property, glob data); \
  glob n##_get(struct widget *self, int property);
#define DEF_STATICWIDGET_PROTO(n) \
  g_error n##_install(struct widget *self); \
  void n##_remove(struct widget *self); \
  g_error n##_set(struct widget *self, int property, glob data); \
  glob n##_get(struct widget *self, int property);

/* Widget prototypes */
DEF_STATICWIDGET_PROTO(toolbar)      /* A container for buttons */
DEF_WIDGET_PROTO(scroll)
DEF_STATICWIDGET_PROTO(label)      /* Text, resizes to fit */
DEF_STATICWIDGET_PROTO(indicator)
DEF_STATICWIDGET_PROTO(bitmap)
DEF_WIDGET_PROTO(button)
DEF_WIDGET_PROTO(panel)

/* Types of widgets (in the same order they are in the table in widget.c) */
#define WIDGET_TOOLBAR    0
#define WIDGET_LABEL      1
#define WIDGET_SCROLL     2
#define WIDGET_INDICATOR  3
#define WIDGET_BITMAP     4
#define WIDGET_BUTTON     5
#define WIDGET_PANEL      6
#define WIDGETMAX         6    /* For error checking */
     
/* Constants for properties */
#define WP_SIZE        1
#define WP_SIDE        2
#define WP_ALIGN       3
#define WP_BGCOLOR     4
#define WP_COLOR       5
#define WP_SIZEMODE    6
#define WP_TEXT        7
#define WP_FONT        8
#define WP_TRANSPARENT 9
#define WP_BORDERCOLOR 10
#define WP_BORDERSIZE  11
#define WP_BITMAP      12
#define WP_LGOP        13
#define WP_VALUE       14
#define WP_BITMASK     15

/* Constants for SIZEMODE */
#define SZMODE_PIXEL         0
#define SZMODE_PERCENT       DIVNODE_UNIT_PERCENT

#define SZMODE_MASK          (~DIVNODE_UNIT_PERCENT)

/* Widget events */
#define WE_ACTIVATE    1  /* Gets focus (or for a non-focusing widget such
			     as a button, it has been clicked/selected  */
#define WE_DEACTIVATE  2  /* Lost focus */

/******* These functions define the 'public' methods for widgets */

g_error widget_create(struct widget **w,int type,struct dtstack *ds,
		      struct divtree *dt,struct divnode **where);
g_error widget_derive(struct widget **w,int type,struct widget *parent,
		      int rship);
void widget_remove(struct widget *w);
g_error widget_set(struct widget *w, int property, glob data);
glob widget_get(struct widget *w, int property);

/* Widget must set their trigger_mask to accept triggers.  This defines
   parameters for some of the triggers.
   
   When the trigger_mask is set to accept pointing device triggers,
   they are sent when pointing device activity is within a divnode owned
   by this widget.

   By default, no global keys are registered.  To register a key,
   set the 'key_owners' element for the key to point to the widget.
   Keys set to NULL in this array are passed on to the focused widget.
   
   For a direct trigger to be named, a trigger name must be stored in
   the widget's 'direct_trigger' string. It should uniquely identify
   the trigger, probably with the widget's title.
 */

#define NUM_KEYS 256
extern struct widget *key_owners[NUM_KEYS];

/* 
   This function should be called by an interactive widget to allocate
   a hotkey.  Its value does not need to be displayed unless there is
   no current pointing device, but it finds the first available acceptable
   hotkey.
*/
int find_hotkey(void);

/**** These are entry points for the various input drivers. */

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
void dispatch_key(long type,int key);
  
/* Dispatch a direct trigger to the widget with a matching direct_trigger.
   The param is passed directly
 */
void dispatch_direct(char *name,long param);

#endif /* __WIDGET_H */

/* The End */







