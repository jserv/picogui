/* $Id: widget.h,v 1.67 2002/09/27 01:14:07 micahjd Exp $
 *
 * widget.h - defines the standard widget interface used by widgets
 * This is an abstract widget framework that loosely follows the
 * Property Method Event model.
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
#include <pgserver/input.h>

struct blob;
struct widgetdef;
struct widget;

/******* Structures */

/* A data type to represent anything */
typedef long glob;

/* This contains function pointers for the widget methods that define
   how a widget acts and what it does.
*/
struct widgetdef {
  /* The number of classes this widget has subclassed. This is needed
   * so that private data areas for each parent class can be allocated
   */
  int subclass_num;

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

  /* Number of mouse cursors over this widget */
  u8 numcursors;

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

  /* Widget's private data- This is an array of private data pointers,
   * one for the widget itself and one for each widget it has subclassed.
   * The data in the widget structure itself is considered data for an
   * abstract "widget" base class.
   */
  void **data;

  /* The widget's optional name (string handle) */
  handle name;

  /* This is the scroll bar bound to us, set with the PG_WP_BIND property. */
  handle scrollbind;

  /* widget sets this to accept triggers.  PG_TRIGGER_* constants or'ed
     together. */
  u32 trigger_mask;

  /* Time (in ticks) for a PG_TRIGGER_TIMER */
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

# define DEF_WIDGET_TABLE(s,n) \
  {s, n##_install, n##_remove, n##_trigger, n##_set, n##_get, n##_resize},
# define DEF_HYBRIDWIDGET_TABLE(n,m) \
  {0, n##_install, m##_remove, m##_trigger, m##_set, m##_get, m##_resize},
# define DEF_STATICWIDGET_TABLE(s,n) \
  {s, n##_install, n##_remove, NULL, n##_set, n##_get, n##_resize},
# define DEF_ERRORWIDGET_TABLE(s) \
  {0, NULL, (void *) s, NULL, NULL, NULL, NULL},

#endif  /* RUNTIME_FUNCPTR */
#define DEF_WIDGET_PROTO(n) \
  g_error n##_install(struct widget *self); \
  void n##_remove(struct widget *self); \
  void n##_trigger(struct widget *self,s32 type,union trigparam *param); \
  g_error n##_set(struct widget *self, int property, glob data); \
  glob n##_get(struct widget *self, int property); \
  void n##_resize(struct widget *self);

/* Macro used by widgets to access their private data through a DATA pointer.
 * The parameters are: 
 *  n - the subclass number for the widget
 *  s - the name of the private structure
 */
#define WIDGET_DATA(n,s)       ((struct s *)(self->data[n]))
#define WIDGET_ALLOC_DATA(n,s) e = g_malloc((void**)&self->data[n],sizeof(struct s));\
                               errorcheck;\
                               memset(DATA,0,sizeof(struct s));

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
DEF_WIDGET_PROTO(dialogbox)
DEF_WIDGET_PROTO(messagedialog)
   
/* Set to the client # if a client has taken over the system resource */
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

/* Traverse to other widgets in a given direction (PG_TRAVERSE_*) */
struct widget *widget_traverse(struct widget *w, int direction, int count);

/* Set flags to rebuild a widget on the next update,
 * Assumes that w->in->div is the visible divnode.
 */
void set_widget_rebuild(struct widget *w);

/* Recursive utilities to change the divtree and container of all widgets in a tree.
 * Sets the divtree to the given value, and sets the container only if the current
 * value matches the old value given.
 */
void r_widget_setcontainer(struct widget *w, handle oldcontainer,
			   handle container, struct divtree *dt);

/* Set the number of cursors occupying the widget, and send any appropriate
 * triggers due to the change.
 */
void widget_set_numcursors(struct widget *self, int num);

/* Find a widget by name, or NULL if it doesn't exist. The widget chosen in the case of
 * duplicate names is undefined.
 */
struct widget *widget_find(const struct pgstring *name);

#endif /* __WIDGET_H */

/* The End */

