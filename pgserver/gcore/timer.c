/* $Id: timer.c,v 1.10 2001/02/14 05:13:18 micahjd Exp $
 *
 * timer.c - OS-specific stuff for setting timers and
 *            figuring out how much time has passed
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

#include <pgserver/common.h>

#ifdef WINDOWS
#include <windows.h>
#include <mmsystem.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#endif

#include <pgserver/timer.h>

/* This defines the maximum 
   precision of the TRIGGER_TIMER */
#define TIMERINTERVAL 50   /* In milliseconds */

/* All this code is OS-specific */

#ifdef WINDOWS
/**************** Windows */

static DWORD first_tick;
static UINT ntimer;

static void CALLBACK HandleAlarm(UINT uID,  UINT uMsg, DWORD dwUser,
				 DWORD dw1, DWORD dw2) {
  trigger_timer();
}

g_error timer_init(void) {
  /* Get a reference point for getticks */
  first_tick = timeGetTime();
  
  /* Set timer resolution */
  timeBeginPeriod(TIMERINTERVAL);

  /* Start up the repeating timer, just like
     the sigalrm handler for linux... */
  ntimer = timeSetEvent(TIMERINTERVAL,1,HandleAlarm,0,TIME_PERIODIC);
  
  return sucess;
}

void timer_release(void) {
  /* Shut off the timer */

  if (ntimer)
    timeKillEvent(ntimer);
  timeEndPeriod(TIMERINTERVAL);
}

unsigned long getticks(void) {
  return timeGetTime();
}

#else
/**************** Linux */

static struct timeval first_tick;

static void sigalarm(int sig) {
  trigger_timer();
}

g_error timer_init(void) {
  struct itimerval itv;
  struct sigaction action;

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

#endif /* WINDOWS */

/* The End */


