/*
 * widget.c - defines the standard widget interface used by widgets, and
 * handles dispatching widget events and triggers.
 *
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#include <widget.h>
#include <divtree.h>
#include <g_malloc.h>

struct widget *key_owners[NUM_KEYS];

/* Table of widgets */
struct widgetdef widgettab[] = {
DEF_STATICWIDGET_TABLE(panel)
DEF_STATICWIDGET_TABLE(label)
DEF_STATICWIDGET_TABLE(scroll)
DEF_STATICWIDGET_TABLE(indicator)
DEF_STATICWIDGET_TABLE(bitmap)
};

/******** Widget interface functions */

g_error widget_create(struct widget **w,int type,struct dtstack *ds,
			     struct divtree *dt,struct divnode **where) {
  g_error e;

  if ((type > WIDGETMAX) || (!ds) || (!dt) || (!where)) return 
      mkerror(ERRT_BADPARAM,"widget_create bad arguments");

  e = g_malloc((void **)w,sizeof(struct widget));
  if (e.type != ERRT_NONE) return e;
  memset(*w,0,sizeof(struct widget));

  (*w)->type = type;
  (*w)->def = widgettab + type;
  (*w)->ds = ds;
  (*w)->dt = dt;

  if ((*w)->def->install) (*(*w)->def->install)(*w);
  if ((*w)->in && (*w)->out) {
     /* If it has an input and output like any well-behaved widget
      * should, we can add it */
     *(*w)->out = *where;
     *where = (*w)->in;
     (*w)->where = where;
     if (*(*w)->out && (*(*w)->out)->owner)
       	(*(*w)->out)->owner->where = (*w)->out;
  }
  else
    return mkerror(ERRT_INTERNAL,
		   "widget_create Widget I/O pointers nonexistant");

  dt->head->flags |= DIVNODE_NEED_RECALC;
  dt->flags |= DIVTREE_NEED_RECALC;
  return sucess;
}

g_error widget_derive(struct widget **w,
			     int type,struct widget *parent,int rship) {
  if (rship==DERIVE_INSIDE)
    return widget_create(w,type,parent->ds,parent->dt,parent->sub);
  else if (rship==DERIVE_AFTER)
    return widget_create(w,type,parent->ds,parent->dt,parent->out);
  else if (rship==DERIVE_BEFORE)
    return widget_create(w,type,parent->ds,parent->dt,parent->where);
  else
    return mkerror(ERRT_BADPARAM,"widget_derive bad derive constant");
}

void widget_remove(struct widget *w) {
  struct divnode *sub_end;  

  /* More pointer mangling...  :)  
   * If this widget has other widgets inside of it,
   * we will need to insert the 'sub' list */

  if (w->sub && *w->sub) {
    sub_end = *w->sub;
    while (sub_end->next) sub_end = sub_end->next;
    if (w->where) *w->where = *w->sub;
    if (w->sub && *w->sub && (*w->sub)->owner)
      (*w->sub)->owner->where = w->where;
    if (w->out) sub_end->next = *w->out;
    if (w->out && *w->out && (*w->out)->owner)
      (*w->out)->owner->where = &sub_end->next;
  }
  else {
    if (w->where && w->out) *w->where = *w->out;
    if (w->out && *w->out && (*w->out)->owner)
      (*w->out)->owner->where = w->where;
  }

  /* If we don't break this link, then deleting
     the widget's divtree will keep on going
     and delete other widgets' divtrees */
  if (w->out) *w->out = NULL; 
  if (w->sub) *w->sub = NULL;
   
  if (w->def->remove) (*w->def->remove)(w);

  /* Set the flags for redraw */
  if (w->dt && w->dt->head) {
    w->dt->head->flags |= DIVNODE_NEED_RECALC;
    w->dt->flags |= DIVTREE_NEED_RECALC;
  }   

  g_free(w);
}

g_error widget_set(struct widget *w, int property, glob data) {
  if (w && w->def->set) return (*w->def->set)(w,property,data);
}

glob widget_get(struct widget *w, int property) {
  if (w && w->def->get) return (*w->def->get)(w,property);
  return 0;
}

/***** Trigger stuff **/

int find_hotkey(void) {
}

void dispatch_pointing(long type,int x,int y,int btn) {
#ifdef DEBUG
  printf("Pointing device event: 0x%08X (%d %d %c%c%c)\n",type,x,y,
	 (btn & 1) ? '*' : '-',
	 (btn & 2) ? '*' : '-',
	 (btn & 4) ? '*' : '-');
#endif
}

void dispatch_key(long type,int key) {
#ifdef DEBUG
  printf("Keyboard event: 0x%08X (#%d, '%c')\n",type,key,key);
#endif
}

void dispatch_direct(char *name,long param) {
}

/* The End */








