/* $Id: timer.c,v 1.2 2000/08/06 00:50:53 micahjd Exp $
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

static struct timeval first_tick;

/* Linked list of scheduled timers */
struct timernode {
  struct widget *w;
  unsigned long alarm_at;
  struct timernode *next;
};

struct timernode *timerlist;

static void sigalarm(int sig) {
#ifdef DEBUG
  printf("Enter sigalarm\n");
#endif

  trigger_timer();

#ifdef DEBUG
  printf("Leave sigalarm\n");
#endif
}

g_error timer_init(void) {
  timerlist = NULL;

  /* Get a reference point for getticks */
  gettimeofday(&first_tick,NULL);

  return sucess;
}

void timer_release(void) {
  struct itimerval itv;

  /* Probably not necessary, but shut off the timer
     for completeness
  */

  memset(&itv,0,sizeof(struct itimerval));
  setitimer(ITIMER_REAL,&itv,NULL);
}

unsigned long getticks(void) {
  static struct timeval now;

  gettimeofday(&now,NULL);
  return (now.tv_sec-first_tick.tv_sec)*1000 + 
    (now.tv_usec-first_tick.tv_usec)/1000;
}

/* Used in widget.c to set the time for the
   next call to trigger_timer
*/
unsigned long settimer(unsigned long interval) {
  struct itimerval itv;
  struct sigaction action;

#ifdef DEBUG
  printf("settimer(%lu)\n",interval);
#endif

  if (!interval) {
#ifdef DEBUG
    printf("settimer() -> trigger_timer()\n");
#endif
    trigger_timer();
    return;
  }

  /* Set up the alarm handler
     (for this thread) */

  
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = sigalarm;
  action.sa_flags = SA_RESTART;
  sigemptyset(&action.sa_mask);
  sigaction(SIGALRM, &action, NULL);

  /*
  signal(SIGALRM,sigalarm);
  */

  /* Set the timer */
  memset(&itv,0,sizeof(struct itimerval));
  itv.it_value.tv_sec     = (interval/1000);
  itv.it_value.tv_usec    = (interval%1000)*1000;
  setitimer(ITIMER_REAL,&itv,NULL);
}

/* The End */
