/* $Id: widget.c,v 1.25 2000/06/11 17:59:18 micahjd Exp $
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
#include <pgnet.h>

struct widget *key_owners[NUM_KEYS];

/* Table of widgets */
struct widgetdef widgettab[] = {
DEF_STATICWIDGET_TABLE(toolbar)
DEF_STATICWIDGET_TABLE(label)
DEF_WIDGET_TABLE(scroll)
DEF_STATICWIDGET_TABLE(indicator)
DEF_STATICWIDGET_TABLE(bitmap)
DEF_WIDGET_TABLE(button)
DEF_WIDGET_TABLE(panel)
DEF_STATICWIDGET_TABLE(popup)
DEF_STATICWIDGET_TABLE(box)
};

/* These are needed to determine which widget is under the pointing
   device, keep track of status */
struct divnode *divmatch;
struct widget *under;
struct widget *prev_under;
int prev_btn;
struct widget *capture;
struct widget *kbdfocus;

/* Set to the client # if a client has taken over the input device */
int keyboard_owner;
int pointer_owner;

/******** Widget interface functions */

g_error widget_create(struct widget **w,int type,
		      struct divtree *dt,struct divnode **where,
		      handle container) {
  g_error e;

  if ((type > WIDGETMAX) || (!dt) || (!where)) return 
      mkerror(ERRT_BADPARAM,"widget_create bad arguments");

  e = g_malloc((void **)w,sizeof(struct widget));
  if (e.type != ERRT_NONE) return e;
  memset(*w,0,sizeof(struct widget));

  (*w)->type = type;
  (*w)->def = widgettab + type;
  (*w)->dt = dt;
  (*w)->container = container;

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
		      int type,struct widget *parent,
		      handle hparent,int rship) {
  if (rship==DERIVE_INSIDE)
    return widget_create(w,type,parent->dt,parent->sub,hparent);
  else if (rship==DERIVE_AFTER)
    return widget_create(w,type,parent->dt,parent->out,parent->container);
  else if (rship==DERIVE_BEFORE)
    return widget_create(w,type,parent->dt,parent->where,parent->container);
  else
    return mkerror(ERRT_BADPARAM,"widget_derive bad derive constant");
}

void widget_remove(struct widget *w) {
  struct divnode *sub_end;  
  handle hw;

  if (!in_shutdown) {
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
  }
  
  if (w->def->remove) (*w->def->remove)(w);

  if (!in_shutdown) {
    /* Set the flags for redraw */
    if (w->dt && w->dt->head) {
      w->dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
      w->dt->flags |= DIVTREE_NEED_RECALC;
    }   
  }
  
  g_free(w);
}

g_error inline widget_set(struct widget *w, int property, glob data) {
  if (w && w->def->set) return (*w->def->set)(w,property,data);
  return mkerror(ERRT_INTERNAL,"Widget is not settable?");
}

glob inline widget_get(struct widget *w, int property) {
  if (w && w->def->get) return (*w->def->get)(w,property);
  return 0;
}

/* This is used in transparent widgets - it propagates a redraw through
   the container the widget is in, in order to redraw the background
*/
void redraw_bg(struct widget *self) {
  struct widget *container;

  /* Dereference the handle */
  if (rdhandle((void **)&container,TYPE_WIDGET,-1,self->container).type!=
      ERRT_NONE || ! container) return;

  /* Flags! Redraws automatically propagate through all child nodes of the
     container's div.
  */
  container->in->flags |= DIVNODE_NEED_RECALC;
  container->dt->flags |= DIVTREE_NEED_RECALC;
}

/***** Trigger stuff **/

/* This is called to reinit the cursor handling when a layer is
   popped from the dtstack
*/
void reset_pointer(void) {
  under = prev_under = capture = NULL;
}

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
  int i;

  param.mouse.x = x;
  param.mouse.y = y;
  param.mouse.btn = btn;
  param.mouse.chbtn = btn ^ prev_btn;
  prev_btn = btn;

  /* Selfish apps that don't like using widgets can just steal the pointing
     device for themselves.  Probably not something a word processor would
     want to do.  Probably for games.
  */
  if (pointer_owner) {
    int evt=0;
    switch (type) {
    case TRIGGER_MOVE:
      evt = WE_PNTR_MOVE;
      break;
    case TRIGGER_UP:
      evt = WE_PNTR_UP;
      break;
    case TRIGGER_DOWN:
      evt = WE_PNTR_DOWN;
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
		 keyboard_owner);
    return;
  }

  if (!(dts && dts->top && dts->top->head)) {
#ifdef DEBUG
    printf("Pointer event with invalid tree\n");
#endif
    return;   /* Without a valid tree, pointer events are meaningless */
  }

  divmatch = NULL;
  widgetunder(x,y,dts->top->head);
  if (divmatch) {
    under = divmatch->owner;

    if ((type == TRIGGER_UP || !btn) && capture && (capture!=under)) {
      send_trigger(capture,TRIGGER_RELEASE,&param);
      capture = NULL;
    }

    /* First send the 'raw' event, then handle the cooked ones. */
    i = send_trigger(under,type,&param);

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

void dispatch_key(long type,int key,int mods) {
#ifdef DEBUG
  printf("Keyboard event: 0x%08X (#%d, '%c') mod:0x%08X\n",type,key,key,mods);
#endif

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
      evt = WE_KBD_CHAR;
      break;
    case TRIGGER_KEYUP:
      evt = WE_KBD_KEYUP;
      break;
    case TRIGGER_KEYDOWN:
      evt = WE_KBD_KEYDOWN;
      break;
    }
    if (evt)
      /* Pack the mods into the high word and the key into the
	 low word.
      */
      post_event(evt,NULL,(mods<<16)|key,keyboard_owner);
    return;
  }


}

void dispatch_direct(char *name,long param) {
#ifdef DEBUG
  printf("Direct event: %s(0x%08X)\n",name,param);
#endif
}

/* The End */








