/* $Id: widget.c,v 1.15 2000/05/05 23:00:59 micahjd Exp $
 *
 * widget.c - defines the standard widget interface used by widgets, and
 * handles dispatching widget events and triggers.
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

#include <widget.h>
#include <divtree.h>
#include <g_malloc.h>

struct widget *key_owners[NUM_KEYS];

/* Table of widgets */
struct widgetdef widgettab[] = {
DEF_STATICWIDGET_TABLE(toolbar)
DEF_STATICWIDGET_TABLE(label)
DEF_WIDGET_TABLE(scroll)
DEF_STATICWIDGET_TABLE(indicator)
DEF_STATICWIDGET_TABLE(bitmap)
DEF_WIDGET_TABLE(button)
};

/* These are needed to determine which widget is under the pointing
   device, keep track of status */
struct dtstack *dts;
struct divnode *divmatch;
struct widget *under;
struct widget *prev_under;
int prev_btn;
struct widget *capture;

/******** Widget interface functions */

g_error widget_create(struct widget **w,int type,struct dtstack *ds,
			     struct divtree *dt,struct divnode **where) {
  g_error e;

  if ((type > WIDGETMAX) || (!ds) || (!dt) || (!where)) return 
      mkerror(ERRT_BADPARAM,"widget_create bad arguments");

  dts = ds;  /* Save it for later */

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

  dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
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
  handle hw;

  /* Get rid of any pointers we have to it */
  if (w==under) under = NULL;
  if (w==prev_under) prev_under = NULL;
  if (w==capture) capture = NULL;

  /* Remove inner widgets if it can be done safely
     (only remove if they have handles) */
  while (w->sub && *w->sub) {    
    if ((*w->sub)->owner && (hw = hlookup((*w->sub)->owner,NULL)))
      handle_free(-1,hw);
    else
      break;
  }

  if (w->sub && *w->sub) {    
    /* More pointer mangling...  :)  
     * If this widget has other widgets inside of it,
     * we will need to insert the 'sub' list */
    
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
    w->dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
    w->dt->flags |= DIVTREE_NEED_RECALC;
  }   

  g_free(w);
}

g_error inline widget_set(struct widget *w, int property, glob data) {
  if (w && w->def->set) return (*w->def->set)(w,property,data);
}

glob inline widget_get(struct widget *w, int property) {
  if (w && w->def->get) return (*w->def->get)(w,property);
  return 0;
}

/***** Trigger stuff **/

int find_hotkey(void) {
}

/*
   Finds the topmost interactive widget under the (x,y) coords.
   Recursive. On first call, div should be set
   to dts->top->head
   NULL if nothing found.
   divmatch should be set to NULL ahead of time, afterwards it is the
   result.
*/
void widgetunder(int x,int y,struct divnode *div) {
  if (!div) return;
  if (div->x<=x && div->y<=y && (div->x+div->w)>x && (div->y+div->h)>y
      && div->owner && div->on_recalc && div->owner->trigger_mask)
    divmatch = div;
  widgetunder(x,y,div->next);
  widgetunder(x,y,div->div);
}

/* Internal function that sends a trigger to a widget if it accepts it. */
int send_trigger(struct widget *w, long type,
			 union trigparam *param) {
  if (w && w->def && w->def->trigger &&
      (w->trigger_mask & type)) {
    (*w->def->trigger)(w,type,param);
    return 1;
  }
  return 0;
}

void dispatch_pointing(long type,int x,int y,int btn) {
  union trigparam param;
  int call_update=0;
  int i;

  if (!(dts && dts->top && dts->top->head)) {
#ifdef DEBUG
    printf("Pointer event with invalid tree\n");
#endif
    return;   /* Without a valid tree, pointer events are meaningless */
  }

  param.mouse.x = x;
  param.mouse.y = y;
  param.mouse.btn = btn;
  param.mouse.chbtn = btn ^ prev_btn;
  prev_btn = btn;

  divmatch = NULL;
  widgetunder(x,y,dts->top->head);
  if (divmatch) {
    under = divmatch->owner;

    if (type == TRIGGER_UP && capture && (capture!=under)) {
      call_update |= send_trigger(capture,TRIGGER_RELEASE,&param);
      capture = NULL;
    }

    /* First send the 'raw' event, then handle the cooked ones. */
    call_update |= (i = send_trigger(under,type,&param));

    if (type==TRIGGER_DOWN) {
      if (i)
	capture = under;
      else
	capture = NULL;
    }

  }
  else {
    under = NULL;
    if (type == TRIGGER_UP && capture) {
      call_update |= send_trigger(capture,TRIGGER_RELEASE,&param);
      capture = NULL;
    }
  }

  if (under!=prev_under) {
    /* Mouse has moved over a different widget */
    call_update |= send_trigger(under,TRIGGER_ENTER,&param);
    call_update |= send_trigger(prev_under,TRIGGER_LEAVE,&param);
    prev_under = under;
  }

  if (call_update) {
#ifdef DEBUG
    printf("Pointer: 0x%08X (%3d %3d %c%c%c %c%c%c) @ 0x%08X in 0x%08X\n",type,x,y,
	   (btn & 1) ? '*' : '-',
	   (btn & 2) ? '*' : '-',
	   (btn & 4) ? '*' : '-',
	   (param.mouse.chbtn & 1) ? '*' : '-',
	   (param.mouse.chbtn & 2) ? '*' : '-',
	   (param.mouse.chbtn & 4) ? '*' : '-',
	   divmatch,under);
#endif

    update(dts);   /* Do all the updates at once, if any are needed */ 
  }
}

void dispatch_key(long type,int key) {
#ifdef DEBUG
  printf("Keyboard event: 0x%08X (#%d, '%c')\n",type,key,key);
#endif
}

void dispatch_direct(char *name,long param) {
#ifdef DEBUG
  printf("Direct event: %s(0x%08X)\n",name,param);
#endif
}

/* The End */








