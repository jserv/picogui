/* $Id$
 *
 * widget.c - defines the standard widget interface used by widgets, and
 * handles dispatching widget events and triggers.
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


/******** Widget interface functions */
 
g_error widget_create(struct widget **w, handle *h, int type, 
		      struct divtree *dt, handle container, int owner) {

   g_error e;

   DBG("type %d, container %d, owner %d\n",type,container,owner);

   type = appmgr_widget_map(type);

   if (!dt)
     dt = DT_NIL;

   /* Check the type.
    */
   if ( type > PG_WIDGETMAX )
      return mkerror(PG_ERRT_BADPARAM, 20);

   /* Check if it's supported
    */
   if (!widgettab[type].install)
     return (g_error) (long) widgettab[type].remove;

#ifdef DEBUG_KEYS
  num_widgets++;
#endif

  /* Allocate new widget memory and zero it out.
   */
  e = g_malloc((void **)w, sizeof(struct widget));
  errorcheck;
  memset(*w, 0, sizeof(struct widget));

  /* Initialize the elements we can.  Since this widget is unattached.
   */
  (*w)->owner = owner;
  (*w)->type = type;
  (*w)->def = widgettab + type;
  (*w)->dt = dt;
  (*w)->container = container;

  /* Allocate data pointers for this widget and everythign it's subclassed from */
  e = g_malloc((void**)&(*w)->subclasses, sizeof(struct widget_subclass) * 
	       ((*w)->def->subclass_num + 1));
  errorcheck;

  /* Allocate a handle for this widget before calling install, so that it may
   * use the handle when setting up subwidgets.
   */
  e = mkhandle(h,PG_TYPE_WIDGET,owner,*w);
  errorcheck;
  (*w)->h = *h;

  /* Initialize this instance of the widget. This install function should initialize
   * the widget it's subclassed from first, if any
   */
  (*(*w)->def->install)(*w);

  return success;
}

/* Recursive utilities to change the divtree and container of all widgets in a tree.
 * Sets the divtree to the given value, and sets the container only if the current
 * value matches the old value given. This doesn't use widget_traverse but instead
 * visits every divnode, so as not to miss widgets embedded in other widgets.
 */
void r_widget_setcontainer(struct divnode *n, handle oldcontainer,
			   handle container, struct divtree *dt) {  
  if (!n)
    return;

  if (n->owner) {
    n->owner->dt = dt;
    if (n->owner->container == oldcontainer)
      n->owner->container = container;
  }    

  r_widget_setcontainer(n->div,oldcontainer,container,dt);
  r_widget_setcontainer(n->next,oldcontainer,container,dt);
}
 
g_error widget_attach(struct widget *w, struct divtree *dt,struct divnode **where, handle container) {

  DBG("widget %p, container %d\n",w,container);
  
  if (!dt)
    dt = &fakedt;

  /* Recalc the old attach point */
  if (w->dt && w->dt->head) {
    w->dt->head->flags |= DIVNODE_NEED_RECALC;
    w->dt->flags |= DIVTREE_NEED_RECALC | DIVTREE_NEED_RESIZE;
  }

  /* Change the container and divtree of this and all child widgets */
  if (w->sub)
    r_widget_setcontainer(*w->sub,w->container,container,dt);
  w->dt = dt;
  w->container = container;
  
  /* If this widget is already in the divtree, remove it */
  if (w->out) {
    if (w->where)
      *w->where = *w->out;
    /* Make sure that the attachment point is connected to the _beginning_
     * of another widget. (Necessary to prevent improper detachment when
     * a widget is embedded within another)
     */
    if (*w->out && (*w->out)->owner && (*w->out)==(*w->out)->owner->in)
      (*w->out)->owner->where = w->where;
    *w->out = NULL;
  }
  else if (w->where)
    *w->where = NULL;

  /* Take off all the unnecessary divscroll flags */
  r_div_unscroll(w->in);
  
  /* Add the widget to the divtree */
  w->where = where;
  if (where) {
    *w->out = *where;
    *where = w->in;
    if (w->out && *w->out && (*w->out)->owner) {
      (*w->out)->owner->where = w->out;
    }

    /* If we just added a widget that can accept text input, and this is inside
     * a popup box, keep it in the nontoolbar area so keyboards still work */
    if ((w->trigger_mask & PG_TRIGGER_NONTOOLBAR) && dt->head->next && dt->head->next->div && 
	dt->head->next->div->owner->type == PG_WIDGET_POPUP) {
      dt->head->next->div->flags |= DIVNODE_POPUP_NONTOOLBAR;
    }

    /* Resize for the first time */
    resizewidget(w);
  }
  
  /* Recalc the new attach point */
  if (dt && dt->head) {
    dt->head->flags |= DIVNODE_NEED_RECALC;
    dt->flags |= DIVTREE_NEED_RECALC | DIVTREE_NEED_RESIZE;
  }
  return success;
}

g_error widget_derive(struct widget **w, handle *h,
		      int type,struct widget *parent,
		      handle hparent,int rship,int owner) {

  g_error e;

  DBG("type %d, rship %d, parent %p, owner %d\n",type,rship,parent,owner);

  /* Allow using this to detach widgets too. Makes sense, since this is called
   * by the attachwidget request handler.
   */
  if (!parent)
    return widget_attach(*w, NULL, NULL, 0);
  
  switch (rship) {

  case PG_DERIVE_INSIDE:
     if (*w == NULL ) {
        e = widget_create(w,h, type, parent->dt, hparent, owner);
        errorcheck;
     }
     e = widget_attach(*w, parent->dt,parent->sub,hparent);
     break;

  case PG_DERIVE_AFTER:
     if ( *w == NULL ) {
        e = widget_create(w,h, type, parent->dt, parent->container, owner);
        errorcheck;
     }
     e = widget_attach(*w,parent->dt,parent->out,parent->container);
     break;

  case PG_DERIVE_BEFORE:
  case PG_DERIVE_BEFORE_OLD:
     if ( *w == NULL ) {
        e = widget_create(w,h, type, parent->dt, parent->container, owner);
        errorcheck;
     }
     e = widget_attach(*w,parent->dt,parent->where,parent->container);
     break;
     
  default:
    return mkerror(PG_ERRT_BADPARAM,22);

  }
  
  /* Error checking code common to all cases */
  if (iserror(e)) {
    widget_remove(*w);
    errorcheck;
  }

  if ((*w)->def->post_attach) {
    e = (*w)->def->post_attach(*w,parent,rship);
    errorcheck;
  }

  return success;
}

void widget_remove(struct widget *w) {
  struct widget *child;
  struct divnode **old_where;
  DBG("%p, type %d\n",w,w->type);

  /* Get out of the timer list */
  remove_timer(w);

  /* Detach the widget from the widget tree */
  old_where = w->where;
  widget_attach(w,NULL,NULL,0);

  /* Detach all children from this widget */
  while ((child = widget_traverse(w,PG_TRAVERSE_CHILDREN,0)) && child->where) {
    DBG("removing child %p, type %d. where %p\n",child,child->type,child->where);
    widget_attach(child,NULL,NULL,0);
  }

  /* Note that the widget may have it's 'sub' attachment point filled even
   * if it doesn't have any real children, if it was used inside another widget.
   * We need to reattach any child divnodes this widget still has back to its insertion
   * point, so that they are properly deleted when the widget owning this subtree is
   * finished removing its component pieces.
   * Note that we're attaching it to the widget's former "where" attachment point,
   * since by this time it's been detached from the widget tree and w->where
   * should be NULL.
   */
  if (w->out && *w->out && w->sub && *w->sub) {
    /* If this widget has two subtrees, we need to append them into just one
     * before we link this back into the parent's subtree. A messy process, but
     * cleaner than the alternative (voodoo memory management :)
     */

    struct divnode *n;
    n = *w->sub;
    while (n->next) n = n->next;
    n->next = *w->out;
    if (old_where)
      *old_where = *w->sub;
    *w->out = *w->sub = NULL;
  }
  /* If there's only one child, link it directly */
  else if (w->out && *w->out) {
    if (old_where)
      *old_where = *w->out;
    *w->out = NULL;
  }
  else if (w->sub && *w->sub) {
    if (old_where)
      *old_where = *w->sub;
    *w->sub = NULL;
  }

  /* Free the widget's private data and divnodes */
  if (w->def->remove) (*w->def->remove)(w);
  
  /* Free the array of subclass data */
  g_free(w->subclasses);

  /* Free the widget itself */
#ifdef DEBUG_KEYS
  num_widgets--;
#endif
  g_free(w);
}

g_error widget_set(struct widget *w, int property, glob data) {
   g_error e;
   
   if (!(w && w->def->set))
     return mkerror(PG_ERRT_BADPARAM,23);   /* Bad widget in widget_set */
   
   /* If the widget has a handler, go with that */
   e = (*w->def->set)(w,property,data);
   if (errtype(e)!=ERRT_PASS)
     return e;
   
   return widget_base_set(w,property,data);
}

glob widget_get(struct widget *w, int property) {  
  if (!(w && w->def->get))
    return 0;
  return (*w->def->get)(w,property);
}

g_error widget_base_set(struct widget *w, int property, glob data) {
  char *str;
  struct divnode *maindiv = w->in->div ? w->in->div : w->in;

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
	   
	  if (w->auto_orientation & PG_AUTO_DIRECTION)
	    widget_set(p, PG_WP_DIRECTION, PG_DIR_VERTICAL);

	  if (w->auto_orientation & PG_AUTO_SIDE)
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

	  if (w->auto_orientation & PG_AUTO_DIRECTION)
	    widget_set(p, PG_WP_DIRECTION, PG_DIR_HORIZONTAL);

	  if (w->auto_orientation & PG_AUTO_SIDE)
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
      w->dt->flags |= DIVTREE_NEED_RESIZE;
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
    if (data > w->in->child.w - w->in->r.w)
      data = w->in->child.w - w->in->r.w;
    if (data < 0)
      data = 0;
    if (maindiv->translation.x != -data) {
      maindiv->translation.x = -data;
      maindiv->flags |= DIVNODE_SCROLL_ONLY | DIVNODE_NEED_RECALC;
      w->dt->flags |= DIVTREE_NEED_REDRAW;
      hotspot_free();
    }
    maindiv->flags |= DIVNODE_DIVSCROLL | DIVNODE_EXTEND_WIDTH;
    break;

  case PG_WP_SCROLL_Y:
    if (data > w->in->child.h - w->in->r.h)
      data = w->in->child.h - w->in->r.h;
    if (data < 0)
      data = 0;
    if (maindiv->translation.y != -data) {
      maindiv->translation.y = -data;
      maindiv->flags |= DIVNODE_SCROLL_ONLY | DIVNODE_NEED_RECALC;
      w->dt->flags |= DIVTREE_NEED_REDRAW;
      hotspot_free();
    }
    maindiv->flags |= DIVNODE_DIVSCROLL | DIVNODE_EXTEND_HEIGHT;
    break;

  case PG_WP_NAME:
    if (iserror(rdhandle((void **)&str,PG_TYPE_PGSTRING,-1,data))) 
      return mkerror(PG_ERRT_HANDLE,18);
    w->name = handle_canonicalize((handle) data);
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

glob widget_base_get(struct widget *w, int property) {  
  struct divnode *maindiv = w->in->div ? w->in->div : w->in;
  
  /* handle some universal properties */
  switch (property) {
    
  case PG_WP_ABSOLUTEX:      /* Absolute coordinates */
    /*
    activate_client_divnodes(w->owner);
    divtree_size_and_calc(w->dt);
    */
    return maindiv->r.x;
  case PG_WP_ABSOLUTEY:
    /*
    activate_client_divnodes(w->owner);
    divtree_size_and_calc(w->dt);
    */
    return maindiv->r.y;
    
  case PG_WP_WIDTH:          /* Real width and height */
    activate_client_divnodes(w->owner);
    divtree_size_and_calc(w->dt);
    return maindiv->calc.w;
  case PG_WP_HEIGHT:
    activate_client_divnodes(w->owner);
    divtree_size_and_calc(w->dt);
    return maindiv->calc.h;
    
  case PG_WP_SCROLL_X:
    return -maindiv->translation.x;
  case PG_WP_SCROLL_Y:
    return -maindiv->translation.y;
    
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
    return max(maindiv->preferred.w, maindiv->child.w);
    
  case PG_WP_PREFERRED_H:
    resizewidget(w);
    return max(maindiv->preferred.h, maindiv->child.h);
    
  case PG_WP_AUTO_ORIENTATION:
    return w->auto_orientation;

  case PG_WP_TYPE:
    return w->type;

  }
  return 0;
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
    container->in->div->div->next->flags |= DIVNODE_NEED_REDRAW;
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
    /* Make sure not only that this widget has an occupied child
     * attachment point, but make sure that the thing attached to it is
     * really the "in" connector on another widget. This is necessary to avoid
     * an improper result when this is called with a widget used inside another
     * widget as a container.
     */
    if (!w->sub || !*w->sub || (*w->sub)->owner->in != *w->sub)
      return NULL;
    return widget_traverse((*w->sub)->owner,PG_TRAVERSE_FORWARD,count);

    /* Go forward by 'count' widgets
     */
  case PG_TRAVERSE_FORWARD:
    for (;w && count;count--) {
      if (!w->out || !*w->out || (*w->out)->owner->in != *w->out)
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
void widget_set_numcursors(struct widget *self, int num, struct cursor *crsr) {
  union trigparam param;

  /* Don't let our puny little u8 roll over if something else fucks up */
  if (num < 0)
    num = 0;

  cursor_getposition(crsr, &param.mouse.x, &param.mouse.y, NULL);
  param.mouse.btn = crsr->prev_buttons;
  param.mouse.chbtn = 0;
  param.mouse.cursor = crsr;

  if (self->numcursors && !num)
    send_trigger(self,PG_TRIGGER_LEAVE,&param);

  if (!self->numcursors && num)
    send_trigger(self,PG_TRIGGER_ENTER,&param);  

  self->numcursors = num;
}

/* Communication between widget_find and iterator */
struct widget_find_data {
  struct widget *result;
  const struct pgstring *string;
};

/* Iterator function for widget_find */
g_error widget_find_iterator(const void **p, void *extra) {
  struct widget *w = (struct widget *) (*p);
  const struct pgstring *str;
  struct widget_find_data *data = (struct widget_find_data *) extra;

  if (iserror(rdhandle((void**)&str,PG_TYPE_PGSTRING,-1,w->name)) || !str)
    return success;
  if (!pgstring_cmp(data->string,str))
    data->result = w;
  return success;
}

/* Find a widget by name, or NULL if it doesn't exist. The widget chosen in the case of
 * duplicate names is undefined.
 */
struct widget *widget_find(const struct pgstring *name) {
  struct widget_find_data iterator_data;
  iterator_data.result = NULL;
  iterator_data.string = name;
  handle_iterate(PG_TYPE_WIDGET,&widget_find_iterator,&iterator_data);
  return iterator_data.result;
}

/* The End */








