/* $Id$
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * Touchscreen driver for PicoGUI on Compaq iPAQ h3600 
 * Copyright (C) 2002 Arnd Bergmann <arnd@bergmann-dalldorf.de>
 *
 * Based on serial mouse driver 
 * Copyright (C) Yunus Altunbicak <yunus.altunbicak@eliar.com.tr>
 *
 * This driver uses the calibrated /dev/h3600_ts input device, so 
 * there is no need to use PicoGUI calibration code, but it could be
 * changed to use the new /dev/h3600_tsraw input if it turns out that
 * the calibration done in the kernel is not good enough.
 *
 * Note that the linux/h3600_ts.h header file is not distributed with
 * standard Linux 2.4 and older kernels.
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/configfile.h>

#include <linux/h3600_ts.h>

static int h3600ts_fd;
int btnstate;

#ifndef __H3600_TS_H__
#warning You need to have the iPAQ kernel from www.handhelds.org to compile h3600ts
/* this is defined here in order to be able to compile the driver anyway... */
typedef struct h3600_ts_event {
        unsigned short pressure;
        unsigned short x;
        unsigned short y;
        unsigned short pad;
} TS_EVENT;
#endif

int h3600ts_fd_activate(int fd) {

  TS_EVENT ev;
  static unsigned short int x,y;
  
  if (fd != h3600ts_fd)
    return 0;

  if (sizeof (ev) != read (fd, &ev, sizeof (ev)))
    return 0;

  if(ev.pressure) {
      x = ev.x; y = ev.y;
  } 

  infilter_send_pointing(PG_TRIGGER_PNTR_STATUS, x, y, ev.pressure, NULL);

   return 1;
}

g_error h3600ts_init(void) {

  btnstate=0;

  h3600ts_fd = open(get_param_str("input-h3600ts","device","/dev/h3600_ts"),
	    O_RDONLY | O_NOCTTY | O_NDELAY);

  if(h3600ts_fd < 0)
    return mkerror(PG_ERRT_IO,43);   /* Error opening touchscreen device */

  return success;
}

void h3600ts_fd_init(int *n,fd_set *readfds,struct
		     timeval *timeout) {
   if ((*n)<(h3600ts_fd+1))
     *n = h3600ts_fd+1;
   if (h3600ts_fd>0)
     FD_SET(h3600ts_fd,readfds);
}

void h3600ts_close(void){
  close(h3600ts_fd);
}

g_error h3600ts_regfunc(struct inlib *i) {
  i->init = &h3600ts_init;
  i->fd_activate = &h3600ts_fd_activate;
  i->fd_init = &h3600ts_fd_init;
  i->message = 0;
  i->close = &h3600ts_close;
  return success;
}
