/* $Id$
 *
 * widget.h - defines the standard widget interface used by widgets
 * This is an abstract widget framework that loosely follows the
 * Property Method Event model.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/svrtheme.h>
#include <pgserver/pgnet.h>
#include <pgserver/render.h>
#include <pgserver/input.h>
#include <pgserver/appmgr.h>

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

  /* Things every widget should have. Install will be called after
   * the widget is internally initialized and assigned a handle,
   * but before it is attached to anything.
   */
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

  /* This function will be called after the widget is
   * attached using widget_derive(). 
   */
  g_error (*post_attach)(struct widget *self, struct widget *parent, int rship);
};

/* The table of widgetdef's, indexed by type */
extern struct widgetdef widgettab[];

/* Finally, and actual widget structure */
struct widget {
   
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
  unsigned int auto_orientation;

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

  /* Data associated with each class comprising this widget.
   * Each class comprising this widget keeps information in the
   * entry corresponding to its subclass number.
   */
  struct widget_subclass {
    void *data;
    struct widgetdef *def;
  } *subclasses;

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

  /* Widgets must have a handle, for event processing and container linking
   * among other reasons...
   */
  handle h;

  /* Other widgets in pgserver may set up a callback to recieve events
   * from the widget. If the callback returns false, the widget is passed
   * on to the client normally, if it returns true the event is absorbed.
   */
  int (*callback)(int event, struct widget *from, s32 param, int owner, const u8 *data);
  struct widget *callback_owner;
};

# define DEF_WIDGET_TABLE(s,n) \
  {s, n##_install, n##_remove, n##_trigger, n##_set, n##_get, n##_resize, NULL},
# define DEF_HYBRIDWIDGET_TABLE(n,m) \
  {0, n##_install, m##_remove, m##_trigger, m##_set, m##_get, m##_resize, NULL},
# define DEF_STATICWIDGET_TABLE(s,n) \
  {s, n##_install, n##_remove, NULL, n##_set, n##_get, n##_resize, NULL},
# define DEF_ERRORWIDGET_TABLE(s) \
  {0, NULL, (void *) s, NULL, NULL, NULL, NULL, NULL},

#define DEF_WIDGET_PROTO(n) \
  g_error n##_install(struct widget *self); \
  void n##_remove(struct widget *self); \
  void n##_trigger(struct widget *self,s32 type,union trigparam *param); \
  g_error n##_set(struct widget *self, int property, glob data); \
  glob n##_get(struct widget *self, int property); \
  void n##_resize(struct widget *self); \
  g_error n##_post_attach(struct widget *self, struct widget *parent, int rship);

/* Macro used by widgets to access their private data through a DATA pointer.
 * The parameters are: 
 *  s - the name of the private structure
 *
 * The macro WIDGET_SUBCLASS should be defined to be this widget's subclass number.
 */
#define WIDGET_DATA(s)       ((struct s *)(self->subclasses[WIDGET_SUBCLASS].data))
#define WIDGET_ALLOC_DATA(s) e = g_malloc((void**)&self->subclasses[WIDGET_SUBCLASS].data,sizeof(struct s));\
                             errorcheck;\
                             memset(DATA,0,sizeof(struct s));

/* Macros to facilitate inheritance and virtual functions in widgets.
 *  n - The subclass number of this widget
 *  t - The PG_WIDGET_* constant of the parent 
 */
#define WIDGET_PARENT              self->subclasses[(WIDGET_SUBCLASS)-1].def
#define WIDGET_INSTALL_PARENT(t)   WIDGET_PARENT = &widgettab[appmgr_widget_map(t)];\
                                   WIDGET_PARENT->install(self);
#define WIDGET_REMOVE_PARENT       WIDGET_PARENT->remove(self);

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
DEF_WIDGET_PROTO(scrollbox)
DEF_WIDGET_PROTO(textedit)
DEF_WIDGET_PROTO(managedwindow)
DEF_WIDGET_PROTO(tabpage)
  
/* Set to the client # if a client has taken over the system resource */
extern int sysevent_owner;

/******* These functions define the 'public' methods for widgets */
     
g_error widget_derive(struct widget **w, handle *h, int type,struct widget *parent, handle hparent,int rship,int owner);
g_error widget_attach(struct widget *w, struct divtree *dt,struct divnode **where, handle container);
g_error widget_create(struct widget **w, handle *h, int type, struct divtree *dt, handle container, int owner);

void widget_remove(struct widget *w);
g_error widget_set(struct widget *w, int property, glob data);
glob widget_get(struct widget *w, int property);

/* Common widget properties provided for all widgets are handled here */
g_error widget_base_set(struct widget *w, int property, glob data);
glob widget_base_get(struct widget *w, int property);

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

/* Call the resizewidget()l.side = "all"
 function on all widgets with handles */
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
void r_widget_setcontainer(struct divnode *n, handle oldcontainer,
			   handle container, struct divtree *dt);

/* Set the number of cursors occupying the widget, and send any appropriate
 * triggers due to the change.
 */
void widget_set_numcursors(struct widget *self, int num, struct cursor *crsr);

/* Find a widget by name, or NULL if it doesn't exist. The widget chosen in the case of
 * duplicate names is undefined.
 */
struct widget *widget_find(const struct pgstring *name);

#endif /* __WIDGET_H */

/* The End */

