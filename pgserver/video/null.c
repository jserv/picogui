/* $Id: null.c,v 1.3 2001/02/14 05:13:19 micahjd Exp $
 *
 * null.c - A dummy driver that produces no actual output but can do
 *          some error checking and debuggative things
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

#include <pgserver/common.h>

#ifdef DRIVER_NULL

#include <pgserver/video.h>

#ifdef DEBUG_VIDEO
int driver_initialized;
#endif

/******************************************** Implementations */

g_error null_init(int xres,int yres,int bpp,unsigned long flags) {
   /* Just go along with whatever they want... */
   vid->xres = xres;
   vid->yres = yres;
   vid->bpp  = bpp;
   
#ifdef DEBUG_VIDEO
   driver_initialized = 1;
#endif
   
   return sucess;
}

#ifdef DEBUG_VIDEO
void null_close(void) {
   driver_initialized = 0;
}
#endif

void null_pixel(int x,int y,hwrcolor c) {
#ifdef DEBUG_VIDEO
   if (!driver_initialized)
     printf("Null driver: pixel set when uninitialized\n",x,y);
   if (x<0 || y<0 || (x>=vid->xres) || (y>=vid->yres))
     printf("Null driver: pixel out of bounds at %d,%d\n",x,y);
#endif
}

hwrcolor null_getpixel(int x,int y) {
#ifdef DEBUG_VIDEO
   if (!driver_initialized)
     printf("Null driver: pixel get when uninitialized\n",x,y);
   if (x<0 || y<0 || (x>=vid->xres) || (y>=vid->yres))
      printf("Null driver: getpixel out of bounds at %d,%d\n",x,y);
#endif
   return 0;
}

/******************************************** Driver registration */

g_error null_regfunc(struct vidlib *v) {
   setvbl_default(v);
   
   v->init = &null_init;
#ifdef DEBUG_VIDEO
   v->close = &null_close;
#endif
   v->pixel = &null_pixel;
   v->getpixel = &null_getpixel;
   return sucess;
}

#endif /* DRIVER_NULL */
/* The End */
