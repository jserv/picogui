/* $Id: videotest.c,v 1.1 2001/02/17 05:17:55 micahjd Exp $
 *
 * videotest.c - implements the -s command line switch, running various
 *               tests on the video driver
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

/************ Line test pattern */

void testpat_line(void) {
   hwrcolor bg = (*vid->color_pgtohwr)(0xFFFFFF);
   hwrcolor fg = (*vid->color_pgtohwr)(0x000000);
   struct fontdesc fd;
   int patx,paty,patw;
   int i;
   
   /* Manufacture a simple fontdesc */
   memset(&fd,0,sizeof(fd));
   fd.fs = fontstyles;
   fd.font = fd.fs->normal;
   fd.hline = -1;
   (*vid->font_newdesc)(&fd);
   
   /* Background */
   (*vid->rect)(0,0,vid->xres,vid->yres,bg);

   /* Lines 5 pixels from each edge */
   (*vid->slab)(0,5,vid->xres,fg);
   (*vid->slab)(0,vid->yres-6,vid->xres,fg);
   (*vid->bar)(5,0,vid->yres,fg);
   (*vid->bar)(vid->xres-6,0,vid->yres,fg);
   
   /* More lines lining the edges (to test for off-by-one framebuffer bugs) */
   (*vid->slab)(7,0,vid->xres-14,fg);
   (*vid->slab)(7,vid->yres-1,vid->xres-14,fg);
   (*vid->bar)(0,7,vid->yres-14,fg);
   (*vid->bar)(vid->xres-1,7,vid->yres-14,fg);
   
   /* 4x4 rectangles in each corner, to make sure we can address those
    * extremities with ease */
   (*vid->rect)(0,0,4,4,fg);
   (*vid->rect)(vid->xres-4,0,4,4,fg);
   (*vid->rect)(0,vid->yres-4,4,4,fg);
   (*vid->rect)(vid->xres-4,vid->yres-4,4,4,fg);
   
   /* Horizontal and vertical text labels along the inside of the lines */
   outtext(&fd,7,7,fg,"PicoGUI Video Test Pattern #1",NULL);
   outtext_v(&fd,7,vid->yres-8,fg,"PicoGUI Video Test Pattern #1",NULL);

   /* Center the test pattern bounding box */
   patw = ((vid->xres<vid->yres)?vid->xres:vid->yres) -
     24 - fd.fs->normal->h*2;
   patx = (vid->xres - patw) >> 1;
   paty = (vid->yres - patw) >> 1;
   
   /* Draw little alignment marks, a 1-pixel gap from the test pattern */
   (*vid->slab)(patx-5,paty,4,fg);
   (*vid->slab)(patx-5,paty+patw,4,fg);
   (*vid->slab)(patx+patw+2,paty,4,fg);
   (*vid->slab)(patx+patw+2,paty+patw,4,fg);
   (*vid->bar)(patx,paty-5,4,fg);
   (*vid->bar)(patx+patw,paty-5,4,fg);
   (*vid->bar)(patx,paty+patw+2,4,fg);
   (*vid->bar)(patx+patw,paty+patw+2,4,fg);

   /* Line thingies within the box */
   for (i=0;i<=patw;i+=4) {
      (*vid->line)(patx+i,paty,patx+patw,paty+i,fg);
      (*vid->line)(patx,paty+i,patx+i,paty+patw,fg);
   }
   outtext(&fd,(vid->xres-fd.fs->normal->h)>>1,(vid->yres-fd.fs->normal->h)>>1,
	   fg,"1",NULL);
}

/************ Color test pattern */

void testpat_color(void) {
   hwrcolor bg = (*vid->color_pgtohwr)(0x000000);
   hwrcolor fg = (*vid->color_pgtohwr)(0xFFFFFF);
   struct fontdesc fd;
   int patx,paty,patw;
   int y=0;
   int h;
   
   /* Manufacture a simple fontdesc */
   memset(&fd,0,sizeof(fd));
   fd.fs = fontstyles;
   fd.font = fd.fs->normal;
   fd.hline = -1;
   (*vid->font_newdesc)(&fd);
   h = fd.fs->normal->h;
   
   /* Background */
   (*vid->rect)(0,0,vid->xres,vid->yres,bg);
   
   outtext(&fd,0,y,fg,"Black -> White",NULL);
   y+=h;
   (*vid->gradient)(0,y,vid->xres,h*2,0,0x000000,0xFFFFFF,0);
   y+=2*h;
   
   outtext(&fd,0,y,fg,"Black -> Red",NULL);
   y+=h;
   (*vid->gradient)(0,y,vid->xres,h*2,0,0x000000,0xFF0000,0);
   y+=2*h;
   
   outtext(&fd,0,y,fg,"Black -> Green",NULL);
   y+=h;
   (*vid->gradient)(0,y,vid->xres,h*2,0,0x000000,0x00FF00,0);
   y+=2*h;
   
   outtext(&fd,0,y,fg,"Black -> Blue",NULL);
   y+=h;
   (*vid->gradient)(0,y,vid->xres,h*2,0,0x000000,0x0000FF,0);
   y+=2*h;
   
}

/************ Front-end */

void videotest_help(void) {
   puts("\nVideo test modes:\n"
	"\t1\tLine test pattern\n"
   	"\t2\tColor test pattern\n");
}

void videotest_run(int number) {
   switch (number) {
   
    case 1:
      testpat_line();
      break;
    case 2:
      testpat_color();
      break;
    
    default:
      printf("Unknown video test mode");
      exit(1);
   }
   
   (*vid->update)(0,0,vid->xres,vid->yres);
}

/* The End */









