/* $Id$
 *
 * null.c - A dummy driver that produces no actual output but can do
 *          some error checking and debuggative things
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

#include <pgserver/video.h>

#ifdef DEBUG_VIDEO
bool driver_initialized;
#endif

/******************************************** Implementations */

g_error null_init(void) {
#ifdef DEBUG_VIDEO
   driver_initialized = 1;
#endif
   vid->display = NULL;
   if (!vid->xres) vid->xres = 640;
   if (!vid->yres) vid->xres = 480;
   if (!vid->bpp) vid->bpp = 8;
   return success;
}

g_error null_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
   /* Just go along with whatever they want... */
   vid->xres = xres;
   vid->yres = yres;
   vid->bpp  = bpp;
   vid->display = NULL;
   return success;
}

#ifdef DEBUG_VIDEO
void null_close(void) {
   driver_initialized = 0;
}
#endif

void null_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
   if (dest) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }
#ifdef DEBUG_VIDEO
   if (!driver_initialized)
     printf("Null driver: pixel at %d,%d set when uninitialized\n",x,y);
   if (x<0 || y<0 || (x>=vid->xres) || (y>=vid->yres))
     printf("Null driver: pixel out of bounds at %d,%d\n",x,y);
#endif
}

hwrcolor null_getpixel(hwrbitmap src,s16 x,s16 y) {
   if (src)
     return def_getpixel(src,x,y);
#ifdef DEBUG_VIDEO
   if (!driver_initialized)
     printf("Null driver: pixel at %d,%d get when uninitialized\n",x,y);
   if (x<0 || y<0 || (x>=vid->xres) || (y>=vid->yres))
      printf("Null driver: getpixel out of bounds at %d,%d\n",x,y);
#endif
   return 0;
}

/******************************************** Driver registration */

g_error null_regfunc(struct vidlib *v) {
   setvbl_default(v);
   
   v->init = &null_init;
   v->setmode = &null_setmode;
#ifdef DEBUG_VIDEO
   v->close = &null_close;
#endif
   v->pixel = &null_pixel;
   v->getpixel = &null_getpixel;
   return success;
}

/* The End */
