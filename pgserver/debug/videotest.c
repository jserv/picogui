/* $Id: videotest.c,v 1.7 2001/03/19 17:34:18 bauermeister Exp $
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
   hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
   hwrcolor fg = VID(color_pgtohwr) (0x000000);
   struct fontdesc fd;
   int patx,paty,patw;
   int i;
   
   /* Manufacture a simple fontdesc */
   memset(&fd,0,sizeof(fd));
   fd.fs = fontstyles;
   fd.font = fd.fs->normal;
   fd.hline = -1;
   VID(font_newdesc) (&fd);
   
   /* Background */
   VID(rect) (0,0,vid->lxres,vid->lyres,bg);

   /* Lines 5 pixels from each edge */
   VID(slab) (0,5,vid->lxres,fg);
   VID(slab) (0,vid->lyres-6,vid->lxres,fg);
   VID(bar) (5,0,vid->lyres,fg);
   VID(bar) (vid->lxres-6,0,vid->lyres,fg);
   
   /* More lines lining the edges (to test for off-by-one framebuffer bugs) */
   VID(slab) (7,0,vid->lxres-14,fg);
   VID(slab) (7,vid->lyres-1,vid->lxres-14,fg);
   VID(bar) (0,7,vid->lyres-14,fg);
   VID(bar) (vid->lxres-1,7,vid->lyres-14,fg);
   
   /* 4x4 rectangles in each corner, to make sure we can address those
    * extremities with ease */
   VID(rect) (0,0,4,4,fg);
   VID(rect) (vid->lxres-4,0,4,4,fg);
   VID(rect) (0,vid->lyres-4,4,4,fg);
   VID(rect) (vid->lxres-4,vid->lyres-4,4,4,fg);
   
   /* Horizontal and vertical text labels along the inside of the lines */
   outtext(&fd,7,7,fg,"PicoGUI Video Test Pattern #1",NULL);
   outtext_v(&fd,7,vid->lyres-8,fg,"PicoGUI Video Test Pattern #1",NULL);

   /* Center the test pattern bounding box */
   patw = ((vid->lxres<vid->lyres)?vid->lxres:vid->lyres) -
     24 - fd.fs->normal->h*2;
   patx = (vid->lxres - patw) >> 1;
   paty = (vid->lyres - patw) >> 1;
   
   /* Draw little alignment marks, a 1-pixel gap from the test pattern */
   VID(slab) (patx-5,paty,4,fg);
   VID(slab) (patx-5,paty+patw,4,fg);
   VID(slab) (patx+patw+2,paty,4,fg);
   VID(slab) (patx+patw+2,paty+patw,4,fg);
   VID(bar) (patx,paty-5,4,fg);
   VID(bar) (patx+patw,paty-5,4,fg);
   VID(bar) (patx,paty+patw+2,4,fg);
   VID(bar) (patx+patw,paty+patw+2,4,fg);

   /* Line thingies within the box */
   for (i=0;i<=patw;i+=4) {
      VID(line) (patx+i,paty,patx+patw,paty+i,fg);
      VID(line) (patx,paty+i,patx+i,paty+patw,fg);
   }
   outtext(&fd,(vid->lxres-fd.fs->normal->h)>>1,(vid->lyres-fd.fs->normal->h)>>1,
	   fg,"1",NULL);
}

/************ Color test pattern */

void testpat_color(void) {
   hwrcolor bg = VID(color_pgtohwr) (0x000000);
   hwrcolor fg = VID(color_pgtohwr) (0xFFFFFF);
   struct fontdesc fd;
   int patx,paty,patw;
   int y=0;
   int h;
   
   /* Manufacture a simple fontdesc */
   memset(&fd,0,sizeof(fd));
   fd.fs = fontstyles;
   fd.font = fd.fs->normal;
   fd.hline = -1;
   VID(font_newdesc) (&fd);
   h = fd.fs->normal->h;
   
   /* Background */
   VID(rect) (0,0,vid->lxres,vid->lyres,bg);
   
   outtext(&fd,0,y,fg,"Black -> White",NULL);
   y+=h;
   VID(gradient) (0,y,vid->lxres,h*2,0,0x000000,0xFFFFFF,0);
   y+=2*h;

   outtext(&fd,0,y,fg,"White -> Black",NULL);
   y+=h;
   VID(gradient) (0,y,vid->lxres,h*2,0,0xFFFFFF,0x000000,0);
   y+=2*h;

   outtext(&fd,0,y,fg,"Black -> Red",NULL);
   y+=h;
   VID(gradient) (0,y,vid->lxres,h*2,0,0x000000,0xFF0000,0);
   y+=2*h;
   
   outtext(&fd,0,y,fg,"Black -> Green",NULL);
   y+=h;
   VID(gradient) (0,y,vid->lxres,h*2,0,0x000000,0x00FF00,0);
   y+=2*h;
   
   outtext(&fd,0,y,fg,"Black -> Blue",NULL);
   y+=h;
   VID(gradient) (0,y,vid->lxres,h*2,0,0x000000,0x0000FF,0);
   y+=2*h;
   
}

/************ Blit/unblit test pattern */

void testpat_unblit(void) {
   hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
   hwrcolor fg = VID(color_pgtohwr) (0x000000);
   struct fontdesc fd;
   int patx,paty,patw;
   int patxstart;
   int i;
   hwrbitmap bit;
   char buf[20];
   
   /* Manufacture a simple fontdesc */
   memset(&fd,0,sizeof(fd));
   fd.fs = fontstyles;
   fd.font = fd.fs->normal;
   fd.hline = -1;
   VID(font_newdesc) (&fd);
   
   /* Manufacture a simple fontdesc */
   memset(&fd,0,sizeof(fd));
   fd.fs = fontstyles;
   fd.font = fd.fs->normal;
   fd.hline = -1;
   VID(font_newdesc) (&fd);
   
   /* Background */
   VID(rect) (0,0,vid->lxres,vid->lyres,bg);

   /* test pattern bounding box */
   patw = 50;
   patx = patxstart = 10;
   VID(bar) (patw+25,0,vid->lyres,fg);
   
   /* Repeat for different pixel alignments */
   for (paty=10;paty+patw<vid->lyres;paty+=patw+15,patx=++patxstart,patw--) {
      
      /* Draw little alignment marks, a 1-pixel gap from the test pattern */
      VID(slab) (patx-5,paty,4,fg);
      VID(slab) (patx-5,paty+patw,4,fg);
      VID(slab) (patx+patw+2,paty,4,fg);
      VID(slab) (patx+patw+2,paty+patw,4,fg);
      VID(bar) (patx,paty-5,4,fg);
      VID(bar) (patx+patw,paty-5,4,fg);
      VID(bar) (patx,paty+patw+2,4,fg);
      VID(bar) (patx+patw,paty+patw+2,4,fg);
      
      /* Line thingies within the box */
      VID(rect) (patx,paty,patw+1,patw+1,fg);
      VID(rect) (patx+1,paty+1,patw-1,patw-1,bg);
      for (i=0;i<=patw;i+=3)
	VID(line) (patx+i,paty+patw,patx+patw,paty+i,fg);
      sprintf(buf,"%d/%d",patx&7,patw);
      outtext(&fd,patx+2,paty+2,fg,buf,NULL);
      
      /*
      for (i=0;i<=patw;i+=4) {
         VID(line) (patx+i,paty,patx+patw,paty+i,fg);
	 VID(line) (patx,paty+i,patx+i,paty+patw,fg);
      }
      */
      
      /* Blit the bounding box */
      VID(bitmap_new) (&bit,patw+1,patw+1);
      VID(unblit) (patx,paty,bit,0,0,patw+1,patw+1);
      
      
      /* Same pattern, shifted to the side in various alignments */
      for (patx=patw+40,i=0;patx+patw<vid->lxres;patx=((patx+patw+25)&(~7))+(i++)) {
	 
	 VID(slab) (patx-5,paty,4,fg);
	 VID(slab) (patx-5,paty+patw,4,fg);
	 VID(slab) (patx+patw+2,paty,4,fg);
	 VID(slab) (patx+patw+2,paty+patw,4,fg);
	 VID(bar) (patx,paty-5,4,fg);
	 VID(bar) (patx+patw,paty-5,4,fg);
	 VID(bar) (patx,paty+patw+2,4,fg);
	 VID(bar) (patx+patw,paty+patw+2,4,fg);
	 
	 VID(blit) (bit,0,0,patx,paty,patw+1,patw+1,PG_LGOP_NONE);
      }

      VID(bitmap_free) (bit);
   }
}

/************ Slab test pattern */

void testpat_slab(void) {
   hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
   hwrcolor fg = VID(color_pgtohwr) (0x000000);
   int i;

   /* Background */
   VID(rect) (0,0,vid->lxres,vid->lyres,bg);

   for (i=0;i<16;i++) {
      VID(slab) (5+i,5+(i<<1),12,fg);
      VID(slab) (35+i,5+(i<<1),8,fg);
      VID(slab) (65+i,5+(i<<1),5,fg);

      VID(slab) (5,45+(i<<1),i+1,fg);
      VID(slab) (35+i,45+(i<<1),i+1,fg);
   }
}

/************ Front-end */

void videotest_help(void) {
   puts("\nVideo test modes:\n"
	"\t1\tLine test pattern\n"
   	"\t2\tColor test pattern\n"
	"\t3\tBlit/unblit test pattern\n"
	"\t4\tSlab alignment test pattern\n"
	"\t99\tAll tests\n"
	"\tnegative value: repeat test in a loop\n"
	);
}


static void videotest_run_one(int number) {
  switch (number) {
  case 1:
    testpat_line();
    break;
  case 2:
    testpat_color();
    break;
  case 3:
    testpat_unblit();
    break;
  case 4:
    testpat_slab();
    break;
  default:
    printf("Unknown video test mode");
    exit(1);
  }
  VID(update) (0,0,vid->lxres,vid->lyres);
}


void videotest_run(int number) {
  int loop, cycle, nr;
  const int delay = 1;

  loop = number<0;
  number = number<0 ? -number : number;
  cycle = number==99;
  nr = cycle ? 1 : number;

  for(;;) {
    videotest_run_one(nr);

    if(loop || cycle) {
      sleep(delay);
      if(cycle) {
	if(++nr>4) {
	  if(!loop) break;
	  else nr = 1;
	}
      }
    }
    else break;
  }
}


/* The End */
