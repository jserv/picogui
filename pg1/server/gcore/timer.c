/* $Id$
 *
 * timer.c - OS-specific stuff for setting timers and
 *            figuring out how much time has passed
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/configfile.h>
#include <pgserver/timer.h>
#include <pgserver/os.h>
#include <limits.h>               /* For INT_MAX */

/* os_getticks() of the last activity */
u32 timer_lastactivity;

/* Timers for activating driver messages after periods of inactivity */
u32 timer_cursorhide;
u32 timer_backlightoff;
u32 timer_sleep;
u32 timer_vidblank;

/* Sorted list of timers widgets have requested, in increasing order of time */
struct widget *timer_widgets;

/* Evaluate a timer that's supposed to go off at 't',
 * return nonzero if the timer should be triggered, otherwise
 * return 0 and set up a timer for it.
 */
int timer_eval(u32 t);


/******************************************************** Public Methods */

/* Load timer values from the config file */
void timer_init(void) {
  timer_cursorhide    = get_param_int("timers","cursorhide",5000);
  timer_backlightoff  = get_param_int("timers","backlightoff",0);
  timer_sleep         = get_param_int("timers","sleep",0);
  timer_vidblank      = get_param_int("timers","vidblank",0);

  master_timer();  /* Reevaluate the next timer to trigger */
}

/* reset the inactivity timer */
void inactivity_reset() {
  inactivity_set(0);
}

/* retrieve the number of milliseconds since the last activity */
u32 inactivity_get() {
  return os_getticks() - timer_lastactivity;
}

/* Set the number of milliseconds since last activity.
 * inactivity_set(0) is equivalent to inactivity_reset().
 */
void inactivity_set(u32 t) {
  if (!t) {  
    /* Wake up the hardware */
    drivermessage(PGDM_POWER,PG_POWER_FULL,NULL);
  }
  timer_lastactivity = os_getticks() - t;
  
  if (!os_get_timer()) {
    master_timer();  /* Reevaluate the next timer to trigger */
  }
}

/*
 * Set a timer.  After the specified interval, in milliseconds,
 * the widget will recieve a PG_TRIGGER_TIMER
 */
void install_timer(struct widget *self,u32 interval) {
  struct widget *w;

  /* Remove old links */
  remove_timer(self);

  self->time = os_getticks() + interval;

  /* Stick it in the sorted timer_widgets list */
  if (timer_widgets && (timer_widgets->time < self->time)) {
    /* Find a place to insert it */

    w = timer_widgets;
    while (w->tnext && (w->tnext->time < self->time)) 
      w = w->tnext;

    /* Stick it in! */
    self->tnext = w->tnext;
    w->tnext = self;
  }
  else {
    /* The list is empty, or the new timer needs to go
       before everything else in the list */

    self->tnext = timer_widgets;
    timer_widgets = self;
  }

  master_timer();  /* Reevaluate the next timer to trigger */
}

void remove_timer(struct widget *w) {
  if (timer_widgets) {
    if (w==timer_widgets) {
      timer_widgets = w->tnext;
    }
    else {
      struct widget *p = timer_widgets;
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

/* This is the callback triggered by os_set_timer, it is the entry
 * point for all widget timers and inactivity timers.
 */
void master_timer(void) {
  struct widget *w;

  /* If nothing below needs triggering, disable the timer */
  os_set_timer(0);

  if (timer_cursorhide && timer_eval(timer_cursorhide + timer_lastactivity))
    hotspot_hide();

  if (timer_backlightoff && timer_eval(timer_backlightoff + timer_lastactivity))
    drivermessage(PGDM_BACKLIGHT,0,NULL);

  if (timer_sleep && timer_eval(timer_sleep + timer_lastactivity))
    drivermessage(PGDM_POWER,PG_POWER_SLEEP,NULL);

  if (timer_vidblank && timer_eval(timer_vidblank + timer_lastactivity))
    drivermessage(PGDM_POWER,PG_POWER_VIDBLANK,NULL);

  if (timer_widgets && timer_eval(timer_widgets->time)) {
    w = timer_widgets;
    timer_widgets = timer_widgets->tnext;
    send_trigger(w,PG_TRIGGER_TIMER,NULL);
  }
}


/******************************************************** Internal utilities */

/* Evaluate a timer that's supposed to go off at 't',
 * return nonzero if the timer should be triggered, otherwise
 * return 0 and set up a timer for it.
 */
int timer_eval(u32 t) {
  u32 now = os_getticks();

  if (t <= now)
    return 1;

  if (os_get_timer())
    os_set_timer(min(os_get_timer(), t));
  else
    os_set_timer(t);
  return 0;
}

/* The End */
