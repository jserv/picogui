/* $Id: null.c,v 1.1 2001/01/16 02:07:19 micahjd Exp $
 *
 * null.c - A dummy driver that produces no actual output
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

#ifdef DRIVER_NULL

#include <pgserver/video.h>
        
/******************************************** Implementations */

g_error null_init(int xres,int yres,int bpp,unsigned long flags) {
   /* Just go along with whatever they want... */
   vid->xres = xres;
   vid->yres = yres;
   vid->bpp  = bpp;

   return sucess;
}

void null_pixel(int x,int y,hwrcolor c) {}

hwrcolor null_getpixel(int x,int y) {
   return 0;
}

/******************************************** Driver registration */

g_error null_regfunc(struct vidlib *v) {
   setvbl_default(v);
   
   v->init = &null_init;
   v->pixel = &null_pixel;
   v->getpixel = &null_getpixel;
   return sucess;
}

#endif /* DRIVER_NULL */
/* The End */
