/* $Id$
 *
 * zaurus.c - Input driver for the Sharp Zaurus SL-5000. This includes a
 *            simple touchscreen driver, and some extras to handle sound
 *            and flash driver messages. The touchscreen and messages could
 *            be put in two separate drivers, but they're both very simple
 *            and Zaurus-dependant, so why not combine them...
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors:
 *
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/os.h>
#include <stdio.h>

/* These headers are from the Zaurus' kernel source */
#include <asm/sharp_ts.h>
#include <asm/sharp_char.h>

int zaurus_ts_fd;
int zaurus_buz_fd;
int zaurus_led_fd;

struct infilter infilter_zaurus;
handle zaurus_if;

/******************************************** Implementations */

g_error zaurus_init(void) {
  g_error e;
  
  zaurus_ts_fd = open("/dev/sharp_ts",O_NONBLOCK);
  if (zaurus_ts_fd <= 0)
    return mkerror(PG_ERRT_IO, 74);
  
  /* Open some auxiliary devices... not really important if they fail */
  zaurus_buz_fd = open("/dev/sharp_buz",O_WRONLY);
  zaurus_led_fd = open("/dev/sharp_led",O_WRONLY);

  /* Insert our own input filter to handle oddities of the Zaurus keyboard */
  e = infilter_insert(&infilter_list, &zaurus_if, -1, &infilter_zaurus);
  errorcheck;
  
  return success;
}

void zaurus_close(void) {
  handle_free(-1,zaurus_if);
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
  
  /* Read raw data from the driver */
  if (fd!=zaurus_ts_fd)
    return 0;
  if (read(zaurus_ts_fd,&ts,sizeof(ts)) < sizeof(ts))
    return 1;
  
  infilter_send_touchscreen(ts.x,ts.y,ts.pressure,ts.pressure != 0);
  
  return 1;
}

void zaurus_message(u32 message, u32 param, u32 *ret) {
  switch (message) {
    
  case PGDM_POWER:
    if (param <= PG_POWER_SLEEP) {
      int fd = open("/proc/sys/pm/suspend",O_WRONLY);
      write(fd,"1\n",2);
      close(fd);
      /* Now it's waking up from sleep */
      inactivity_reset();
    }
    break;
  
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

#if 0   /* I couln't get this to work... maybe try again later -- Micah */
    case PG_SND_VISUALBELL: {
      struct sharp_led_status s1 = { SHARP_LED_PHONE_IN, 1 };
      struct sharp_led_status s2 = { SHARP_LED_MAIL_EXISTS, 1 };
      struct sharp_led_status s3 = { SHARP_LED_SALARM, 1 };
      struct sharp_led_status s4 = { SHARP_LED_DALARM, 1 };
      ioctl(zaurus_led_fd, SHARP_LED_SETSTATUS, &s1);
      ioctl(zaurus_led_fd, SHARP_LED_SETSTATUS, &s2);
      ioctl(zaurus_led_fd, SHARP_LED_SETSTATUS, &s3);
      ioctl(zaurus_led_fd, SHARP_LED_SETSTATUS, &s4);
    }
      break;
#endif

    }
    break;
  }
}

/******************************************** Zaurus input filter */

/* This input filter will handle zaurus-specific keyboard events, like
 * powering off and adjusting the frontlight.
 */
void infilter_zaurus_handler(struct infilter *self, u32 trigger, union trigparam *param) {
  static u32 last_cancel_press = 0;
  u32 now;
  union trigparam newtrig;

  switch (param->kbd.key) {

    /* Detect double-press of the cancel button as suspend */
  case PGKEY_ESCAPE:
    now = os_getticks();
    if (now < last_cancel_press + 250)
      drivermessage(PGDM_POWER, PG_POWER_SLEEP, NULL);
    else
      last_cancel_press = now;
    break;

    /* Seems to be F6 is what happens when you press Fn-C, map that to CTRL-C */
  case PGKEY_F6:
    newtrig = *param;
    newtrig.kbd.key = PGKEY_c;
    newtrig.kbd.mods = PGMOD_CTRL;
    infilter_send(self,PG_TRIGGER_KEY,&newtrig);
    break;

  }
}

struct infilter infilter_zaurus = {
  accept_trigs: PG_TRIGGER_KEYDOWN,
  absorb_trigs: 0,
  handler: &infilter_zaurus_handler,
};

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
