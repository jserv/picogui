/* $Id$
 *
 * vncserver.c - Video driver that runs a VNC server and processes
 *               input events for multiple clients, using the
 *               included copy of libvncserver.
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
#include <pgserver/input.h>
#include <pgserver/configfile.h>
#include "libvncserver/rfb.h"

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

rfbScreenInfoPtr vncserver_screeninfo = NULL;      /* VNC server's main structure */
hwrbitmap vncserver_buffer = NULL;                 /* Framebuffer provided to VNC clients */


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
  rfbClientIteratorPtr iterator;
  rfbClientPtr cl;

  iterator = rfbGetClientIterator(vncserver_screeninfo);
  while((cl=rfbClientIteratorNext(iterator))) {
    /* Terminate each client thread */
    pthread_kill(cl->client_thread, 9);
  }

  /* Terminate the listener thread */
  pthread_kill(vncserver_screeninfo->listener_thread, 9);

  unload_inlib(inlib_main);   /* Take out our input driver */
  if (FB_MEM) {
    g_free(FB_MEM);
    vid->bitmap_free(vncserver_buffer);
  } 
}

g_error vncserver_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
   g_error e;
   
   if (vncserver_screeninfo) {
     if (xres == vid->xres && yres == vid->yres)
       return success;
     else {
       /* Error setting video mode
	* (libvncserver can't yet handle reinitializing)
	*/
       return mkerror(PG_ERRT_BADPARAM, 47); 
     }
   }
   
   /* Free the old buffer */
   if (FB_MEM) {
      g_free(FB_MEM);
      vid->bitmap_free(vncserver_buffer);
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
   e = vid->bitmap_new(&vncserver_buffer, vid->xres, vid->yres, vid->bpp);
   errorcheck;
   vncserver_screeninfo->frameBuffer = ((struct stdbitmap*) vncserver_buffer)->bits;

   /* Process configuration variables */
   vncserver_screeninfo->rfbPort            = get_param_int("video-vncserver", "port", 5900);
   vncserver_screeninfo->rfbMaxClientWait   = get_param_int("video-vncserver", "wait", 20000);
   vncserver_screeninfo->rfbAuthPasswdData  = (char*) get_param_str("video-vncserver", "password-file", NULL);
   vncserver_screeninfo->rfbDeferUpdateTime = get_param_int("video-vncserver", "defer-update", 1);
   vncserver_screeninfo->desktopName        = get_param_str("video-vncserver", "name", "PicoGUI VNC Server");
   vncserver_screeninfo->rfbAlwaysShared    = get_param_int("video-vncserver", "always-shared", 0);
   vncserver_screeninfo->rfbNeverShared     = get_param_int("video-vncserver", "never-shared", 0);
   vncserver_screeninfo->rfbDontDisconnect  = get_param_int("video-vncserver", "dont-disconnect", 0);
   rfbLogEnable(get_param_int("video-vncserver", "verbose", 0));

   /* Start the VNC server's thread */
   rfbInitServer(vncserver_screeninfo);
   rfbRunEventLoop(vncserver_screeninfo, -1, TRUE);

   /* Load the input driver */
   return load_inlib(&vncinput_regfunc,&inlib_main);
}
 
void vncserver_update(hwrbitmap d,s16 x,s16 y,s16 w,s16 h) {
  vid->blit(vncserver_buffer, x,y,w,h, d, x,y, PG_LGOP_NONE);
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
