/* $Id: ez328_chipslice_lcd.c,v 1.1 2000/10/12 16:35:45 pney Exp $
 *
 * seikolcd_1.c - video driver for the seiko LCD low resolution.
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

#ifdef DRIVER_EZ328_CHIPSLICE

#include <pgserver/video.h>
#include <pgserver/input.h>


/******************************************** Utils */

/* Assimilates the given area into the update rectangle */
void sdl_addarea(int x,int y,int w,int h) {
}

/******************************************** Implementations */

g_error sdl_init(int xres,int yres,int bpp,unsigned long flags) {
  return 0;
}

void sdl_close(void) {
}

void sdl_pixel(int x,int y,hwrcolor c) {
}

hwrcolor sdl_getpixel(int x,int y) {
  return 0;
}

void sdl_update(void) {
}

void sdl_blit(struct stdbitmap *src,int src_x,int src_y,
		 struct stdbitmap *dest,int dest_x,int dest_y,
		 int w,int h,int lgop) {
}

void sdl_clip_set(int x1,int y1,int x2,int y2) {
}

void sdl_rect(int x,int y,int w,int h,hwrcolor c) {
}

hwrcolor sdl_color_pgtohwr(pgcolor c) {
  return 0;
}

pgcolor sdl_color_hwrtopg(hwrcolor c) {
  return 0;
}


/* Eek!  See the def_gradient for more comments.  This one is just
   reworked for the SDL driver
*/
void sdl_gradient(int x,int y,int w,int h,int angle,
                  pgcolor c1,pgcolor c2,int translucent) {
}

/******************************************** Driver registration */

g_error sdl_regfunc(struct vidlib *v) {
  return sucess;
}

#endif /* DRIVER_EZ328_CHIPSLICE */
/* The End */
