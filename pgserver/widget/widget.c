/* $Id: widget.c,v 1.175 2002/04/15 01:33:19 micahjd Exp $
 *
 * widget.c - defines the standard widget interface used by widgets, and
 * handles dispatching widget events and triggers.
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

#include <pgserver/common.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/hotspot.h>
#include <pgserver/timer.h>
#include <pgserver/input.h>
#include <pgserver/configfile.h>
#include <string.h>

#ifdef DEBUG_WIDGET
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

/* Table of widgets */
#ifdef RUNTIME_FUNCPTR
struct widgetdef widgettab[PG_WIDGETMAX+1];
void widgettab_init(void) {
   struct widgetdef *p = widgettab;
#else
struct widgetdef widgettab[] = {
#endif
   
DEF_STATICWIDGET_TABLE(toolbar)
DEF_STATICWIDGET_TABLE(label)
DEF_WIDGET_TABLE(scroll)
DEF_STATICWIDGET_TABLE(indicator)
DEF_STATICWIDGET_TABLE(bitmap)
DEF_WIDGET_TABLE(button)
DEF_STATICWIDGET_TABLE(panel)
DEF_WIDGET_TABLE(popup)
DEF_STATICWIDGET_TABLE(box)
DEF_WIDGET_TABLE(field)
DEF_WIDGET_TABLE(background)
DEF_HYBRIDWIDGET_TABLE(menuitem,button)

#ifdef CONFIG_WIDGET_TERMINAL
DEF_WIDGET_TABLE(terminal)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,102))
#endif

#ifdef CONFIG_WIDGET_CANVAS
DEF_WIDGET_TABLE(canvas)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,103))
#endif
				     
DEF_HYBRIDWIDGET_TABLE(checkbox,button)
DEF_HYBRIDWIDGET_TABLE(flatbutton,button)
DEF_HYBRIDWIDGET_TABLE(listitem,button)
DEF_HYBRIDWIDGET_TABLE(submenuitem,button)
DEF_HYBRIDWIDGET_TABLE(radiobutton,button)
DEF_WIDGET_TABLE(textbox)

#ifndef CONFIG_NOPANELBAR
DEF_WIDGET_TABLE(panelbar)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,94))
#endif

DEF_STATICWIDGET_TABLE(simplemenu)
};

/* These are needed to determine which widget is under the pointing
   device, keep track of status */
struct divnode *div_under_crsr, *deepest_div_under_crsr;
struct widget *under;
struct widget *lastclicked;
struct widget *prev_under;
int prev_btn;
struct widget *capture;
struct widget *kbdfocus;
int capturebtn;           /* Button that is holding the capture */
s16 last_char_key;        /* Key for the last character event received */
s16 last_pressed_key;     /* Key for the last keydown even received */

/* Sorted (chronological order) list of widgets
   with timers
*/
struct widget *timerwidgets;

/* Set to the client # if a client has taken over the resource */
int keyboard_owner;
int pointer_owner;
int sysevent_owner;
   
int timerlock = 0;

/* To save space, instead of checking whether the divtree is valid every time
 * we have to set a divtree flag, assign unattached widgets a fake divtree
 */
struct divnode fakedt_head;
struct divtree fakedt = {
  head: &fakedt_head
};

/******** Widget interface functions */
 
g_error widget_create(struct widget **w, int type, struct divtree *dt, handle container, int owner) {

   g_error e;

   DBG("type %d, container %d, owner %d\n",type,container,owner);

   if (!dt)
     dt = &fakedt;

   //
   // Check the type.
   //
   if ( type > PG_WIDGETMAX )
      return mkerror(PG_ERRT_BADPARAM, 20);

#ifdef DEBUG_KEYS
  num_widgets++;
#endif

  //
  // Allocate new widget memory and zero it out.
  //
  e = g_malloc((void **)w, sizeof(struct widget));
  errorcheck;
  memset(*w, 0, sizeof(struct widget));

  //
  // Initialize the elements we can.  Since this widget is unattached.
  //
  (*w)->owner = owner;
  (*w)->type = type;
  (*w)->def = widgettab + type;
  (*w)->dt = dt;
  (*w)->container = container;

  //
  // Manufacture an instance of the desired widget.
  //
  if ((*w)->def->install) {
     (*(*w)->def->install)(*w);
  }
  else {
      /* This widget is not supported, return the error code we
       * conveniently crammed into the 'remove' field */
      g_free(*w);
      return (g_error) (long) (*w)->def->remove;
  }

  //
  // That is is for widget creation.
  //
  return success;
  
}  // End of widget_create()

/* Recursive utilities to change the divtree and container of all widgets in a tree.
 * Sets the divtree to the given value, and sets the container only if the current
 * value matches the old value given.
 */
void r_widget_setcontainer(struct widget *w, handle oldcontainer,
			   handle container, struct divtree *dt) {
  if (!w)
    return;

  w->dt = dt;
  if (w->container == oldcontainer)
    w->container = container;

  if (w->sub && *w->sub && (*w->sub)->owner)
    r_widget_setcontainer((*w->sub)->owner,w->container,container,dt);
  if (w->out && *w->out && (*w->out)->owner)
    r_widget_setcontainer((*w->out)->owner,w->container,container,dt);
}
 
g_error widget_attach(struct widget *w, struct divtree *dt,struct divnode **where, handle container, int owner) {

  DBG("widget %p, container %d, owner %d\n",w,container,owner);
  
  if (!dt)
    dt = &fakedt;

  /* Change the container and divtree of this and all child widgets */
  if (w->sub && *w->sub && (*w->sub)->owner)
    r_widget_setcontainer((*w->sub)->owner,w->container,container,dt);
  w->dt = dt;
  w->container = container;
  
  /* If we just added a widget that can accept text input, and this is inside
   * a popup box, keep it in the nontoolbar area so keyboards still work */
  if ((w->trigger_mask & TRIGGER_NONTOOLBAR) && dt->head->next && dt->head->next->div && 
      dt->head->next->div->owner->type == PG_WIDGET_POPUP) {
    dt->head->next->div->flags |= DIVNODE_POPUP_NONTOOLBAR;
  }

  /* If this widget is already in the divtree, remove it */
  if (w->where) {
    if (w->out && *w->out) {
      *w->where = *w->out;
      if (*w->out && (*w->out)->owner)
	(*w->out)->owner->where = w->where;
    }
    else if ( w->where )
      *w->where = NULL;
    if (w->out)
      *w->out = NULL;

    /* Take off all the unnecessary divscroll flags */
    r_div_unscroll(w->in);

    /* If we just detached the widget under the mouse, tell it the mouse is gone */
    if (div_under_crsr && div_under_crsr->owner && 
	div_under_crsr->owner->dt==&fakedt) {
      union trigparam param;
      memset(&param,0,sizeof(param));
      send_trigger(div_under_crsr->owner,TRIGGER_LEAVE,&param);
    }
  }
  
  /* Add the widget to the divtree */
  if (where) {
    *w->out = *where;
    *where = w->in;
    w->where = where;
    if (w->out && *w->out && (*w->out)->owner) {
      (*w->out)->owner->where = w->out;
    }

    /* Resize for the first time */
    resizewidget(w);
  }
  else
    w->where = NULL;
  
  dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_FORCE_CHILD_RECALC;
  dt->flags |= DIVTREE_NEED_RECALC;
  return success;
}

g_error widget_derive(struct widget **w,
		      int type,struct widget *parent,
		      handle hparent,int rship,int owner) {

  g_error e;

  DBG("type %d, rship %d, parent %p, owner %d\n",type,rship,parent,owner);

  /* Allow using this to detach widgets too. Makes sense, since this is called
   * by the attachwidget request handler.
   */
  if (!parent)
    return widget_attach(*w, NULL, NULL, 0, owner);
  
  switch (rship) {

  case PG_DERIVE_INSIDE:
     if (*w == NULL ) {
        e = widget_create(w, type, parent->dt, hparent, owner);
        errorcheck;
     }
     return widget_attach(*w, parent->dt,parent->sub,hparent,owner);

  case PG_DERIVE_AFTER:
     if ( *w == NULL ) {
        e = widget_create(w, type, parent->dt, parent->container, owner);
        errorcheck;
     }
     return widget_attach(*w,parent->dt,parent->out,parent->container,owner);

  case PG_DERIVE_BEFORE:
  case PG_DERIVE_BEFORE_OLD:
     if ( *w == NULL ) {
        e = widget_create(w, type, parent->dt, parent->container, owner);
        errorcheck;
     }
     return widget_attach(*w,parent->dt,parent->where,parent->container,owner);
   
  default:
    return mkerror(PG_ERRT_BADPARAM,22);

  }
}

/* Used internally */
void remove_from_timerlist(struct widget *w) {
  if (timerwidgets) {
    if (w==timerwidgets) {
      timerwidgets = w->tnext;
    }
    else {
      struct widget *p = timerwidgets;
      while (p->tnext)
	if (p->tnext == w) {
	  /* Take us out */
	  p->tnext = w->tnext;
	  break;
	}
	else
	  p = p->tnext;
    }
  }
}

void widget_remove(struct widget *w) {
  struct divnode *sub_end;  
  handle hw;

  DBG("%p\n",w);

  if (!in_shutdown) {
    /* Get out of the timer list */
    remove_from_timerlist(w);

    /* Get rid of any pointers we have to it */
    if (w==under) under = NULL;
    if (w==lastclicked) lastclicked = NULL;
    if (w==prev_under) prev_under = NULL;
    if (w==capture) capture = NULL;
    if (w==kbdfocus) kbdfocus = NULL;

    /* Remove inner widgets if it can be done safely
       (only remove if they have handles) */
    while (w->sub && *w->sub) {    
      if ((*w->sub)->owner && (hw = hlookup((*w->sub)->owner,NULL))) {
	DBG("Removing inner widget %d\n",hw);
	handle_free(-1,hw);
      }
      else {
	DBG("Can't remove inner widget!\n");
	break;
      }
    }
    
    if (w->sub && *w->sub) {    
      /* More pointer mangling...  :) If this widget has other 
	 widgets inside of it, we will need to insert the 'sub'
	 list. This is a desperate attempt to not segfault. */

      DBG("************** Relocating sub list. w=%p\n",w);
      
      sub_end = *w->sub;
      while (sub_end->next) 
	sub_end = sub_end->next;

      if (w->where) 
	*w->where = *w->sub;
      
      if (w->sub && *w->sub && (*w->sub)->owner)
	(*w->sub)->owner->where = w->where;
	
      if (w->out) {
	sub_end->next = *w->out;
	if (*w->out && (*w->out)->owner)
	  (*w->out)->owner->where = &sub_end->next;
      }     
      else
	sub_end->next = NULL;

    }
    else {
      if (w->out && *w->out && w->where) {
	DBG("Reattaching *w->where to *w->out\n");
	*w->where = *w->out;

	/* We must make sure the new where pointer would be valid
	 * before we set it. This is important when a tree of widgets
	 * is completely self-contained within another widget, as in
	 * the case of the panelbar inside the panel widget.
	 */
       	if (*w->out && (*w->out)->owner && *w->where==(*w->out)->owner->in)
	  (*w->out)->owner->where = w->where;
      }
      else if ( w->where ) {
	DBG("Setting *w->where = NULL\n");
	*w->where = NULL;
      }
    }
    
    /* If we don't break this link, then deleting
       the widget's divtree will keep on going
       and delete other widgets' divtrees */
    if (w->out) *w->out = NULL; 
    if (w->sub) *w->sub = NULL;
  }
  
  if (w->def->remove) (*w->def->remove)(w);

  if (!in_shutdown) {
    /* Set the flags for redraw */
    if (w->dt && w->dt->head) {
      w->dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_FORCE_CHILD_RECALC;
      w->dt->flags |= DIVTREE_NEED_RECALC;
    }   
  }

#ifdef DEBUG_KEYS
  num_widgets--;
#endif
  g_free(w);
}

g_error inline widget_set(struct widget *w, int property, glob data) {
   g_error e;
   char *str;
   
   if (!(w && w->def->set))
     return mkerror(PG_ERRT_BADPARAM,23);   /* Bad widget in widget_set */
   
   /* If the widget has a handler, go with that */
   e = (*w->def->set)(w,property,data);
   if (errtype(e)!=ERRT_PASS)
     return e;
   
   /* Otherwise provide some defaults */
   switch (property) {
    
      /* Set the size, assuming initial split at w->in.
       * Calls resize handler if it exists, and sets the
       * appropriate flags.
       */
    case PG_WP_SIDE:
      if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,2);
      w->in->flags &= SIDEMASK;
      w->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC;
      resizewidget(w);
      w->dt->flags |= DIVTREE_NEED_RECALC;
      redraw_bg(w);

      if (w->auto_orientation) {
	/* Set orientation on all child widgets */

	struct widget *p;
	p = widget_traverse(w, PG_TRAVERSE_CHILDREN, 0);
	while (p) {
	  switch (data) {

	  case PG_S_LEFT:
	  case PG_S_RIGHT:
	   
	    widget_set(p, PG_WP_DIRECTION, PG_DIR_VERTICAL);

	    switch (widget_get(p, PG_WP_SIDE)) {
	      
	    case PG_S_LEFT:
	      widget_set(p, PG_WP_SIDE, PG_S_TOP);
	      break;

	    case PG_S_RIGHT:
	      widget_set(p, PG_WP_SIDE, PG_S_BOTTOM);
	      break;

	    }
	    break;

	  case PG_S_TOP:
	  case PG_S_BOTTOM:

	    widget_set(p, PG_WP_DIRECTION, PG_DIR_HORIZONTAL);

	    switch (widget_get(p, PG_WP_SIDE)) {
	      
	    case PG_S_TOP:
	      widget_set(p, PG_WP_SIDE, PG_S_LEFT);
	      break;

	    case PG_S_BOTTOM:
	      widget_set(p, PG_WP_SIDE, PG_S_RIGHT);
	      break;

	    }
	    break;

	  }
	  p = widget_traverse(p, PG_TRAVERSE_FORWARD,1);
	}
      }
      break;

    case PG_WP_SIZE:
      if (data<0) {
	/* Automatic sizing */
	w->in->flags |= DIVNODE_SIZE_AUTOSPLIT;
      }
      else {
	w->in->split = data;
	w->in->flags &= ~DIVNODE_SIZE_AUTOSPLIT;     /* No auto resizing */
      }
      w->in->flags |= DIVNODE_NEED_RECALC;
      w->dt->flags |= DIVTREE_NEED_RECALC;
      redraw_bg(w);
      break;

    case PG_WP_SIZEMODE:
      w->in->flags &= ~DIVNODE_SIZE_AUTOSPLIT;     /* No auto resizing */
      w->in->flags &= ~PG_SZMODEMASK;
      w->in->flags |= data & PG_SZMODEMASK;
      redraw_bg(w);
      break;
      
    case PG_WP_SCROLL_X:
      if (data < 0)
	data = 0;
      if (data > w->in->cw-w->in->w)
	data = w->in->cw-w->in->w;
      if (w->in->div->tx != -data) {
	w->in->div->tx = -data;
	w->in->div->flags |= DIVNODE_SCROLL_ONLY;
	w->dt->flags |= DIVTREE_NEED_REDRAW;
	hotspot_free();
      }
      w->in->div->flags |= DIVNODE_DIVSCROLL | DIVNODE_EXTEND_HEIGHT;
      break;

    case PG_WP_SCROLL_Y:
      if (data > w->in->ch-w->in->h)
	data = w->in->ch-w->in->h;
      if (data < 0)
	data = 0;
      DBG("PG_WP_SCROLL_Y: ty = %d, data = %d\n",w->in->div->ty, (int)data);
      if (w->in->div->ty != -data) {
	w->in->div->ty = -data;
	w->in->div->flags |= DIVNODE_SCROLL_ONLY | DIVNODE_NEED_RECALC;
	w->dt->flags |= DIVTREE_NEED_REDRAW;
	hotspot_free();
      }
      w->in->div->flags |= DIVNODE_DIVSCROLL | DIVNODE_EXTEND_HEIGHT;
      break;

    case PG_WP_NAME:
      if (iserror(rdhandle((void **)&str,PG_TYPE_STRING,-1,data))) 
	return mkerror(PG_ERRT_HANDLE,18);
      w->name = data;
      break;

    case PG_WP_PUBLICBOX:
      w->publicbox = data;
      break;

   case PG_WP_BIND:
     w->scrollbind = data;
     break;

   case PG_WP_THOBJ:
     w->in->div->state = data;
     resizewidget(w);
     w->in->flags |= DIVNODE_NEED_RECALC;
     w->dt->flags |= DIVTREE_NEED_RECALC;
     break;

   case PG_WP_TRIGGERMASK:
     w->trigger_mask = data;
     break;
     
   case PG_WP_HILIGHTED:
    {
      struct widget *p;
      
      //
      // Pass the message onto the other sub widgets
      //
      p = widget_traverse(w, PG_TRAVERSE_CHILDREN, 0);
      while (p) {
	widget_set(p, PG_WP_HILIGHTED, data);
	p = widget_traverse(p, PG_TRAVERSE_FORWARD,1);
      }
    }
    break;

   case PG_WP_AUTO_ORIENTATION:
     w->auto_orientation = data;
     break;

    default:
      return mkerror(PG_ERRT_BADPARAM,6);   /* Unknown property */
   }
   return success;
}

glob widget_get(struct widget *w, int property) {
   if (!(w && w->def->get))
     return 0;
   
   /* handle some universal properties */
   switch (property) {
     
   case PG_WP_ABSOLUTEX:      /* Absolute coordinates */
     return w->in->div->x;
   case PG_WP_ABSOLUTEY:
     return w->in->div->y;
     
   case PG_WP_SCROLL_X:
     return -w->in->div->tx;
   case PG_WP_SCROLL_Y:
     return -w->in->div->ty;
     
   case PG_WP_SIDE:
     return w->in->flags & (~SIDEMASK);
     
   case PG_WP_SIZE:
     return w->in->split;
     
   case PG_WP_NAME:
     return w->name;
     
   case PG_WP_PUBLICBOX:
     return w->publicbox;

   case PG_WP_STATE:
     return w->in->div->state;
     
   case PG_WP_BIND:
     return w->scrollbind;

   case PG_WP_TRIGGERMASK:
     return w->trigger_mask;

   case PG_WP_PREFERRED_W:
     resizewidget(w);
     return max(w->in->div->pw, w->in->div->cw);

   case PG_WP_PREFERRED_H:
     resizewidget(w);
     return max(w->in->div->ph, w->in->div->ch);

   case PG_WP_AUTO_ORIENTATION:
     return w->auto_orientation;
     
   default:
     return (*w->def->get)(w,property);
   }
}

/* This is used in transparent widgets - it propagates a redraw through
   the container the widget is in, in order to redraw the background
*/
void redraw_bg(struct widget *self) {
  struct widget *container;

  /* Dereference the handle */
  if (iserror(rdhandle((void **)&container,PG_TYPE_WIDGET,-1,
		       self->container)) || ! container) return;

  /* Flags! Redraws automatically propagate through all child nodes of the
     container's div.
  */
#ifndef CONFIG_NOPANELBAR
  if (container->type == PG_WIDGET_PANEL) /* Optimize for panels: don't redraw panelbar */
    container->in->div->next->flags |= DIVNODE_NEED_REDRAW;
  else
#endif
    container->in->flags |= DIVNODE_NEED_REDRAW;
  if ( container->dt ) container->dt->flags |= DIVTREE_NEED_REDRAW;
}

/***** Trigger stuff **/

/* This is called to reinit the cursor handling when a layer is
   popped from the dtstack
*/
void reset_widget_pointers(void) {
  lastclicked = under = prev_under = capture = NULL;
}

/*
   Set a timer.  At the time, in ticks, specified by 'time',
   the widget will recieve a TRIGGER_TIMER
*/
void install_timer(struct widget *self,u32 interval) {
  struct widget *w;

  /* Remove old links */
  remove_from_timerlist(self);

  self->time = getticks() + interval;

  /* Stick it in the sorted timerwidgets list */
  if (timerwidgets && (timerwidgets->time < self->time)) {
    /* Find a place to insert it */

    w = timerwidgets;
    while (w->tnext && (w->tnext->time < self->time)) 
      w = w->tnext;

    /* Stick it in! */
    self->tnext = w->tnext;
    w->tnext = self;
  }
  else {
    /* The list is empty, or the new timer needs to go
       before everything else in the list */

    self->tnext = timerwidgets;
    timerwidgets = self;
  }
}

/* Trigger and remove the next timer trigger */
void inline trigger_timer(void) {
  struct widget *w;


  /* Verify that the trigger is actually due.
   * The trigger might have been modified between
   * now and when it was set.
   */
  if (timerwidgets && getticks()>=timerwidgets->time) {
    /* Good. Take it off and trigger it */
    w = timerwidgets;
    timerwidgets = timerwidgets->tnext;

    send_trigger(w,TRIGGER_TIMER,NULL);
  };
}

/*
  A widget has requested focus... Send out the ACTIVATE and DEACTIVATE
  triggers, and update necessary vars
*/
void request_focus(struct widget *self) {
  /* Already focused? */
  if (kbdfocus==self) return;

  /* Deactivate the old widget, activate the new */
  send_trigger(kbdfocus,TRIGGER_DEACTIVATE,NULL);
  kbdfocus = self;
  appmgr_focus(appmgr_findapp(self));
  send_trigger(self,TRIGGER_ACTIVATE,NULL);

  /* If the cursor isn't already within this widget, scroll it in
   * and warp the cursor to it. This is important for
   * correct navigation with hotspots.
   */
  if (self != under && self) {
    s16 px,py;

    /* Scroll in */
    scroll_to_divnode(self->in->div);
    
    /* Pointer warp */
    divnode_hotspot_position(self->in->div,&px,&py);
    VID(coord_physicalize)(&px,&py);
    dispatch_pointing(PG_TRIGGER_MOVE,px,py,0);
    drivermessage(PGDM_CURSORVISIBLE,1,NULL);
  }
}

/*
   Finds the topmost interactive widget under the (x,y) coords.
   Recursive. On first call, div should be set
   to dts->top->head
   NULL if nothing found.
   div_under_crsr should be set to NULL ahead of time, afterwards it is the
   result.
*/
void widgetunder(int x,int y,struct divnode *div) {
  if (!div) return;
  if ( ((!((div->flags & DIVNODE_DIVSCROLL) && div->divscroll)) ||
	( div->divscroll->calcx<=x && div->divscroll->calcy<=y &&
	  (div->divscroll->calcx+div->divscroll->calcw)>x &&
	  (div->divscroll->calcy+div->divscroll->calch)>y )) &&
       div->x<=x && div->y<=y && (div->x+div->w)>x && (div->y+div->h)>y) {

    /* The cursor is inside this divnode */
    
    /* If this divnode has an interactive widget as its owner, and it
     * is visible, store it in div_under_crsr */
    if (div->owner && div->owner->trigger_mask && (div->grop || div->build))
      div_under_crsr = div;

    /* Always store the deepest match in here */
    deepest_div_under_crsr = div;

    /* Check this divnode's children */
    widgetunder(x,y,div->next);
    widgetunder(x,y,div->div);
  }
}

/* Internal function that sends a trigger to a widget if it accepts it. */
int send_trigger(struct widget *w, s32 type,
			 union trigparam *param) {
  if (w && w->def && w->def->trigger &&
      (w->trigger_mask & type)) {
    (*w->def->trigger)(w,type,param);
    return 1;
  }
  return 0;
}

/* Sends a trigger to all of a widget's children */
void r_send_trigger(struct widget *w, s32 type,
		    union trigparam *param, u16 *stop,int forward) {
  struct widget *bar;
  
  if (!w || (*stop) > 0)
    return;
  send_trigger(w,type,param);
  
  /* Also traverse the panelbar if there is one */
  if (!iserror(rdhandle((void**)&bar, PG_TYPE_WIDGET, w->owner, 
			widget_get(w,PG_WP_PANELBAR))) && bar) {
    r_send_trigger(bar, type, param, stop,0);
    if ((*stop) > 0)
      return;
  }
  
  r_send_trigger(widget_traverse(w,PG_TRAVERSE_CHILDREN,0),type,param,stop,1);
  if (forward)
    r_send_trigger(widget_traverse(w,PG_TRAVERSE_FORWARD,1),type,param,stop,1);
}

void dispatch_pointing(u32 type,s16 x,s16 y,s16 btn) {
  union trigparam param;
  s16 physx,physy;
  memset(&param,0,sizeof(param));

  if (disable_input)
    return;

  /* See if the video driver wants it instead */
  if (vid->pointing_event_hook(&type, &x, &y, &btn))
    return;

  inactivity_reset();

  if (type == TRIGGER_DOWN &&
      get_param_int("sound","click",0))
    drivermessage(PGDM_SOUNDFX,PG_SND_KEYCLICK,NULL);

  /* Convert coordinates from physical to logical */
  physx = x;
  physy = y;
  VID(coord_logicalize) (&x,&y);

  /* If this is a button up/down event and we're not already at the
   * specified coordinates, move there. This is almost completely unnecessary
   * for mice, but a must with touchscreens!
   */
  if ( (type == TRIGGER_DOWN || type == TRIGGER_UP) &&
       (x != cursor->x || y != cursor->y) )
    dispatch_pointing(TRIGGER_MOVE,physx,physy,prev_btn);
   
  param.mouse.x = x;
  param.mouse.y = y;
  param.mouse.btn = btn;
  param.mouse.chbtn = btn ^ prev_btn;
  prev_btn = btn;

#ifdef DEBUG_EVENT
  printf("Pointing event: 0x%08X (%d,%d) %d\n",type,x,y,btn);
#endif

  /* Update the cursor */
  cursor->x = x;
  cursor->y = y;
  if (!events_pending())
    VID(sprite_update) (cursor);

  /* Selfish apps that don't like using widgets can just steal the pointing
     device for themselves.  Probably not something a word processor would
     want to do.  Probably for games.
  */
  if (pointer_owner) {
    int evt=0;
    switch (type) {
    case TRIGGER_MOVE:
      evt = PG_NWE_PNTR_MOVE;
      break;
    case TRIGGER_UP:
      evt = PG_NWE_PNTR_UP;
      break;
    case TRIGGER_DOWN:
      evt = PG_NWE_PNTR_DOWN;
      break;
    }
    if (evt)
      /* Squeeze all the mouse params into a long, as follows.
	 Note that if PicoGUI is to support screens bigger than
	 4096x4096 this won't work!
	 
	 Bits 31-28:  buttons
	 Bits 27-24:  changed buttons
	 Bits 23-12:  Y
	 Bits 11-0 :  X
      */
      post_event(evt,NULL,
		 (param.mouse.btn << 28) |
		 (param.mouse.chbtn << 24) |
		 (param.mouse.y << 12) |
		 param.mouse.x,
		 pointer_owner,NULL);
    return;
  }

  if (!(dts && dts->top && dts->top->head)) {
#ifdef DEBUG_EVENT
    printf("Pointer event with invalid tree\n");
#endif
    return;   /* Without a valid tree, pointer events are meaningless */
  }

  div_under_crsr = NULL;
  deepest_div_under_crsr = NULL;
  
  /* If we need to worry about toolbars under the popup boxes, pass events
     occurring in the toolbar area to the root divtree */
  if (popup_toolbar_passthrough()) {
    struct divnode *ntb = appmgr_nontoolbar_area();

    if (x < ntb->x ||
	y < ntb->y ||
	x >= ntb->x+ntb->w ||
	y >= ntb->y+ntb->h) {
      
      /* Get a widget from the bottom layer, with the toolbars */
      widgetunder(x,y,dts->root->head);
    }
    else
      widgetunder(x,y,dts->top->head);
  }
  else
    widgetunder(x,y,dts->top->head);

  if (div_under_crsr) {
    int release_captured = 0;

    under = div_under_crsr->owner;
    
    /* Keep track of the most recently clicked widget */
    if (type==TRIGGER_DOWN) {
      lastclicked = under;
      
      /* Also, allow clicks to focus applications */
      appmgr_focus(appmgr_findapp(under));
    }

    if ((!(btn & capturebtn)) && capture && (capture!=under)) {
      release_captured = send_trigger(capture,TRIGGER_RELEASE,&param);
      capture = NULL;
    }

    /* First send the 'raw' event, then handle the cooked ones. */
    if (!release_captured)
      send_trigger(under,type,&param);

    if (type==TRIGGER_DOWN && !capture) {
      capture = under;
      capturebtn = param.mouse.chbtn;
    }

  }
  else {
    under = NULL;
    if ((!(btn & capturebtn)) && capture) {
      send_trigger(capture,TRIGGER_RELEASE,&param);
      capture = NULL;
    }
  }

  if (under!=prev_under) {
    /* Mouse has moved over a different widget */
    send_trigger(under,TRIGGER_ENTER,&param);
    send_trigger(prev_under,TRIGGER_LEAVE,&param);
    prev_under = under;
  }

  /* If a captured widget accepts TRIGGER_DRAG, send it even when the
     mouse is outside its divnodes. */
  if (type == TRIGGER_MOVE && capture)
    send_trigger(capture,TRIGGER_DRAG,&param);
}

/* Update which widget/divnode the mouse is inside, etc, when the divtree changes */
void update_pointing(void) {
  /* For now the easiest way to do this is send a fake mouse 
   * move event to the current location 
   */
  s16 x = cursor->x;
  s16 y = cursor->y;
  VID(coord_physicalize) (&x,&y);
  dispatch_pointing(TRIGGER_MOVE,x,y,prev_btn);
}

void dispatch_key(u32 type,s16 key,s16 mods) {
  struct widget *p;
  struct app_info **app;
  struct app_info *ap;
  union trigparam param;
  s32 keycode = (mods<<16) | key;     /* Combines mods and the key */
  int kflags;
  struct divtree *dt;

#ifdef DEBUG_INPUT
  printf(__FUNCTION__": type = %d, key = %d, mods = %d\n", type, key, mods);
#endif

  if (disable_input)
    return;

  /* See if the video driver wants it instead */
  if (vid->key_event_hook(&type, &key, &mods))
    return;

  inactivity_reset();

  /* For rotating arrow keys along with the rest of PicoGUI */
  VID(coord_keyrotate)(&key);

  if (type == TRIGGER_KEYDOWN &&
      get_param_int("sound","keyclick",0))
    drivermessage(PGDM_SOUNDFX,PG_SND_KEYCLICK,NULL);

  /* First, process magic 'double bucky' keys */
  if (type==TRIGGER_KEYDOWN && (mods & PGMOD_CTRL) && (mods & PGMOD_ALT))
    magic_button(key);

#ifdef DEBUG_EVENT
  printf("Keyboard event: 0x%08X (#%d, '%c') mod:0x%08X\n",type,key,key,mods);
#endif

  /* The user can optionally double-press escape to suspend the device */
#ifdef CONFIG_SUSPEND_DBLESC
  if (last_pressed_key==PGKEY_ESCAPE && type==PG_TRIGGER_KEYDOWN && key==PGKEY_ESCAPE)
    drivermessage(PGDM_POWER, PG_POWER_SLEEP, NULL);
#endif

  /* Store the last character event sent */
  switch (type) {
  case PG_TRIGGER_CHAR:
    last_char_key = key;
    break;
  case PG_TRIGGER_KEYDOWN:
    last_pressed_key = key;
    break;
  }

  /* If the keyboard has been captured by a client, redirect everything
     to them.  This would seem a bit selfish of the client to want ALL
     keyboard input, but programs like games and kiosks are traditionally
     very selfish and always dislike however the operating system or GUI
     might handle the hardware.
     Well, I guess this is for games, then.
  */
  if (keyboard_owner) {
    int evt=0;
    switch (type) {
    case TRIGGER_CHAR:
      evt = PG_NWE_KBD_CHAR;
      break;
    case TRIGGER_KEYUP:
      evt = PG_NWE_KBD_KEYUP;
      break;
    case TRIGGER_KEYDOWN:
      evt = PG_NWE_KBD_KEYDOWN;
      break;
    }
    if (evt)
      /* Pack the mods into the high word and the key into the
	 low word.
      */
      post_event(evt,NULL,(type==TRIGGER_CHAR) ? key : keycode,keyboard_owner,NULL);
    return;
  }

  /* Ignore CHAR events for keys modified by ALT or CTRL */
  if (type == TRIGGER_CHAR && (mods & (PGMOD_ALT | PGMOD_CTRL))) return;

  /* This event gets propagated until 'consume' is > 0 */
  param.kbd.key     = key;
  param.kbd.mods    = mods;
  param.kbd.consume = 0;
  param.kbd.flags   = 0;

  /*
   *  Now for the fun part... propagating this event to all the widgets.
   *  It will eventually end up at all the widgets, but the order is
   *  important. What seemed to make the most sense for PicoGUI was:
   *
   *   1. focused widget
   *   2. focused widget's children
   *   3. focused widget's ancestors (within one root widget)
   *   4. all popup widgets and their children, from top to bottom
   *   5. all root widgets and their children, in decreasing pseudo-z-order
   *
   *  Since PicoGUI doesn't have a real z-order for root widgets, the
   *  order will determined by how recently the root widget had a 
   *  focused child widget. 
   */

  /* Let widgets know we're starting a new propagation */
  for (dt=dts->top;dt;dt=dt->next)
    if (dt->head->next)
      r_send_trigger(dt->head->next->owner, TRIGGER_KEY_START, NULL, &param.kbd.consume,1);

  if (kbdfocus) {
    kflags = PG_KF_ALWAYS;

    /* Is the focused widget in the topmost app?
     */
    app = appmgr_findapp(kbdfocus);
    if (app && (*app)==applist)
      kflags |= PG_KF_APP_TOPMOST;

    /* 1. focused widget 
     */
    param.kbd.flags = kflags | PG_KF_FOCUSED;
    send_trigger(kbdfocus,type,&param);
    if (param.kbd.consume > 0)
      return;
    
    /* 2. focused widget's children
     */
    param.kbd.flags = kflags | PG_KF_CONTAINER_FOCUSED;
    r_send_trigger(widget_traverse(kbdfocus,PG_TRAVERSE_CHILDREN,0),
		   type, &param, &param.kbd.consume, 1);
    if (param.kbd.consume > 0)
      return;    
    
    /* 3. focused widget's ancestors
     */
    p = kbdfocus;
    param.kbd.flags = kflags | PG_KF_CHILD_FOCUSED;
    while ((p = widget_traverse(p, PG_TRAVERSE_CONTAINER, 1))) {
      send_trigger(p,type,&param);
      if (param.kbd.consume > 0)
	return;
    }
  }

  /* 4. Popup widgets and their children
   */
  param.kbd.flags = PG_KF_ALWAYS;  
  for (dt=dts->top;dt && dt!=dts->root;dt=dt->next) {
    if (dt->head->next)
      r_send_trigger(dt->head->next->owner, type, &param, &param.kbd.consume, 0);
    if (param.kbd.consume > 0)
      return;    
  }

  /* 5. Other root widgets and their children 
   */
  param.kbd.flags = PG_KF_ALWAYS | PG_KF_APP_TOPMOST;
  for (ap=applist;ap;ap=ap->next) {
    if (iserror(rdhandle((void**)&p,PG_TYPE_WIDGET,ap->owner,ap->rootw)))
      continue;

    r_send_trigger(p, type, &param, &param.kbd.consume, 0);
    if (param.kbd.consume > 0)
      return;

    param.kbd.flags &= ~PG_KF_APP_TOPMOST;
  }
    
  /* If none of the widgets have consumed the event, 
   * give the global hotkeys a chance
   */
  global_hotkey(key,mods,type);
}
 
void resizewidget(struct widget *w) {
  /* FIXME: only resize when the size actually changes? 
   */
  (*w->def->resize)(w);
  w->dt->flags |= DIVTREE_NEED_RESIZE;
}
   
/* Iterator function used by resizeall() */
g_error resizeall_iterate(const void **p) {
   (*((struct widget *) (*p))->def->resize)((struct widget *) (*p));
   return success;
}
/* Call the resize() function on all widgets with handles */
void resizeall(void) {
  struct divtree *tree;

  handle_iterate(PG_TYPE_WIDGET,&resizeall_iterate);
  
  for (tree=dts->top;tree;tree=tree->next)
    tree->flags |= DIVTREE_NEED_RESIZE;
}

/* Global hotkeys, and a function to load them */
u16 hotkey_left, hotkey_right, hotkey_up, hotkey_down;
u16 hotkey_activate, hotkey_next;
void reload_hotkeys(void) {
  hotkey_left     = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_LEFT);
  hotkey_right    = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_RIGHT);
  hotkey_up       = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_UP);
  hotkey_down     = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_DOWN);
  hotkey_activate = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_ACTIVATE);
  hotkey_next     = theme_lookup(PGTH_O_DEFAULT,PGTH_P_HOTKEY_NEXT);
}

/* Check for global hotkeys, like those used to move between widgets.
 * Widgets that can be focused (this includes widgets with hotspots)
 * should send unused keys here.
 */
void global_hotkey(u16 key,u16 mods, u32 type) {

#ifdef DEBUG_INPUT
   printf(__FUNCTION__": Enter\n");
#endif
  if (type == TRIGGER_KEYDOWN) {

#ifdef DEBUG_INPUT
     printf(__FUNCTION__": type == TRIGGER_KEYDOWN\n");
#endif
    if (!(mods & ~(PGMOD_CAPS|PGMOD_NUM))) {
      /* Key down, no modifiers */

      if (key == hotkey_next)
	hotspot_traverse(HOTSPOT_NEXT);
      else if (key == hotkey_up)
	hotspot_traverse(HOTSPOT_UP);
      else if (key == hotkey_down)
	hotspot_traverse(HOTSPOT_DOWN);
      else if (key == hotkey_left)
	hotspot_traverse(HOTSPOT_LEFT);
      else if (key == hotkey_right)
	hotspot_traverse(HOTSPOT_RIGHT);

      else if (key == PGKEY_ALPHA) {
	/* Nifty table for looking up the next alpha character. To find the
	 * next character in the rotation, look up the first occurance of the
	 * existing character, and the next character in the string is the
	 * character to replace it with.
	 */
	const char *alphatab = errortext(mkerror(0,3));  /* Error 3 is the table */
	const char *p;

	if (last_char_key > 255)
	  return;
	p = strchr(alphatab,last_char_key);
	if (!p)
	  return;
	p++;

	/* The new character is in '*p' now. Simulate a backspace and the
	 * new character.
	 */
	dispatch_key(PG_TRIGGER_CHAR,PGKEY_BACKSPACE,0);
	dispatch_key(PG_TRIGGER_CHAR,*p,0);
      }

    }
    
    else if (!(mods & ~(PGMOD_CAPS|PGMOD_NUM|PGMOD_SHIFT))) {
      /* Key down with shift */

      if (key == hotkey_next)
	hotspot_traverse(HOTSPOT_PREV);
    }
  } 
}

/* Traverse to other widgets in a given direction (PG_TRAVERSE_*) */
struct widget *widget_traverse(struct widget *w, int direction, int count) {
  struct widget *p;
  struct divnode *d;
  
  if (!w)
    return NULL;

  switch (direction) {

    /* Traverse to the first child, then go forward 
     */
  case PG_TRAVERSE_CHILDREN:
    if (!w->sub || !*w->sub)
      return NULL;
    return widget_traverse((*w->sub)->owner,PG_TRAVERSE_FORWARD,count);

    /* Go forward by 'count' widgets
     */
  case PG_TRAVERSE_FORWARD:
    for (;w && count;count--) {
      if (!*w->out)
	return NULL;
      w = (*w->out)->owner;
    }
    break;

    /* Go up by 'count' container levels 
     */
  case PG_TRAVERSE_CONTAINER:
    for (;w && count;count--) {
      if (iserror(rdhandle((void**) &w, PG_TYPE_WIDGET, -1, w->container)))
	return NULL;
    }
    break;

    /* Traversing backwards is harder, since there's no 'parent' pointer in the divnode
     */
  case PG_TRAVERSE_BACKWARD:
    for (;w && count;count--) {
      /* Find a suitable subtree */
      if (iserror(rdhandle((void**) &p, PG_TYPE_WIDGET, -1, w->container)))
	return NULL;
      if (p)
	d = p->in;
      else
        d = w->dt->head;

      /* Find parent divnode */
      d = divnode_findparent(d, w->in);
      if (!d)
	return NULL;
      if (d->next != w->in)   /* div child doesn't count */
	return NULL;
      w = d->owner;
    }
    break;
  }

  return w;
}



/* Set flags to rebuild a widget on the next update,
 * Assumes that w->in->div is the visible divnode.
 */
void set_widget_rebuild(struct widget *w) {
  w->in->div->flags |= DIVNODE_NEED_REBUILD;
  w->dt->flags |= DIVTREE_NEED_RECALC;
}
/* The End */








