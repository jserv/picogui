/* $Id: timer.c,v 1.1 2000/08/05 18:28:53 micahjd Exp $
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

static void updatetimers(int sig) {
  /* Called periodically by an alarm (or the equivalent in the running OS)
     Dispatches TRIGGER_TIMER events to widgets as necessary
  */

  printf("updatetimers()\n");
}

g_error timer_init(void) {
  struct sigaction action;
  struct itimerval itv;

  /* Get a reference point for getticks */
  gettimeofday(&first_tick,NULL);

  /* Set up the alarm handler */
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = updatetimers;
  action.sa_flags = SA_RESTART;
  sigemptyset(&action.sa_mask);
  sigaction(SIGALRM, &action, NULL);

  /* Start the timer */
  memset(&itv,0,sizeof(struct itimerval));
  itv.it_value.tv_usec = itv.it_interval.tv_usec = 1000;
  setitimer(ITIMER_REAL,&itv,NULL);

  return sucess;
}

void timer_release(void) {

}

unsigned long getticks(void) {
  static struct timeval now;

  gettimeofday(&now,NULL);
  return (now.tv_sec-first_tick.tv_sec)*1000 + 
    (now.tv_usec-first_tick.tv_usec)/1000;
}


/* The End */
