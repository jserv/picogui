/* $Id: input.h,v 1.19 2001/11/01 18:32:44 epchristi Exp $
 *
 * input.h - Abstract input driver interface
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

#ifndef __H_INPUT
#define __H_INPUT

#if defined(__WIN32__) || defined(WIN32)
#ifndef WINDOWS
#define WINDOWS
#endif
#include <winsock2.h>
#define EAGAIN WSAEWOULDBLOCK
#define ioctl ioctlsocket
#else
#include <sys/time.h>    /* For timeval */
#include <sys/types.h>   /* For fd_set */
#endif

#include <pgserver/g_error.h>

/* Just like the video lib, use a structure of pointers
   to functions. These aren't called by much though, and
   defaults don't make as much sense as for video drivers.
   So, unused functions are set to null and simply not
   called. */

struct inlib {
  
  /* Called upon loading the input driver. This inits
     the hardware or underlying library, and optionally
     starts an input thread.
  */
  g_error (*init)(void);

  /* Upon unloading... */
  void (*close)(void);

  /* This method gives the driver control when activity
     on filedescriptors is detected.  fd_init() is called
     prior to entering the select loop, to set the
     necessary bits. After the select loop, if the 
     network code doesn't need the fd it is sent to
     fd_activate(). If the fd is owned by the driver,
     process it and return a 1. Else, return a 0 and
     the fd will go to the next driver. (More efficient
     on average, considering that normally only 1 input
     driver will be loaded)
  */
  /* Look familiar? See select(2). These are already
     initialized, but the input driver gets a chance
     to modify them.
  */
  void (*fd_init)(int *n,fd_set *readfds,struct timeval *timeout);
  int (*fd_activate)(int fd);

  /* This is called after every iteration through the select
     loop no matter what. Note that the default select
     timeout is a few seconds, so if you rely on this for input
     you should define fd_init to lower timeout to something ok
     for the driver
  */
  void (*poll)(void);

  /* If the input device queues events, this should return nonzero
   * when the queue is nonempty. */
  int (*ispending)(void);

  /* For recieving driver messages */
  void (*message)(u32 message, u32 param, u32 *ret);
   
  /* Do not touch (drivers) */
  g_error (*regfunc)(struct inlib *i);  /* For avoiding duplicates */
  struct inlib *next;
};

/* Head of the inlib list. */
extern struct inlib *inlib_list;

/* The main driver - that which the vidlib loads
   and unloads automatically. This can be NULL if the
   vidlib doesn't need an input driver. 
*/
extern struct inlib *inlib_main;

/* Loads an input driver, and puts a pointer to it in
   ppinlib.
*/
g_error load_inlib(g_error (*regfunc)(struct inlib *i),
		   struct inlib **inl);

/* Unload a specific driver */
void unload_inlib(struct inlib *inl);

/* Check whether any drivers have a backlog of events.
 * This calls the driver's ispending function */
int events_pending(void);

/* Unload all drivers */
void cleanup_inlib(void);

/* Registration functions */
g_error sdlinput_regfunc(struct inlib *i);
g_error svgainput_regfunc(struct inlib *i);
g_error ncursesinput_regfunc(struct inlib *i);
g_error chipslicets_regfunc(struct inlib *i);
g_error r3912ts_regfunc(struct inlib *i);
g_error rrdsc21_gpio_regfunc(struct inlib *i);
g_error rrsim_regfunc(struct inlib *i);
g_error rrts_regfunc(struct inlib *i);
g_error vr3ts_regfunc(struct inlib *i);
g_error tuxts_regfunc(struct inlib *i);
g_error ttykb_regfunc(struct inlib *i);
g_error serialmouse_regfunc(struct inlib *i);

/* List of installed input drivers */
struct inputinfo {
  char *name;
  g_error (*regfunc)(struct inlib *i);
};
extern struct inputinfo inputdrivers[];

g_error (*find_inputdriver(const char *name))(struct inlib *i);

#endif /* __H_INPUT */
/* The End */










