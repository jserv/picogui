/* $Id: eventbroker.c,v 1.6 2001/12/14 22:56:43 micahjd Exp $
 *
 * eventbroker.c - input driver to manage driver messages
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
 * Author:
 *   Philippe Ney <philippe.ney@smardata.ch>
 * 
 * Contributors:
 *   Pascal Bauermeister <pascal.bauermeister@smardata.ch>
 * 
 */

#include <pgserver/common.h>

#ifdef DRIVER_EVENTBROKER

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/kd.h>  /* for KIOCSOUND and KDMKTONE */

#include <pgserver/input.h>
#include <pgserver/pgnet.h>
#include <pgserver/configfile.h>
#include <pgserver/timer.h>

#if defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE)
# include <rm_client.h> /* to access the PocketBee ResourceManager */
#endif

#define LOCAL_DEBUG 0
#define LOCAL_TRACE 0

/* ------------------------------------------------------------------------- */

#if LOCAL_DEBUG
# define DPRINTF(x...) printf(__FILE__": " x)
# define WARNF(x...)   printf(__FILE__": " x)
#else
# define DPRINTF(x...)
# define WARNF(x...)   printf(__FILE__": " x)
# undef LOCAL_TRACE
# define LOCAL_TRACE 0
#endif

#if LOCAL_TRACE
# define TRACEF(x...)  printf(__FILE__": " x)
#else
# define TRACEF(x...)
#endif

/* ------------------------------------------------------------------------- */

static int shortbeep_freq;
static int shortbeep_len;
static int keyclick_freq;
static int keyclick_len;
  
/* ------------------------------------------------------------------------- */
/*                     Resource Manager events handling                      */
/* ------------------------------------------------------------------------- */

#if defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE)

void rm_event_callback (RMStateEvent ev)
{
  TRACEF(">>> rm_event_callback\n");
  switch(ev) {

  case RM_ST_CPU_RUNNING:
    /* The RM said the system went up: reset the timers, so that we won't
     * get a PG_POWER_SLEEP too soon !
     */
    DPRINTF("Got RM_ST_CPU_RUNNING event from the RM "
	    "=> calling inactivity_reset()\n"); 
    inactivity_reset();
    break;

  default:
    fprintf(stderr, "%s: received unexpected event from the RM: %d\n",
	    __FILE__, ev);
  }

}

#endif /* defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE) */

/* ------------------------------------------------------------------------- */
/*                      PicoGUI input events handling                        */
/* ------------------------------------------------------------------------- */

#if defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE)

static int eventbroker_fd_activate(int fd)
{
  int index;

  /* is the fd mine ? */
  if(fd!=rm_fd)
    return 0;

  /* run the RM event pump by 1 step */
  rm_loop(1);

  return 1;
}


static void eventbroker_fd_init(int *n, fd_set *readfds,
				struct timeval *timeout)
{
  /* register the RM's fd in the set of fd pgserver will watch */
  if ((*n)<(rm_fd+1))
    *n = rm_fd+1;
  if (rm_fd>0)
    FD_SET(rm_fd, readfds);
}

#endif /* defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE) */

/* ------------------------------------------------------------------------- */
/*                      PicoGUI driver messages handling                     */
/* ------------------------------------------------------------------------- */

/* Process messages comming from PicoGui */
void eventbroker_message(u32 message, u32 param, u32 *ret)
{

  TRACEF(">>> eventbroker_message\n");

  DPRINTF("message=%d param=%d\n", message, param);

  if(ret) *ret = 0;

  switch (message) {

  case PGDM_BACKLIGHT:
    switch(param) {
    }
    break;

  /* sound support through /dev/tty2 implemented in drivers/char/vt.c */
  case PGDM_SOUNDFX: {
    int freq=0, len;

    switch(param) {
    case PG_SND_SHORTBEEP:
      DPRINTF("PG_SND_SHORTBEEP\n");
      freq = shortbeep_freq;
      len = shortbeep_len;
      break;
    case PG_SND_KEYCLICK:
      DPRINTF("PG_SND_KEYCLICK\n");
      freq = keyclick_freq;
      len = keyclick_len;
      break;
    default:
      break;
    } /* switch(param) */

    if(freq) {
      /* open virtual terminal read only */
      int fd = open("/dev/tty2",O_RDONLY);

      if(fd < 1) {
	fprintf(stderr, "cannot open /dev/tty2 for KDMKTONE\n");
	break;
      }
      ioctl(fd, KDMKTONE, (freq&0xffff) | ((len&0xffff) << 16) );
      close(fd);
    }
    break; /* PGDM_SOUNDFX */
  }

  case PGDM_POWER:
    switch(param) {
    case PG_POWER_SLEEP:
#if defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE)
      DPRINTF("PG_POWER_SLEEP => rm_emit(RM_EV_IDLE)\n");
      rm_emit(RM_EV_IDLE);
#endif /* defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE) */
      break;
    case PG_POWER_OFF:
    case PG_POWER_VIDBLANK:
    case PG_POWER_FULL:
    }
    break; /* PGDM_POWER */

  default:
    break;
  } /* switch(message) */
  return;
}

/* ------------------------------------------------------------------------- */
/*                             Un/initializations                            */
/* ------------------------------------------------------------------------- */

static g_error eventbroker_init(void)
{
  TRACEF(">>> g_error eventbroker_init\n");

  /* get our params */
  shortbeep_freq  = get_param_int("eventbroker", "shortbeep_freq", 8000);
  shortbeep_len   = get_param_int("eventbroker", "shortbeep_len",    50);
  keyclick_freq   = get_param_int("eventbroker", "keyclick_freq", 16000);
  keyclick_len    = get_param_int("eventbroker", "keyclick_len",     30);

#if defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE)
  /* init the Ressources Manager */
  rm_init();

  /* set the rm event handlers */
  rm_set_event_callback(rm_event_callback);

# if !LOCAL_DEBUG
  rm_register(RM_ST_CPU_RUNNING);
# else
  DPRINTF("registering to all RM events\n");
  {
    int i;
    for(__RM_ST_INITIAL__ +1; i<__RM_LAST_EVENT__; ++i)
      rm_register(i);
  }
# endif
#endif /* defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE) */
}

/* ------------------------------------------------------------------------- */

static void eventbroker_close(void)
{
  TRACEF(">>> void eventbroker_close\n");

#if defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE)
# if !LOCAL_DEBUG
  rm_unregister(RM_ST_CPU_RUNNING);
# else
  DPRINTF("unregistering from all RM events\n");
  {
    int i;
    for(__RM_ST_INITIAL__ +1; i<__RM_LAST_EVENT__; ++i)
      rm_unregister(i);
  }
# endif

  /* Disconnects the client from the Resource Manager */
  rm_exit ();
#endif /* defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE) */
}

/* ------------------------------------------------------------------------- */
/*                          Driver registration                              */
/* ------------------------------------------------------------------------- */

g_error eventbroker_regfunc(struct inlib *i) {
  TRACEF(">>> eventbroker_regfunc\n");
  i->init = &eventbroker_init;
  i->close = &eventbroker_close;
#if defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE)
  i->fd_activate = &eventbroker_fd_activate;
  i->fd_init = &eventbroker_fd_init;
#endif /* defined(CONFIG_SOFT_CHIPSLICE) || defined(CONFIG_CHIPSLICE) */
  i->message = &eventbroker_message;
  return success;
}

/* ------------------------------------------------------------------------- */
#endif /* DRIVER_EVENTBROKER */

/* The End */
