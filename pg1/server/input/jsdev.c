/* $Id$
 *
 * jsdev.c - Driver for cursor control and key input using a joystick,
 *           reads from the linux joystick device. Note that all joystick 
 *           mapping is specified in a config file, so read README.configfile
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
 */
#include <pgserver/common.h>
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/configfile.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

int jsdev_fd;

/* Types of joystick axis mappings */

#define AXIS_IGNORE    0   /* Do nothing with the axis
			    */
#define AXIS_POINTER   1   /* Axis has a center position, and moves the mouse
			    * pointer when it is pushed away from center.
			    * Params: pointer_axis (x,y)
			    *         deadzone
			    *         multiplier
			    *         divisor
			    */
#define AXIS_KEYS      2   /* Axis represents up to two keys that are pressed
			    * when the axis enters a given range.
			    * Params: key0
			    *         min0
			    *         max0
			    *         key1
			    *         min1
			    *         max1
			    */

/* Types of joystick button mappings */

#define BUTTON_IGNORE  0   /* Do nothing with the button
			    */
#define BUTTON_KEY     1   /* Button represents one keyboard key that is pressed
			    * when the key is.
			    * Params: key
			    */
#define BUTTON_CLICK   2   /* Button represents a mouse button
			    * Params: mousebutton
			    */

/* Axes for the pointer_axis */
#define AXIS_X 0
#define AXIS_Y 1

/* Number of keys AXIS_KEYS supports */
#define NUM_AXIS_KEYS 2

struct axis_mapping {
  int type;
  int pointer_axis;
  int deadzone,multiplier,divisor;
  struct axis_key_mapping {
    int key, min, max, state;
  } k[NUM_AXIS_KEYS];
} *jsdev_axes;

struct button_mapping {
  int type;
  int key;
  int mousebutton;
} *jsdev_buttons;

/* Velocity and button status of the mouse cursor */
int jsdev_cursorvx, jsdev_cursorvy, jsdev_button;

/* Time unit for velocity, in microseconds */
long jsdev_polltime;

struct cursor *js_cursor;

int jsdev_fd_activate(int fd) {
  struct js_event js;
  int i;

  if (fd != jsdev_fd)
    return 0;
  read(jsdev_fd, &js, sizeof(js));

  /* Now we have an event. It could be a changed
   * axis value or a button press/release
   */
  switch (js.type & ~JS_EVENT_INIT) {

  case JS_EVENT_BUTTON:
    switch (jsdev_buttons[js.number].type) {

    case BUTTON_KEY:
      if (js.value) {
	/* press */
	if (jsdev_buttons[js.number].key < 128)
	  infilter_send_key(PG_TRIGGER_CHAR, jsdev_buttons[js.number].key, 0);
	infilter_send_key(PG_TRIGGER_KEYDOWN, jsdev_buttons[js.number].key, 0);
      }
      else {
	/* Release */
	infilter_send_key(PG_TRIGGER_KEYUP, jsdev_buttons[js.number].key, 0);
      }
       break;

    case BUTTON_CLICK:
      if (js.value)
	jsdev_button |= jsdev_buttons[js.number].mousebutton;
      else
	jsdev_button &= ~jsdev_buttons[js.number].mousebutton;
      break;

    }
    break;
    
  case JS_EVENT_AXIS:
    switch (jsdev_axes[js.number].type) {

      /* Map a range of values in the axis or two to buttons */
    case AXIS_KEYS:
      for (i=0;i<NUM_AXIS_KEYS;i++) {
	struct axis_key_mapping *map = &jsdev_axes[js.number].k[i];
	int newstate = js.value >= map->min && js.value <= map->max;
       
	if (newstate && !map->state) {
	  /* press */
	  if (map->key < 128)
	    infilter_send_key(PG_TRIGGER_CHAR, map->key, 0);
	  infilter_send_key(PG_TRIGGER_KEYDOWN, map->key, 0);
	}
	else if (map->state && !newstate) {
	  /* Release */
	  infilter_send_key(PG_TRIGGER_KEYUP, map->key, 0);
	}
	map->state = newstate;
      }
      break;

      /* The axis moves the mouse pointer */
    case AXIS_POINTER:
      /* Ignore it if it's in the dead zone */
      if (js.value <=  jsdev_axes[js.number].deadzone &&
	  js.value >= -jsdev_axes[js.number].deadzone) {
	i = 0;
      }
      else {
	/* Get the delta value for one of the mouse pointer axes */	
	i = js.value * jsdev_axes[js.number].multiplier / jsdev_axes[js.number].divisor;
      }	

      switch (jsdev_axes[js.number].pointer_axis) {
      case AXIS_X:
	jsdev_cursorvx = i;
	break;
      case AXIS_Y:
	jsdev_cursorvy = i;
	break;
      }
      break;

    }
  }
  return 1;
}

g_error jsdev_init(void) {
  const char *jsdev;
  unsigned char axes = 0, buttons = 0;
  g_error e;
  int i,j;
  char buf[64];      /* For constructing config variable names */
  const char *value;
  
  /* If the device wasn't specified in the config file, try /dev/input/js0
   * and if that doesn't work try /dev/js0.
   */
  jsdev = get_param_str("input-jsdev","device",NULL);
  if (jsdev) {
    jsdev_fd = open(jsdev,O_RDONLY);
  }
  else {
    jsdev_fd = open("/dev/input/js0",O_RDONLY);
    if (jsdev_fd < 0)
      jsdev_fd = open("/dev/js0",O_RDONLY);
  }

  if(jsdev_fd < 0)
    return mkerror(PG_ERRT_IO,40);   /* Error opening joystick device */

  /* Determine how many axes and buttons there are, allocate all the structures */
  ioctl(jsdev_fd, JSIOCGAXES, &axes);
  ioctl(jsdev_fd, JSIOCGBUTTONS, &buttons);
  if (axes) {
    e = g_malloc((void**)&jsdev_axes, sizeof(struct axis_mapping) * axes);
    errorcheck;
  }
  else 
    jsdev_axes = NULL;
  if (buttons) {
    e = g_malloc((void**)&jsdev_buttons, sizeof(struct button_mapping) * buttons);
    errorcheck;
  }
  else 
    jsdev_buttons = NULL;

  /* Now read everything in from the configuration variables 
   */

  jsdev_polltime = get_param_int("input-jsdev","polltime",300);

  buf[sizeof(buf)-1]=0;
  for (i=0;i<axes;i++) {

    snprintf(buf, sizeof(buf)-1, "axis%d-type", i);
    value = get_param_str("input-jsdev",buf,"ignore");
    if (!strcmp(value,"pointer"))
      jsdev_axes[i].type = AXIS_POINTER;
    else if (!strcmp(value,"keys"))
      jsdev_axes[i].type = AXIS_KEYS;
    else
      jsdev_axes[i].type = AXIS_IGNORE;

    snprintf(buf, sizeof(buf)-1, "axis%d-pointer-axis", i);
    value = get_param_str("input-jsdev",buf,"");
    if (!strcmp(value,"y"))
      jsdev_axes[i].pointer_axis = AXIS_Y;
    else
      jsdev_axes[i].pointer_axis = AXIS_X;

    snprintf(buf, sizeof(buf)-1, "axis%d-deadzone", i);
    jsdev_axes[i].deadzone = get_param_int("input-jsdev",buf,0);

    snprintf(buf, sizeof(buf)-1, "axis%d-multiplier", i);
    jsdev_axes[i].multiplier = get_param_int("input-jsdev",buf,1);

    snprintf(buf, sizeof(buf)-1, "axis%d-divisor", i);
    jsdev_axes[i].divisor = get_param_int("input-jsdev",buf,5000);

    for (j=0;j<NUM_AXIS_KEYS;j++) {
      snprintf(buf, sizeof(buf)-1, "axis%d-key%d", i,j);
      jsdev_axes[i].k[j].key = get_param_int("input-jsdev",buf,0);
      
      snprintf(buf, sizeof(buf)-1, "axis%d-min%d", i,j);
      jsdev_axes[i].k[j].min = get_param_int("input-jsdev",buf,1);
      
      snprintf(buf, sizeof(buf)-1, "axis%d-max%d", i,j);
      jsdev_axes[i].k[j].max = get_param_int("input-jsdev",buf,0);
    }
  }
  for (i=0;i<buttons;i++) {

    snprintf(buf, sizeof(buf)-1, "button%d-type", i);
    value = get_param_str("input-jsdev",buf,"ignore");
    if (!strcmp(value,"key"))
      jsdev_buttons[i].type = BUTTON_KEY;
    if (!strcmp(value,"click"))
      jsdev_buttons[i].type = BUTTON_CLICK;
    else
      jsdev_buttons[i].type = BUTTON_IGNORE;

    snprintf(buf, sizeof(buf)-1, "button%d-key", i);
    jsdev_buttons[i].key = get_param_int("input-jsdev",buf,0);

    snprintf(buf, sizeof(buf)-1, "button%d-mousebutton", i);
    jsdev_buttons[i].mousebutton = get_param_int("input-jsdev",buf,1);
  }    

  return cursor_new(&js_cursor,NULL,-1);
}

void jsdev_fd_init(int *n,fd_set *readfds,struct
		     timeval *timeout) {
   if ((*n)<(jsdev_fd+1))
     *n = jsdev_fd+1;
   if (jsdev_fd>0)
     FD_SET(jsdev_fd,readfds);

   /* If the mouse should be moved, set the timeout */
   if (jsdev_cursorvx || jsdev_cursorvy) {
     timeout->tv_sec = 0;
     if (timeout->tv_usec > jsdev_polltime)
       timeout->tv_usec = jsdev_polltime;
   }
}

void jsdev_close(void){
  if (jsdev_buttons)
    g_free(jsdev_buttons);
  if (jsdev_axes)
    g_free(jsdev_axes);
  close(jsdev_fd);
  pointer_free(-1,js_cursor);
}

void jsdev_poll(void) {
  infilter_send_pointing(PG_TRIGGER_PNTR_RELATIVE,jsdev_cursorvx,jsdev_cursorvy,jsdev_button,js_cursor);
}

g_error jsdev_regfunc(struct inlib *i) {
  i->init = &jsdev_init;
  i->fd_activate = &jsdev_fd_activate;
  i->fd_init = &jsdev_fd_init;
  i->close = &jsdev_close;
  i->poll = &jsdev_poll;
 return success;
}

/* The End */
