/* $Id: zaurus.c,v 1.1 2002/02/23 07:57:59 micahjd Exp $
 *
 * zaurus.c - Input driver for the Sharp Zaurus SL-5000. This includes a
 *            simple touchscreen driver, and some extras to handle sound
 *            and flash driver messages. The touchscreen and messages could
 *            be put in two separate drivers, but they're both very simple
 *            and Zaurus-dependant, so why not combine them...
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/touchscreen.h>
#include <stdio.h>

/* These headers are from the Zaurus' kernel source */
#include <asm/sharp_ts.h>
#include <asm/sharp_char.h>

int zaurus_ts_fd;
int zaurus_buz_fd;
int zaurus_led_fd;

/******************************************** Implementations */

g_error zaurus_init(void) {
   g_error e;

   e=touchscreen_init();
   errorcheck;
   zaurus_ts_fd = open("/dev/sharp_ts",O_NONBLOCK);
   if (zaurus_ts_fd <= 0)
     return mkerror(PG_ERRT_IO, 74);
   
   /* Open some auxiliary devices... not really important if they fail */
   zaurus_buz_fd = open("/dev/sharp_buz",O_WRONLY);
   zaurus_led_fd = open("/dev/sharp_led",O_WRONLY);

   return success;
}

void zaurus_close(void) {
   close(zaurus_ts_fd);
   close(zaurus_buz_fd);
   close(zaurus_led_fd);
}
   
void zaurus_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
   if ((*n)<(zaurus_ts_fd+1))
     *n = zaurus_ts_fd+1;
   FD_SET(zaurus_ts_fd,readfds);
}

int zaurus_ts_fd_activate(int fd) {
   tsEvent ts;
   static u8 state = 0;
   int trigger, x, y;
   
   /* Read raw data from the driver */
   if (fd!=zaurus_ts_fd)
     return 0;
   if (read(zaurus_ts_fd,&ts,sizeof(ts)) < sizeof(ts))
     return 1;

   //   printf("touchscreen: x=%d y=%d pressure=%d\n", ts.x, ts.y, ts.pressure);
   
   /* Convert to screen coordinates */
   x = ts.x;
   y = ts.y;
   touchscreen_pentoscreen(&x, &y);

   /* What type of pointer event?
    *
    * NOTE: As far as i can tell, the pressure value isn't very useful. It's zero when
    * there's no contact, and somewhere around 550 when there is, without much actual variation
    * depending on pressure. So here we just treat it as a boolean.
    */
   if (ts.pressure) {
      if (state)
	trigger = TRIGGER_MOVE;
      else
	trigger = TRIGGER_DOWN;
   }
   else {
      if (state)
	trigger = TRIGGER_UP;
      else
	return 1;
   }
   
   /* If we got this far, accept the new state and send the event */
   state = (trigger != TRIGGER_UP);
   dispatch_pointing(trigger,x,y,state);
   
   return 1;
}

void zaurus_message(u32 message, u32 param, u32 *ret) {
  switch (message) {
  
  case PGDM_SOUNDFX:
    ioctl(zaurus_buz_fd, SHARP_BUZZER_MAKESOUND,param);
    return;

    switch (param) {

    case PG_SND_KEYCLICK:
      ioctl(zaurus_buz_fd, SHARP_BUZZER_MAKESOUND, SHARP_BUZ_KEYSOUND);
      break;

    case PG_SND_ALARM:
      ioctl(zaurus_buz_fd, SHARP_BUZZER_MAKESOUND, SHARP_BUZ_SCHEDULE_ALARM);
      break;

      /*
    case PG_SND_VISUALBELL: {
      struct sharp_led_status s = { SHARP_LED_MAIL_EXISTS, LED_MAIL_NEWMAIL_EXISTS };
      ioctl(zaurus_led_fd, SHARP_LED_SETSTATUS, &s);
    }
      break;
      */

    }
    break;

  default:
    touchscreen_message(message,param,ret);
  }
}
   
/******************************************** Driver registration */

g_error zaurus_regfunc(struct inlib *i) {
  i->init = &zaurus_init;
  i->close = &zaurus_close;
  i->message = &zaurus_message;
  i->fd_activate = &zaurus_ts_fd_activate;
  i->fd_init = &zaurus_fd_init;
  return success;
}

/* The End */