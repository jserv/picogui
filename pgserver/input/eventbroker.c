/* $Id: eventbroker.c,v 1.1 2001/11/09 17:31:06 pney Exp $
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

#include <fcntl.h>     /* File control definitions (provide O_RDONLY) */
#include <linux/kd.h>  /* for KIOCSOUND and KDMKTONE */

#include <pgserver/input.h>
#include <pgserver/pgnet.h>
#include <pgserver/configfile.h>

#include <rm_client.h>


/******************************************** Implementations */

void eventbroker_fd_activate(int fd) {
}

static g_error eventbroker_init(void) {

}

static void eventbroker_fd_init(int *n,
				fd_set *readfds,
				struct timeval *timeout) {
}

static void eventbroker_close(void){
}



/* message between driver to provide sound (for exemple) */
void eventbroker_message(u32 message, u32 param, u32 *ret) {

  int snd_type;
  *ret = 0;

  switch (message) {

#ifdef DRIVER_CHIPSLICE_SND
  /* sound support through /dev/tty2 implemented in drivers/char/vt.c */
  case PGDM_SOUNDFX:

    switch(param) {

    case PG_SND_SHORTBEEP:
      snd_type = KDMKTONE;
      break;

    case PG_SND_KEYCLICK:
      snd_type = KDMKTONE;
      break;

    default:
      break;
    }

# if defined(CONFIG_XCOPILOT) || defined(CONFIG_SOFT_CHIPSLICE)
    printf("beep\n");

# elif defined(CONFIG_CHIPSLICE)
    {
      int snd_freq = get_param_int("input-eventbroker","snd_frequency",8000);
      int snd_leng = get_param_int("input-eventbroker","snd_length",50);
      int fd = 0;

      /* if no frequency defined, get_param_int return 5000.
       * if no length defined, get_param_int return 300.
       */

      /* open virtual terminal read only */
      fd = open("/dev/tty2",O_RDONLY);

      if(fd < 1) {
	printf("/dev/tty2: open error\n");
	return;
      }
      ioctl(fd,snd_type,(snd_freq + (snd_leng << 16)));
      close(fd);
    }
# endif /* defined(CONFIG_XCOPILOT) || defined(CONFIG_SOFT_CHIPSLICE) */
#endif /* DRIVER_CHIPSLICE_SND */

  default:
    break;
  }
  return;
}



/******************************************** Driver registration */
g_error eventbroker_regfunc(struct inlib *i) {
  i->init = &eventbroker_init;
  i->fd_activate = &eventbroker_fd_activate;
  i->fd_init = &eventbroker_fd_init;
  i->close = &eventbroker_close;
  return sucess;
}

#endif /* DRIVER_EVENTBROKER */

/* The End */
