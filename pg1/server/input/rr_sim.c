/* 
 * Copyright (C) 2001 RidgeRun, Inc.
 * Author: RidgeRun, Inc.
 *         Eric Christianson echristi@ridgerun.com or info@ridgerun.com
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
 */

#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>

#include <stdio.h>          

#define TRACE(args...)
//#define TRACE(args...) fprintf(stderr, args)

int rrsim_fd_mouse;
int rrsim_fd_kb;

/* this is a rudimentary keymap. It should be expanded to include modifiers for keys. (ie. alt/shift/ctrl) */
/* I determined the map using xmodmap -pk */


g_error rrsim_init(void) {

  TRACE("initializing\n");

  rrsim_fd_kb = open("/dev/input/rrkeyb0",O_NONBLOCK);
  TRACE("initialized keyboard: %d\n",rrsim_fd_kb);
  if (rrsim_fd_kb <= 0) 
    return mkerror(PG_ERRT_IO, 74); 

  rrsim_fd_mouse = open("/dev/input/rrmouse0",O_NONBLOCK);
  TRACE("initialized mouse: %d\n",rrsim_fd_mouse);
  if (rrsim_fd_mouse <= 0)
    return mkerror(PG_ERRT_IO, 74);

  return success;
}

void rrsim_close(void) {
  close(rrsim_fd_kb);
  close(rrsim_fd_mouse);
}
   
void rrsim_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {

  if(rrsim_fd_kb > 0) {
    if ((*n)<(rrsim_fd_kb+1)) 
      *n = rrsim_fd_kb+1;
    FD_SET(rrsim_fd_kb,readfds); 
  }

  if(rrsim_fd_mouse > 0) {
    if ((*n)<(rrsim_fd_mouse+1)) 
      *n = rrsim_fd_mouse+1;
    FD_SET(rrsim_fd_mouse,readfds); 
  }
}

void rrsim_mouse_handler()
{
  struct mouse_packet 
  {
    int x;
    int y;
    int buttons;
  };

  int n;
  static int was_pressed          = 0;
  static int waiting_for_second_1 = 0;
  static int waiting_for_second_0 = 0;
  u_long flags = 0;
  struct mouse_packet mouse;

  n = read(rrsim_fd_mouse, &mouse, sizeof(mouse));

  if ( n != sizeof(mouse))
    return;

/*    printf("(x=%d,y=%d) mouse.buttons: %d\n", mouse.x, mouse.y, mouse.buttons);   */

  if ( mouse.buttons && !was_pressed)
    {
      if ( waiting_for_second_1 )
	{
	  dispatch_pointing(PG_TRIGGER_UP, mouse.x, mouse.y, 1);
	  waiting_for_second_1 = 0;
	  was_pressed = 1;
	}
      else
        waiting_for_second_1 = 1;
    }
  else if ( was_pressed && !mouse.buttons )
    {
      if ( waiting_for_second_0 )
        {
	  dispatch_pointing(PG_TRIGGER_DOWN, mouse.x, mouse.y, 1);
	  waiting_for_second_0 = 0;
	  was_pressed = 0;
        }
      else
        waiting_for_second_0 = 1;
    }
  else
    dispatch_pointing(PG_TRIGGER_MOVE, mouse.x, mouse.y, 1);
}

#define EVENT_MASK (1 << 31)

void rrsim_keyboard_handler()
{
  unsigned int    buf[256], *b;
  int		         n;
  unsigned int    keycode;
  unsigned int    key_state;
  unsigned int    press;
  int input_mod = 0;

  while ((n = read (rrsim_fd_kb, buf, sizeof (buf))) > 0) {
    b = buf;
    while (n) {
      input_mod = 0;
      keycode   = b[0];
      key_state = b[1] & ~EVENT_MASK; 
      press = !(b[1] & EVENT_MASK);

      // The key state determines whether any modifiers are set
      // such as shift, ctrl, etc.

      /* key_state bits 0=normal 1=shift 2=capslock 4=ctrl 8=alt, use to set input_mod */
      if(key_state) {
	/* set input_mod appropriately with PGMOD masks */
	if(key_state & 0x01) {
	  input_mod |= PGMOD_SHIFT;
	}
	if(key_state & 0x02) {
	  input_mod |= PGMOD_CAPS;
	}
	if(key_state & 0x04) {
	  input_mod |= PGMOD_CTRL;
	}
	if(key_state & 0x08) {
	  input_mod |= PGMOD_ALT;
	}
      }

      
      /* also need to figure out if only ascii chars get PG_TRIGGER_CHAR or if special keys only get
       * PG_TRIGGER_KEYUP */
      if(press && (keycode < 255)) {
	TRACE("PG_TRIGGER_CHAR %d\n",keycode);
	dispatch_key(PG_TRIGGER_CHAR, keycode,input_mod);
      }

      TRACE("keycode=%d key_state=%d press=%s input_mod=%x\n",keycode,key_state,press?"PG_TRIGGER_KEYDOWN":"PG_TRIGGER_KEYUP",input_mod);  

      dispatch_key(press ? PG_TRIGGER_KEYDOWN : PG_TRIGGER_KEYUP,keycode,input_mod);

      b+=2;
      n-=8;
    }
  }


}

int rrsim_fd_activate(int fd) {
  
  if ( fd == rrsim_fd_mouse ) {
    rrsim_mouse_handler();
    return 1;
  } else if ( fd == rrsim_fd_kb) {
    rrsim_keyboard_handler();
    return 1;
  }
  return 0;
}


/******************************************** Driver registration */

g_error rrsim_regfunc(struct inlib *i) {
  i->init = &rrsim_init;
  i->close = &rrsim_close;
  i->fd_activate = &rrsim_fd_activate;
  i->fd_init = &rrsim_fd_init;
  return success;
}

