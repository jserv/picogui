/* $Id: svgainput.c,v 1.6 2000/10/10 00:33:37 micahjd Exp $
 *
 * svgainput.h - input driver for SVGAlib
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
 * Contributors:
 * 
 * 
 * 
 */

#ifdef DRIVER_SVGAINPUT

#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>

#include <vga.h>

/* Yes, this is a hack, but it's more efficient this way.
   Note to the authors of SVGAlib: why hide these vars?
*/
extern int __svgalib_mouse_fd;
extern int __svgalib_kbd_fd;

/******************************************** Implementations */

g_error svgainput_init(void) {

  if (keyboard_init()==-1)
    return mkerror(PG_ERRT_IO,73);
  vga_setmousesupport(1);

  return sucess;
}
 
void svgainput_close(void) {
  keyboard_close();
}

void svgainput_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if ((*n) < (__svgalib_mouse_fd+1)) *n = __svgalib_mouse_fd+1;
  if ((*n) < (__svgalib_kbd_fd+1)) *n = __svgalib_kbd_fd+1;
  FD_SET(__svgalib_mouse_fd,readfds);
  FD_SET(__svgalib_kbd_fd,readfds);
}

int svgainput_fd_activate(int fd) {
  if (fd==__svgalib_mouse_fd) {
    mouse_update();
  }
  else if (fd==__svgalib_kbd_fd) {
    keyboard_update();
    dispatch_key(TRIGGER_KEYDOWN,PGKEY_u,PGMOD_CTRL|PGMOD_ALT);
  }
  else 
    return 0;
  return 1;
}

void svgainput_poll(void) {
#ifdef DEBUG
  //  guru("svgainput_poll()");
#endif
}

/******************************************** Driver registration */

g_error svgainput_regfunc(struct inlib *i) {
  //  i->init = &svgainput_init;
  //  i->close = &svgainput_close;
  //  i->fd_init = &svgainput_fd_init;
  //  i->fd_activate = &svgainput_fd_activate;
  i->poll = &svgainput_poll;

  return sucess;
}

#endif /* DRIVER_SVGAINPUT */
/* The End */
