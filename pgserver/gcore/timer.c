/* $Id: timer.c,v 1.16 2001/10/29 23:57:55 micahjd Exp $
 *
 * timer.c - OS-specific stuff for setting timers and
 *            figuring out how much time has passed
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

u32 lastactivity;

/* Timers for activating driver messages after periods of inactivity */
u32 timer_cursorhide;
u32 timer_backlightoff;
u32 timer_sleep;
u32 timer_vidblank;

/* Internal function to send driver messages after periods of inactivity
 * specified in the configuration file
 */
void inactivity_check(void);

/* Load timer values from the config file */
void inactivity_init(void);

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
  inactivity_check();
  trigger_timer();
}

g_error timer_init(void) {
  inactivity_init();
  
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

u32 getticks(void) {
  return timeGetTime();
}

#else
/**************** Linux */

static struct timeval first_tick;

static void sigalarm(int sig) {
  inactivity_check();
  trigger_timer();
}

g_error timer_init(void) {
  struct itimerval itv;
  struct sigaction action;

  inactivity_init();

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

u32 getticks(void) {
  static struct timeval now;

  gettimeofday(&now,NULL);
  return (now.tv_sec-first_tick.tv_sec)*1000 + 
    (now.tv_usec-first_tick.tv_usec)/1000;
}

#endif /* WINDOWS */

/**************** OS-Neutral code */

/* reset the inactivity timer */
void inactivity_reset() {
  inactivity_set(0);
}

/* retrieve the number of milliseconds since the last activity */
u32 inactivity_get() {
  return getticks() - lastactivity;
}

/* Set the number of milliseconds since last activity.
 * inactivity_set(0) is equivalent to inactivity_reset().
 */
void inactivity_set(u32 t) {
  if (!t) {  
    /* Wake up the hardware */
    drivermessage(PGDM_POWER,PG_POWER_FULL,NULL);
  }

  lastactivity = getticks() - t;
}

/* These variables are accessed so often (10 times a second) that it
 * makes sense to not do a config lookup each time they're used
 */
void inactivity_init(void) {
  timer_cursorhide    = get_param_int("timers","cursorhide",0xFFFFFFF);
  timer_backlightoff  = get_param_int("timers","backlightoff",0xFFFFFFF);
  timer_sleep         = get_param_int("timers","sleep",0xFFFFFFF);
  timer_vidblank      = get_param_int("timers","vidblank",0xFFFFFFF);
}

void inactivity_check(void) {
  u32 now = inactivity_get()/100;
  static u32 then = 0;

  if (now == then)
    return;
  else if (now < then) {
    then = 0;
    return;
  }

  if ( now >= timer_cursorhide && then < timer_cursorhide )
    drivermessage(PGDM_CURSORVISIBLE,0,NULL);

  if ( now >= timer_backlightoff && then < timer_backlightoff )
    drivermessage(PGDM_BACKLIGHT,0,NULL);

  if ( now >= timer_sleep && then < timer_sleep )
    drivermessage(PGDM_POWER,PG_POWER_SLEEP,NULL);

  if ( now >= timer_vidblank && then < timer_vidblank )
    drivermessage(PGDM_POWER,PG_POWER_VIDBLANK,NULL);

  then = now;
}

/* The End */


