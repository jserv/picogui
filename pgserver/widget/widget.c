/* $Id: widget.c,v 1.46 2000/10/29 02:54:19 micahjd Exp $
 *
 * widget.c - defines the standard widget interface used by widgets, and
 * handles dispatching widget events and triggers.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/widget.h>
#include <pgserver/pgnet.h>

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
DEF_WIDGET_TABLE(field)
DEF_STATICWIDGET_TABLE(background)
};

/* These are needed to determine which widget is under the pointing
   device, keep track of status */
struct divnode *divmatch;
struct widget *under;
struct widget *prev_under;
int prev_btn;
struct widget *capture;
struct widget *kbdfocus;

/* Sorted (chronological order) list of widgets
   with timers
*/
struct widget *timerwidgets;

/* Set to the client # if a client has taken over the input device */
int keyboard_owner;
int pointer_owner;

int timerlock = 0;
/******** Widget interface functions */

g_error widget_create(struct widget **w,int type,
		      struct divtree *dt,struct divnode **where,
		      handle container,int owner) {
  g_error e;

  if ((type > PG_WIDGETMAX) || (!dt) || (!where)) return 
      mkerror(PG_ERRT_BADPARAM,20);

  e = g_malloc((void **)w,sizeof(struct widget));
  errorcheck;
  memset(*w,0,sizeof(struct widget));

  (*w)->owner = owner;
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
    return mkerror(PG_ERRT_INTERNAL,21);

  /* Resize for the first time */
  if ((*w)->resize) (*(*w)->resize)(*w);

  dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
  dt->flags |= DIVTREE_NEED_RECALC;
  return sucess;
}

g_error widget_derive(struct widget **w,
		      int type,struct widget *parent,
		      handle hparent,int rship,int owner) {
  switch (rship) {

  case PG_DERIVE_INSIDE:
    return widget_create(w,type,parent->dt,parent->sub,hparent,owner);

  case PG_DERIVE_AFTER:
    return widget_create(w,type,parent->dt,parent->out,parent->container,owner);

  case PG_DERIVE_BEFORE:
  case PG_DERIVE_BEFORE_OLD:
    return widget_create(w,type,parent->dt,parent->where,parent->container,owner);
   
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

#ifdef DEBUG
  printf("widget_remove(0x%08X)\n",w);
#endif

  if (!in_shutdown) {
    /* Get us out of the hotkey list */
    install_hotkey(w,0);

    /* Get out of the timer list */
    remove_from_timerlist(w);

    /* Get rid of any pointers we have to it */
    if (w==under) under = NULL;
    if (w==prev_under) prev_under = NULL;
    if (w==capture) capture = NULL;
    if (w==kbdfocus) kbdfocus = NULL;

    /* Remove inner widgets if it can be done safely
       (only remove if they have handles) */
    while (w->sub && *w->sub) {    
      if ((*w->sub)->owner && (hw = hlookup((*w->sub)->owner,NULL)))
	handle_free(-1,hw);
      else
	break;
    }
    
    if (w->sub && *w->sub) {    
      /* More pointer mangling...  :) If this widget has other 
	 widgets inside of it, we will need to insert the 'sub'
	 list. This is a desperate attempt to not segfault. */

#ifdef DEBUG
      printf("************** Relocating sub list. w=0x%08X\n",w);
#endif
      
      sub_end = *w->sub;
      while (sub_end->next) 
	sub_end = sub_end->next;

      //      if (w->where) 
      *w->where = *w->sub;

	//      if (w->sub && *w->sub && (*w->sub)->owner)
      (*w->sub)->owner->where = w->where;
	
      if (w->out) {
	sub_end->next = *w->out;
	if (*w->out && (*w->out)->owner)
	  (*w->out)->owner->where = &sub_end->next;
      }     
      else
	sub_end->next = NULL;

      //      if (w->out && *w->out && (*w->out)->owner)

    }
    else {
      if (w->out) {
	*w->where = *w->out;
       	if (*w->out && (*w->out)->owner)
	  (*w->out)->owner->where = w->where;
      }
      else
	*w->where = NULL;
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
  return mkerror(PG_ERRT_INTERNAL,23);
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
  if (iserror(rdhandle((void **)&container,PG_TYPE_WIDGET,-1,
		       self->container)) || ! container) return;

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

/*
  FIXME: implement find_hotkey!
*/
long find_hotkey(void) {
}

/*
  Installs or updates the hotkey for a widget
*/
void install_hotkey(struct widget *self,long hotkey) {

  if ((!self->hotkey) && hotkey) {
    /* Add to the hotkey widget list if this is our first hotkey */
    self->hknext = self->dt->hkwidgets;
    self->dt->hkwidgets = self;
  }
  else if (self->hotkey && (!hotkey)) {
    /* Remove us from the list */
    if (self->dt->hkwidgets) {
      if (self==self->dt->hkwidgets) {
	self->dt->hkwidgets = self->hknext;
      }
      else {
	struct widget *p = self->dt->hkwidgets;
	while (p->hknext)
	  if (p->hknext == self) {
	    /* Take us out */
	    p->hknext = self->hknext;
	    break;
	  }
	  else
	    p = p->hknext;
      }
    }
  }
  
  self->hotkey = hotkey;
}

/*
   Set a timer.  At the time, in ticks, specified by 'time',
   the widget will recieve a TRIGGER_TIMER
*/
void install_timer(struct widget *self,unsigned long interval) {
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
  if (kbdfocus==self) return;
  send_trigger(kbdfocus,TRIGGER_DEACTIVATE,NULL);
  kbdfocus = self;
  send_trigger(self,TRIGGER_ACTIVATE,NULL);
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
      && div->owner && div->build && div->owner->trigger_mask)
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

  /* Update the pointer */
  pointer->x = x;
  pointer->y = y;
  (*vid->sprite_update)(pointer);

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
  struct widget *p;
  union trigparam param;
  int suppress = 0;    /* If a keycode is used by a hotkey, it is only passed
			  to the hotkey owner for KEYDOWNs but other events
			  for that keycode should not be sent to the focused
			  widget */

  long keycode = (mods<<16) | key;     /* Combines mods and the key */
  
  /* First, process magic keys */
  if (type==TRIGGER_KEYDOWN &&
      (mods & PGMOD_CTRL) &&
      (mods & PGMOD_ALT)) {

    switch (key) {

    case PGKEY_SLASH:       /* CTRL-ALT-SLASH exits */
      request_quit();
      return;
    
#ifdef DEBUG                /* The rest only work in debug mode */

    case PGKEY_g:           /* Just for fun :) */
      guru("GURU MEDITATION #%08X\n\nCongratulations!\n"
	   "    Either you have read the source code or\n"
	   "    you have very persistantly banged your\n"
	   "    head on the keyboard ;-)",divmatch);
      return;

    case PGKEY_b:           /* CTRL-ALT-b blanks the screen */
      (*vid->clip_off)();
      (*vid->clear)();
      (*vid->update)();
      return;

    case PGKEY_u:           /* CTRL-ALT-u makes a blue screen */
      (*vid->clip_off)();
      (*vid->rect)(0,0,vid->xres,vid->yres,
		   (*vid->color_pgtohwr)(0x0000FF));
      (*vid->update)();
      return;

#endif

    }
  }

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
      post_event(evt,NULL,(type==TRIGGER_CHAR) ? key : keycode,keyboard_owner);
    return;
  }

  /* Ignore CHAR events for keys modified by anything other than shift */
  if (type == TRIGGER_CHAR && (mods & ~PGMOD_SHIFT)) return;

  /* Iterate through the hotkey-owning widgets if there's a KEYDOWN */
  p = dts->top->hkwidgets;
  while (p) {
    if (p->hotkey == keycode) {
      suppress = 1;
      if (type == TRIGGER_KEYDOWN)
	send_trigger(p,TRIGGER_HOTKEY,NULL);
    }
    p = p->hknext;
  }
  if (suppress) return;

  /* All other keypresses go to the focused widget (if any) */
  if (kbdfocus) {
    param.kbd.key = key;
    param.kbd.mods = mods;
    send_trigger(kbdfocus,type,&param);
  }
}

void dispatch_direct(char *name,long param) {
#ifdef DEBUG
  printf("Direct event: %s(0x%08X)\n",name,param);
#endif
}

/* The End */








