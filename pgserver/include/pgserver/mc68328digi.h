/*----------------------------------------------------------------------------*/
/* $Id: mc68328digi.h,v 1.2 2001/03/07 18:34:57 pney Exp $
 *
 * linux/drivers/char/mc68328digi.h - Touch screen driver.
 *                                    Header file.
 *
 * Author: Philippe Ney <philippe.ney@smartdata.ch>
 * Copyright (C) 2001 SMARTDATA <www.smartdata.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Thanks to:
 *    Kenneth Albanowski for is first work on a touch screen driver.
 *    Alessandro Rubini for is "Linux device drivers" book.
 *    Ori Pomerantz for is "Linux Kernel Module Programming" guide.
 *
 *
 * Event generated:
 *                __________      ______________
 *   1) /PENIRQ            |     |
 *                         -------
 *
 *                         |     |
 *                         |     +-> generate PEN_UP
 *                         |
 *                         +-------> generate PEN_DOWN
 *
 *                __________               ______________
 *   2) /PENIRQ            |              |
 *                         ------#####-----
 *
 *                         |     |||||    |
 *                         |     |||||    +-> generate PEN_UP
 *                         |     |||||
 *                         |     ||||+------> generate PEN_MOVE
 *                         |     |||+-------> generate PEN_MOVE
 *                         |     ||+--------> generate PEN_MOVE
 *                         |     |+---------> generate PEN_MOVE
 *                         |     +----------> generate PEN_MOVE
 *                         |
 *                         -----------------> generate PEN_DOWN
 *
 */
/*----------------------------------------------------------------------------*/

#ifndef _MC68328DIGI_H
#define _MC68328DIGI_H

#include <linux/time.h>   /* for timeval struct */
#include <linux/ioctl.h>  /* for the _IOR macro to define the ioctl commands  */

/*----------------------------------------------------------------------------*/

/* Pen events */
#define EV_PEN_DOWN    0
#define EV_PEN_UP      1
#define EV_PEN_MOVE    2

/* Pen states */
/* defined through the 2 lsb of an integer variable. If an error occure,
 * the driver will recover the state PEN_UP and the error bit will be set.
 */
#define ST_PEN_DOWN    (0<<0)   /* bit 0 at 0 = the pen is down            */
#define ST_PEN_UP      (1<<0)   /* bit 0 at 1 = the pen is up              */
#define ST_PEN_ERROR   (1<<1)   /* bit 1 at 1 means that an error occure   */

/* Definition for the ioctl of the driver */
/* Device is type misc then major=10 */
#define MC68328DIGI_MAJOR  10

#define TS_PARAMS_GET _IOR(MC68328DIGI_MAJOR, 0, struct ts_drv_params)
#define TS_PARAMS_SET _IOR(MC68328DIGI_MAJOR, 1, struct ts_drv_params)
 
/*----------------------------------------------------------------------------*/

/* Available info from pen position and status */
struct ts_pen_info {
  int x,y;    /* pen position                                      */
  int dx,dy;  /* delta move from last position                     */
  int event;  /* event from pen (DOWN,UP,CLICK,MOVE)               */
  int state;  /* state of pen (DOWN,UP,ERROR)                      */
  int ev_no;  /* no of the event                                   */
  unsigned long ev_time;  /* time of the event (ms) since ts_open  */
};

/* Structure that define touch screen parameters */
struct ts_drv_params {
  int x_ratio_num; /*                        */
  int x_ratio_den; /*                        */
  int y_ratio_num; /*                        */
  int y_ratio_den; /*                        */
  int x_offset;    /*                        */
  int y_offset;    /*                        */
  int xy_invert;   /*                        */
  int x_min;       /*                        */
  int y_min;       /*                        */
  int x_max;       /*                        */
  int y_max;       /*                        */
  int mv_thrs;     /* minimum delta move to considere the pen start to move */
  int follow_thrs; /* minimum delta move to follow the pen move when the pen
                    * is moving
		    */
  int deglitch_ms; /* Deglitch time for pen irq (ms) */
  int event_queue_on; /* Switch on and off the event queue */
};


#endif /* _MC68328DIGI_H */
