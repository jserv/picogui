/* $Id: widget.c,v 1.184 2002/07/03 22:03:32 micahjd Exp $
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
DEF_HYBRIDWIDGET_TABLE(label,button)
DEF_WIDGET_TABLE(scroll)
DEF_STATICWIDGET_TABLE(indicator)
DEF_HYBRIDWIDGET_TABLE(label,button)    /* FIXME: This is here for binary compatibility temporarily,
					 * since the bitmap widget has been deprecated.
					 */
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
  if ((w->trigger_mask & PG_TRIGGER_NONTOOLBAR) && dt->head->next && dt->head->next->div && 
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

void widget_remove(struct widget *w) {
  struct divnode *sub_end;  
  handle hw;

  DBG("%p\n",w);

  /* Get out of the timer list */
  remove_timer(w);
  
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
   * the widget's divtree will keep on going
   * and delete other widgets' divtrees 
   */
  if (w->out) *w->out = NULL; 
  if (w->sub) *w->sub = NULL;
 
  if (w->def->remove) (*w->def->remove)(w);
  
  /* Set the flags for redraw */
  if (w->dt && w->dt->head) {
    w->dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_FORCE_CHILD_RECALC;
    w->dt->flags |= DIVTREE_NEED_RECALC;
  }   

#ifdef DEBUG_KEYS
  num_widgets--;
#endif
  g_free(w);
}

g_error inline widget_set(struct widget *w, int property, glob data) {
   g_error e;
   char *str;
   struct divnode *maindiv = w->in->div ? w->in->div : w->in;
   
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
      if (maindiv->tx != -data) {
	maindiv->tx = -data;
	maindiv->flags |= DIVNODE_SCROLL_ONLY;
	w->dt->flags |= DIVTREE_NEED_REDRAW;
	hotspot_free();
      }
      maindiv->flags |= DIVNODE_DIVSCROLL | DIVNODE_EXTEND_HEIGHT;
      break;

    case PG_WP_SCROLL_Y:
      if (data > w->in->ch-w->in->h)
	data = w->in->ch-w->in->h;
      if (data < 0)
	data = 0;
      DBG("PG_WP_SCROLL_Y: ty = %d, data = %d\n",maindiv->ty, (int)data);
      if (maindiv->ty != -data) {
	maindiv->ty = -data;
	maindiv->flags |= DIVNODE_SCROLL_ONLY | DIVNODE_NEED_RECALC;
	w->dt->flags |= DIVTREE_NEED_REDRAW;
	hotspot_free();
      }
      maindiv->flags |= DIVNODE_DIVSCROLL | DIVNODE_EXTEND_HEIGHT;
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
     maindiv->state = data;
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
   struct divnode *maindiv = w->in->div ? w->in->div : w->in;
  
   if (!(w && w->def->get))
     return 0;
   
   /* handle some universal properties */
   switch (property) {
     
   case PG_WP_ABSOLUTEX:      /* Absolute coordinates */
     activate_client_divnodes(w->owner);
     divtree_size_and_calc(w->dt);
     return maindiv->x;
   case PG_WP_ABSOLUTEY:
     activate_client_divnodes(w->owner);
     divtree_size_and_calc(w->dt);
     return maindiv->y;

   case PG_WP_WIDTH:          /* Real width and height */
     activate_client_divnodes(w->owner);
     divtree_size_and_calc(w->dt);
     return maindiv->calcw;
   case PG_WP_HEIGHT:
     activate_client_divnodes(w->owner);
     divtree_size_and_calc(w->dt);
     return maindiv->calch;
     
   case PG_WP_SCROLL_X:
     return -maindiv->tx;
   case PG_WP_SCROLL_Y:
     return -maindiv->ty;
     
   case PG_WP_SIDE:
     return w->in->flags & (~SIDEMASK);
     
   case PG_WP_SIZE:
     return w->in->split;
     
   case PG_WP_NAME:
     return w->name;
     
   case PG_WP_PUBLICBOX:
     return w->publicbox;

   case PG_WP_STATE:
     return maindiv->state;
     
   case PG_WP_BIND:
     return w->scrollbind;

   case PG_WP_TRIGGERMASK:
     return w->trigger_mask;

   case PG_WP_PREFERRED_W:
     resizewidget(w);
     return max(maindiv->pw, maindiv->cw);

   case PG_WP_PREFERRED_H:
     resizewidget(w);
     return max(maindiv->ph, maindiv->ch);

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

void resizewidget(struct widget *w) {
  /* FIXME: only resize when the size actually changes? 
   */
  (*w->def->resize)(w);
  w->dt->flags |= DIVTREE_NEED_RESIZE;
}
   
/* Iterator function used by resizeall() */
g_error resizeall_iterate(const void **p, void *extra) {
   (*((struct widget *) (*p))->def->resize)((struct widget *) (*p));
   return success;
}
/* Call the resize() function on all widgets with handles */
void resizeall(void) {
  struct divtree *tree;

  handle_iterate(PG_TYPE_WIDGET,&resizeall_iterate,NULL);
  
  for (tree=dts->top;tree;tree=tree->next)
    tree->flags |= DIVTREE_NEED_RESIZE;
}

/* Traverse to other widgets in a given direction (PG_TRAVERSE_*) */
struct widget *widget_traverse(struct widget *w, int direction, int count) {
  struct widget *p;
  struct divnode *d;
  struct app_info **appinfo_p, *appinfo;
  
  switch (direction) {

    /* Traverse to the first child, then go forward 
     */
  case PG_TRAVERSE_CHILDREN:
    if (!w)
      return NULL;
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

    /* Traverse through the application list */
  case PG_TRAVERSE_APP:
    if (w) {
      /* Find the appinfo structure associated with this widget */
      appinfo_p = appmgr_findapp(w);
      if (!appinfo_p)
	return NULL;
      
      /* Traverse the appinfo list */
      for (appinfo=*appinfo_p;appinfo && count;count--)
	appinfo = appinfo->next;
    }
    else {
      /* If they passed a 0 widget, start them off with the first app */
      appinfo = applist;
    }

    /* Return the root widget */
    if (!appinfo)
      return NULL;
    if (iserror(rdhandle((void**) &w, PG_TYPE_WIDGET, -1, appinfo->rootw)))
      return NULL;
    break;
  }

  return w;
}


/* Set flags to rebuild a widget on the next update,
 */
void set_widget_rebuild(struct widget *w) {
  struct divnode *maindiv = w->in->div ? w->in->div : w->in;

  maindiv->flags |= DIVNODE_NEED_REBUILD;
  w->dt->flags |= DIVTREE_NEED_RECALC;
}

/* Set the number of cursors occupying the widget, and send any appropriate
 * triggers due to the change.
 */
void widget_set_numcursors(struct widget *self, int num) {
  /* Don't let our puny little u8 roll over if something else fucks up */
  if (num < 0)
    num = 0;

  if (self->numcursors && !num)
    send_trigger(self,PG_TRIGGER_LEAVE,NULL);

  if (!self->numcursors && num)
    send_trigger(self,PG_TRIGGER_ENTER,NULL);  

  self->numcursors = num;
}

/* The End */








