/* $Id$
 *
 * nullfb.c - For testing and profiling. Uses linear* VBLs to generate a frame
 *            but does not blit the buffer to any actual hardware
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

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

g_error nullfb_init(void) {
   /* Avoid freeing a nonexistant backbuffer in close() */
   FB_MEM = NULL;
   
   /* Default mode: 640x480 */
   if (!vid->xres) vid->xres = 640;
   if (!vid->yres) vid->yres = 480;
   if (!vid->bpp)  vid->bpp  = 32;
   
   return success;
}

void nullfb_close(void) {
   /* Free backbuffer */
   if (FB_MEM)
     g_free(FB_MEM);
}

g_error nullfb_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
   g_error e;
   
   /* Make screen divisible by a byte */
   if (bpp<8)
     xres &= ~((8/bpp)-1);

   /* Free the old buffer */
   if (FB_MEM) {
      g_free(FB_MEM);
      FB_MEM = NULL;
   }
   
   /* Load a VBL */
   switch (bpp) {
      
#ifdef CONFIG_VBL_LINEAR1
    case 1:
      setvbl_linear1(vid);
      break;
#endif
      
#ifdef CONFIG_VBL_LINEAR2
    case 2:
      setvbl_linear2(vid);
      break;
#endif
      
#ifdef CONFIG_VBL_LINEAR4
    case 4:
      setvbl_linear4(vid);
      break;
#endif
      
#ifdef CONFIG_VBL_LINEAR8
    case 8:
      setvbl_linear8(vid);
      break;
#endif
      
#ifdef CONFIG_VBL_LINEAR16
    case 16:
     setvbl_linear16(vid);
     break;
#endif

#ifdef CONFIG_VBL_LINEAR24
    case 24:
      setvbl_linear24(vid);
      break;
#endif
      
#ifdef CONFIG_VBL_LINEAR32
    case 32:
      setvbl_linear32(vid);
      break;
#endif
      
    default:
      nullfb_close();
      return mkerror(PG_ERRT_BADPARAM,101);   /* Unknown bpp */
   }
   
   vid->xres = xres;
   vid->yres = yres;
   vid->bpp = bpp;
   
   FB_BPL = (vid->xres * bpp) >> 3;
   /* The +1 allows blits some margin to read unnecessary bytes.
    * Keeps linear1's blit from triggering electric fence when the
    * cursor is put in the bottom-right corner ;-)
    */
   e = g_malloc((void**)&FB_MEM,(FB_BPL * vid->yres) + 1);
   errorcheck;
   
   return success; 
}
   
g_error nullfb_regfunc(struct vidlib *v) {
  v->init = &nullfb_init;
  v->setmode = &nullfb_setmode; 
  v->close = &nullfb_close;
  return success;
}

/* The End */
