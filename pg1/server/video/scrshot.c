/* $Id$
 *
 * scrshot.c - Maintains a virtual framebuffer, taking screenshots on update
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
#include <pgserver/configfile.h>
#include <pgserver/init.h>

#include <stdio.h>

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

int scrshot_frame;
const char *scrshot_file;
int scrshot_skip;
int scrshot_total;

void scrshot_close();

g_error scrshot_init(void) {
   /* Avoid freeing a nonexistant backbuffer in close() */
   FB_MEM = NULL;
   
   /* Default mode: 640x480 */
   if (!vid->xres) vid->xres = 640;
   if (!vid->yres) vid->yres = 480;
   if (!vid->bpp)  vid->bpp  = 32;

   scrshot_frame = 0;
   scrshot_file = get_param_str("video-scrshot","file","pgshot%04d.ppm");
   scrshot_skip = get_param_int("video-scrshot","skip",1);
   scrshot_total = get_param_int("video-scrshot","total",1);
   
   return success;
}

g_error scrshot_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
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
      scrshot_close();
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
      
void scrshot_close(void) {
   /* Free backbuffer */
   if (FB_MEM)
     g_free(FB_MEM);
}

void scrshot_update(hwrbitmap d,s16 unusedx, s16 unusedy, s16 w, s16 h) {
  char buf[256];    /* I hate static buffers... */
  FILE *f;
  int x,y;

  if (d!=vid->display)
    return;

  if (scrshot_skip) {
    scrshot_skip--;
    printf("scrshot: Skipping frame (%d remaining)\n", scrshot_skip);
    return;
  }

  snprintf(buf,sizeof(buf)-1,scrshot_file,scrshot_frame++);
  buf[sizeof(buf)-1]=0;
  f = fopen(buf,"wb");
  printf("scrshot: Taking screenshot '%s' (%d remaining)\n",
	 buf,scrshot_total-scrshot_frame);
  fprintf(f,"P6\n%d %d\n255\n",vid->xres,vid->yres);

  for (y=0;y<vid->yres;y++)
    for (x=0;x<vid->xres;x++) {
      pgcolor c;
      
      c = VID(color_hwrtopg)(VID(getpixel)(vid->display,x,y));
      fputc(getred(c),f);
      fputc(getgreen(c),f);
      fputc(getblue(c),f);
    }

  fclose(f);

  if (scrshot_total==scrshot_frame) {
    printf("scrshot: Done with screenshots, exiting\n");
    pgserver_mainloop_stop();
  }
}

g_error scrshot_regfunc(struct vidlib *v) {
  v->init = &scrshot_init;
  v->setmode = &scrshot_setmode; 
  v->close = &scrshot_close;
  v->update = &scrshot_update;
  return success;
}

/* The End */
