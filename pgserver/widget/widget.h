/*
 * widget.h - defines the standard widget interface used by widgets
 * This is an abstract widget framework that loosely follows the
 * Property Method Event model.
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
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

/* This contains function pointers for the widget methods that define
   how a widget acts and what it does.
*/
struct widgetdef {
  /* Things every widget should have */
  g_error (*install)(struct widget *self);
  void (*remove)(struct widget *self);

  /* Optional, only for interactive controls */
  void (*activate)(struct widget *self);   /* If this is non-NULL, this
					      widget will get a key assigned
					      to it */
  void (*getfocus)(struct widget *self);
  void (*removefocus)(struct widget *self);
  void (*keyevent)(struct widget *self,char k);

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
  void (*on_event)(struct widget *self,int event);  /* (Events) */
   
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

  /* Widget's private data (Properties) */
  void *data;
};

/* Macros to help define widgets */
#define DEF_WIDGET_TABLE(n) \
  n##_install, n##_remove, n##_activate, n##_getfocus, n##_removefocus, \
  n##_keyevent, n##_set, n##_get,
#define DEF_STATICWIDGET_TABLE(n) \
  n##_install, n##_remove, NULL,NULL,NULL,NULL, n##_set, n##_get,
#define DEF_WIDGET_PROTO(n) \
  g_error n##_install(struct widget *self); \
  void n##_remove(struct widget *self); \
  void n##_activate(struct widget *self); \
  void n##_getfocus(struct widget *self); \
  void n##_removefocus(struct widget *self); \
  void n##_keyevent(struct widget *self, char k); \
  g_error n##_set(struct widget *self, int property, void *data); \
  void * n##_get(struct widget *self, int property);
#define DEF_STATICWIDGET_PROTO(n) \
  g_error n##_install(struct widget *self); \
  void n##_remove(struct widget *self); \
  g_error n##_set(struct widget *self, int property, glob data); \
  glob n##_get(struct widget *self, int property);

/* Widget prototypes */
DEF_STATICWIDGET_PROTO(panel)      /* A simple container widget */
DEF_STATICWIDGET_PROTO(scroll)
DEF_STATICWIDGET_PROTO(label)      /* Text, resizes to fit */
DEF_STATICWIDGET_PROTO(indicator)
DEF_STATICWIDGET_PROTO(bitmap)

/* Types of widgets (in the same order they are in the table in widget.c) */
#define WIDGET_PANEL      0
#define WIDGET_LABEL      1
#define WIDGET_SCROLL     2
#define WIDGET_INDICATOR  3
#define WIDGET_BITMAP     4
#define WIDGETMAX         4    /* For error checking */

/* Widget properties */
#define WP_PANEL_SIZE	     0
#define WP_PANEL_SIDE        1
#define WP_PANEL_COLOR       2
#define WP_PANEL_BORDERCOLOR 3
#define WP_PANEL_BORDERSIZE  4
#define WP_PANEL_SIZEMODE    5

#define WP_LABEL_SIDE        0
#define WP_LABEL_COLOR       1
#define WP_LABEL_FONT        2
#define WP_LABEL_TEXT        3
#define WP_LABEL_ALIGN       4
#define WP_LABEL_TRANSPARENT 5   /* zero to use color, nonzero to be
				    transparent */

#define WP_BITMAP_SIDE        0
#define WP_BITMAP_COLOR       1
#define WP_BITMAP_BITMAP      2   /* Call with NULL data to indicate that
				     current bitmap's data has changed and
				     needs to be redrawn (but not recalc'ed)
				  */
#define WP_BITMAP_LGOP        3
#define WP_BITMAP_ALIGN       4
#define WP_BITMAP_TRANSPARENT 5   /* zero to use color, nonzero to be
				    transparent */

#define WP_SCROLL_VALUE      0

#define WP_INDICATOR_VALUE   0
#define WP_INDICATOR_SIDE    1
#define WP_INDICATOR_WIDTH   2

/* Constants for SIZEMODE */
#define SZMODE_PIXEL         0
#define SZMODE_PERCENT       DIVNODE_UNIT_PERCENT

#define SZMODE_MASK          (~DIVNODE_UNIT_PERCENT)

/* Widget events */
#define WE_ACTIVATE          0
#define WE_CHANGE            1
#define WE_FOCUS             2
#define WE_UNFOCUS           3
#define WE_KEY               4

/******* These functions define the 'public' methods for widgets */

g_error widget_create(struct widget **w,int type,struct dtstack *ds,
		      struct divtree *dt,struct divnode **where);
g_error widget_derive(struct widget **w,int type,struct widget *parent,
		      int rship);
void widget_remove(struct widget *w);
g_error widget_set(struct widget *w, int property, glob data);
glob widget_get(struct widget *w, int property);

/**** Functions that can be used by widgets */
void trigger_event(struct widget *w, int event);

#endif /* __WIDGET_H */

/* The End */



