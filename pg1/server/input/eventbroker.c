/* $Id$
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/kd.h>  /* for KIOCSOUND and KDMKTONE */

#include <pgserver/input.h>
#include <pgserver/pgnet.h>
#include <pgserver/configfile.h>
#include <pgserver/timer.h>

#ifdef PM_ENABLED
# include <pm_client.h> /* to access the PocketBee Process Manager */
#endif /* PM_ENABLED */

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
/*                      PicoGUI driver messages handling                     */
/* ------------------------------------------------------------------------- */

/* Process messages comming from PicoGui */
void eventbroker_message(u32 message, u32 param, u32 *ret)
{

  TRACEF(">>> eventbroker_message\n");

  DPRINTF("message=%d param=%d\n", message, param);

  if(ret) *ret = 0;

  switch (message) {

    /* the server is up and running, we possibly need to warn the
       Resource Manager about that */
  case PGDM_READY:
#ifdef PM_ENABLED
    pm_ready ();
#endif /* PM_ENABLED */
    break;


  case PGDM_BACKLIGHT:
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

  return success;
}

/* ------------------------------------------------------------------------- */

static void eventbroker_close(void)
{
  TRACEF(">>> void eventbroker_close\n");
}

/* ------------------------------------------------------------------------- */
/*                          Driver registration                              */
/* ------------------------------------------------------------------------- */

g_error eventbroker_regfunc(struct inlib *i) {
  TRACEF(">>> eventbroker_regfunc\n");
  i->init = &eventbroker_init;
  i->close = &eventbroker_close;
  i->message = &eventbroker_message;
  return success;
}

/* ------------------------------------------------------------------------- */
#endif /* DRIVER_EVENTBROKER */

/* The End */
