/* $Id: input_util.c,v 1.2 2002/07/03 22:03:30 micahjd Exp $
 *
 * input_util.c - Collection of utilities used by the input code
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

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/appmgr.h>
#include <pgserver/widget.h>

/***************************************** Timers *******/

struct widget *timerwidgets;

/*
   Set a timer.  At the time, in ticks, specified by 'time',
   the widget will recieve a PG_TRIGGER_TIMER
*/
void install_timer(struct widget *self,u32 interval) {
  struct widget *w;

  /* Remove old links */
  remove_timer(self);

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

    send_trigger(w,PG_TRIGGER_TIMER,NULL);
  };
}

void remove_timer(struct widget *w) {
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

/***************************************** Focus *******/

/*
  A widget has requested focus... Send out the ACTIVATE and DEACTIVATE
  triggers, and update necessary vars
*/
void request_focus(struct widget *self) {
  handle hself = hlookup(self,NULL);
  struct widget *kbdfocus;

  /* Already focused? */
  if (dts->top->focus==hself) return;

  kbdfocus = NULL;
  rdhandle((void**)&kbdfocus,PG_TYPE_WIDGET,-1,dts->top->focus);

  /* Deactivate the old widget, activate the new */
  send_trigger(kbdfocus,PG_TRIGGER_DEACTIVATE,NULL);
  dts->top->focus = hself;
  appmgr_focus(appmgr_findapp(self));
  send_trigger(self,PG_TRIGGER_ACTIVATE,NULL);

  /* Scroll in */
  scroll_to_divnode(self->in->div);

  /* If there's an active hotspot cursor, move it to this widget */
  if (dts->top->hotspot_cursor) {
    int x,y;
    divnode_hotspot_position(self->in->div, &x, &y);
    cursor_move(dts->top->hotspot_cursor,x,y);
  }
}

/***************************************** Trigger utils *******/

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
		    union trigparam *param, int *stop,int forward) {
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


/* The End */






