/* $Id: timer.c,v 1.3 2000/08/06 02:48:17 micahjd Exp $
 *
 * timer.c - OS-specific stuff for setting timers and
 *            figuring out how much time has passed
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

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include <timer.h>

/* This defines the maximum 
   precision of the TRIGGER_TIMER */
#define TIMERINTERVAL 50   /* In milliseconds */

static struct timeval first_tick;

/* Linked list of scheduled timers */
struct timernode {
  struct widget *w;
  unsigned long alarm_at;
  struct timernode *next;
};

struct timernode *timerlist;

static void sigalarm(int sig) {
  trigger_timer();
}

g_error timer_init(void) {
  struct itimerval itv;
  struct sigaction action;

  timerlist = NULL;

  /* Get a reference point for getticks */
  gettimeofday(&first_tick,NULL);

  /* Start the ever-repeating SIGALRM timer
   * I tried using individual setitimers for the events,
   * but it had some thread issues and sometimes it
   * would just in general act wierd.  This probably
   * isn't as efficient, but it seems to work better.
   * And hey, this is how SDL does it, so it must
   * be right, right?
   */

  /* Signal handler */
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = sigalarm;
  action.sa_flags = SA_RESTART;
  sigemptyset(&action.sa_mask);
  sigaction(SIGALRM, &action, NULL);

  /* itimer */
  memset(&itv,0,sizeof(struct itimerval));
  itv.it_interval.tv_sec = itv.it_value.tv_sec = 
    ((TIMERINTERVAL)/1000);
  itv.it_interval.tv_usec = itv.it_value.tv_usec =
    ((TIMERINTERVAL)%1000)*1000;
  setitimer(ITIMER_REAL,&itv,NULL);

  return sucess;
}

void timer_release(void) {
  struct itimerval itv;

  /* Shut off the timer */

  memset(&itv,0,sizeof(struct itimerval));
  setitimer(ITIMER_REAL,&itv,NULL);
}

unsigned long getticks(void) {
  static struct timeval now;

  gettimeofday(&now,NULL);
  return (now.tv_sec-first_tick.tv_sec)*1000 + 
    (now.tv_usec-first_tick.tv_usec)/1000;
}

/* The End */
