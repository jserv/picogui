/* $Id: scrshot.c,v 1.1 2001/07/05 06:17:57 micahjd Exp $
 *
 * scrshot.c - Maintains a virtual framebuffer, taking screenshots on update
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <stdio.h>

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

int scrshot_frame;
const char *scrshot_file;
int scrshot_skip;
int scrshot_total;

g_error scrshot_init(void) {
   /* Avoid freeing a nonexistant backbuffer in close() */
   FB_MEM = NULL;
   
   /* Default mode: 640x480 */
   if (!vid->xres) vid->xres = 640;
   if (!vid->yres) vid->yres = 480;

   scrshot_frame = 0;
   scrshot_file = get_param_int("video-scrshot","file","pgshot%04d.ppm");
   scrshot_skip = get_param_int("video-scrshot","skip",1);
   scrshot_total = get_param_int("video-scrshot","total",1);
   
   return sucess;
}

g_error scrshot_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
   g_error e;
   
   if (FB_MEM) {
      g_free(FB_MEM);
      FB_MEM = NULL;
   }
   
   setvbl_linear24(vid);   
   vid->xres = xres;
   vid->yres = yres;
   vid->bpp = 24;
   
   FB_BPL = vid->xres * 3;
   e = g_malloc((void**)&FB_MEM,(FB_BPL * vid->yres));
   errorcheck;
   
   return sucess; 
}
   
void scrshot_close(void) {
   /* Free backbuffer */
   if (FB_MEM)
     g_free(FB_MEM);
}

void scrshot_update(s16 x, s16 y, s16 w, s16 h) {
  char buf[256];    /* I hate static buffers... */
  FILE *f;

  if (scrshot_skip) {
    scrshot_skip--;
    printf("scrshot: Skipping frame (%d remaining)\n");
    return;
  }

  sprintf(buf,scrshot_file,scrshot_frame++);
  f = fopen(buf,"wb");
  printf("scrshot: Taking screenshot '%s' (%d remaining)\n",
	 buf,scrshot_total-scrshot_frame);
  sprintf(buf,"P6\n%d %d\n255\n",vid->xres,vid->yres);
  fputs(buf,f);
  fwrite(FB_MEM,FB_BPL,vid->yres,f);
  fclose(f);

  if (scrshot_total==scrshot_frame) {
    printf("scrshot: Done with screenshots, exiting\n");
    request_quit();
  }
}

g_error scrshot_regfunc(struct vidlib *v) {
  v->init = &scrshot_init;
  v->setmode = &scrshot_setmode; 
  v->close = &scrshot_close;
  v->update = &scrshot_update;
  return sucess;
}

/* The End */
