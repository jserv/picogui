/* $Id: svgainput.c,v 1.1 2000/09/03 21:44:02 micahjd Exp $
 *
 * svgainput.h - input driver for SVGAlib
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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
    return mkerror(ERRT_IO,73);
  vga_setmousesupport(1);

  printf("keyboard: %d, mouse: %d\n",__svgalib_kbd_fd,__svgalib_mouse_fd);

  return sucess;
}
 
void svgainput_close(void) {
  keyboard_close();
}

/******************************************** Driver registration */

g_error svgainput_regfunc(struct inlib *i) {
  i->init = &svgainput_init;
  i->close = &svgainput_close;

  return sucess;
}

/* The End */
