/* $Id: vncserver.c,v 1.2 2003/01/19 07:36:08 micahjd Exp $
 *
 * vncserver.c - Video driver that runs a VNC server and processes
 *               input events for multiple clients, using the
 *               included copy of libvncserver.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/video.h>
#include "libvncserver/rfb.h"

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

rfbScreenInfoPtr vncserver_screeninfo = NULL;


g_error vncserver_init(void) {
   /* Avoid freeing a nonexistant framebuffer in close() */
   FB_MEM = NULL;
   
   /* Default mode: 640x480 */
   if (!vid->xres) vid->xres = 640;
   if (!vid->yres) vid->yres = 480;
   if (!vid->bpp)  vid->bpp  = 32;
   
   return success;
}

void vncserver_close(void) {
   if (FB_MEM)
     g_free(FB_MEM);
}

g_error vncserver_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
   g_error e;
   
   if (vncserver_screeninfo) {
     if (xres == vid->xres && yres == vid->yres)
       return success;
     else {
       /* Error setting video mode
	* (we can't stop the event thread yet)
	*/
       return mkerror(PG_ERRT_BADPARAM, 47); 
     }
   }
   
   /* Free the old buffer */
   if (FB_MEM) {
      g_free(FB_MEM);
      FB_MEM = NULL;
   }

   /* Set up 32bpp mode */
   setvbl_linear32(vid);
   vncserver_screeninfo = rfbGetScreen(xres,yres,8,3,4);
   vid->xres = xres;
   vid->yres = yres;
   vid->bpp = 32;
   
   /* Allocate framebuffer */
   FB_BPL = (vid->xres * bpp) >> 3;
   e = g_malloc((void**)&FB_MEM,(FB_BPL * vid->yres));
   errorcheck;
   vncserver_screeninfo->frameBuffer = FB_MEM;

   /* Set up the RFB server */
   rfbInitServer(vncserver_screeninfo);
   rfbRunEventLoop(vncserver_screeninfo,-1,TRUE);
   
   return success; 
}
 
void vncserver_update(hwrbitmap d,s16 x,s16 y,s16 w,s16 h) {
  rfbMarkRectAsModified(vncserver_screeninfo, x,y,x+w,y+h);
}

g_error vncserver_regfunc(struct vidlib *v) {
  v->init = &vncserver_init;
  v->setmode = &vncserver_setmode; 
  v->close = &vncserver_close;
  v->update = &vncserver_update;
  return success;
}

/* The End */
