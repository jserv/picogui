/* $Id$
 *
 * videotest.c - implements the -s command line switch, running various
 *               tests on the video driver
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
#include <pgserver/render.h>


/************ Line test pattern */

void testpat_line(void) {
  hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
  hwrcolor fg = VID(color_pgtohwr) (0x000000);
  struct font_descriptor *fd;
  struct font_metrics m;
  int patx,paty,patw;
  int i;
  struct pgquad clip = {0,0,vid->xres-1,vid->yres-1};
   
  rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,res[PGRES_DEFAULT_FONT]);
  fd->lib->getmetrics(fd,&m);
   
  /* Background */
  VID(rect) (VID(window_debug)(),0,0,vid->lxres,vid->lyres,bg,PG_LGOP_NONE);

  /* Lines 5 pixels from each edge */
  VID(slab) (VID(window_debug)(),0,5,vid->lxres,fg,PG_LGOP_NONE);
  VID(slab) (VID(window_debug)(),0,vid->lyres-6,vid->lxres,fg,PG_LGOP_NONE);
  VID(bar) (VID(window_debug)(),5,0,vid->lyres,fg,PG_LGOP_NONE);
  VID(bar) (VID(window_debug)(),vid->lxres-6,0,vid->lyres,fg,PG_LGOP_NONE);
   
  /* More lines lining the edges (to test for off-by-one framebuffer bugs) */
  VID(slab) (VID(window_debug)(),7,0,vid->lxres-14,fg,PG_LGOP_NONE);
  VID(slab) (VID(window_debug)(),7,vid->lyres-1,vid->lxres-14,fg,PG_LGOP_NONE);
  VID(bar) (VID(window_debug)(),0,7,vid->lyres-14,fg,PG_LGOP_NONE);
  VID(bar) (VID(window_debug)(),vid->lxres-1,7,vid->lyres-14,fg,PG_LGOP_NONE);
   
  /* 4x4 rectangles in each corner, to make sure we can address those
   * extremities with ease */
  VID(rect) (VID(window_debug)(),0,0,4,4,fg,PG_LGOP_NONE);
  VID(rect) (VID(window_debug)(),vid->lxres-4,0,4,4,fg,PG_LGOP_NONE);
  VID(rect) (VID(window_debug)(),0,vid->lyres-4,4,4,fg,PG_LGOP_NONE);
  VID(rect) (VID(window_debug)(),vid->lxres-4,vid->lyres-4,4,4,fg,PG_LGOP_NONE);
   
  /* Horizontal and vertical text labels along the inside of the lines */
  fd->lib->draw_string(fd,VID(window_debug)(), xy_to_pair(7,7),
		       fg,pgstring_tmpwrap("PicoGUI Video Test Pattern #1"),&clip,PG_LGOP_NONE,0);
  fd->lib->draw_string(fd,VID(window_debug)(), xy_to_pair(7,vid->lyres-8),
		       fg,pgstring_tmpwrap("PicoGUI Video Test Pattern #1"),&clip,PG_LGOP_NONE,90);

  /* Center the test pattern bounding box */
  patw = ((vid->lxres<vid->lyres)?vid->lxres:vid->lyres) - 40;
  patx = (vid->lxres - patw) >> 1;
  paty = (vid->lyres - patw) >> 1;
   
  /* Draw little alignment marks, a 1-pixel gap from the test pattern */
  VID(slab) (VID(window_debug)(),patx-5,paty,4,fg,PG_LGOP_NONE);
  VID(slab) (VID(window_debug)(),patx-5,paty+patw,4,fg,PG_LGOP_NONE);
  VID(slab) (VID(window_debug)(),patx+patw+2,paty,4,fg,PG_LGOP_NONE);
  VID(slab) (VID(window_debug)(),patx+patw+2,paty+patw,4,fg,PG_LGOP_NONE);
  VID(bar) (VID(window_debug)(),patx,paty-5,4,fg,PG_LGOP_NONE);
  VID(bar) (VID(window_debug)(),patx+patw,paty-5,4,fg,PG_LGOP_NONE);
  VID(bar) (VID(window_debug)(),patx,paty+patw+2,4,fg,PG_LGOP_NONE);
  VID(bar) (VID(window_debug)(),patx+patw,paty+patw+2,4,fg,PG_LGOP_NONE);

  /* Line thingies within the box */
  for (i=0;i<=patw;i+=4) {
    VID(line) (VID(window_debug)(),patx+i,paty,patx+patw,paty+i,fg,PG_LGOP_NONE);
    VID(line) (VID(window_debug)(),patx,paty+i,patx+i,paty+patw,fg,PG_LGOP_NONE);
  }

  fd->lib->draw_string(fd,VID(window_debug)(),
		       xy_to_pair(vid->lxres>>1,vid->lyres>>1),
		       fg,pgstring_tmpwrap("1"),&clip,PG_LGOP_NONE,0);
}

/************ Color test pattern */

void testpat_color(void) {
  hwrcolor bg = VID(color_pgtohwr) (0x000000);
  hwrcolor fg = VID(color_pgtohwr) (0xFFFFFF);
  struct font_descriptor *fd;
  int y=0;
  int h;
  struct font_metrics m;
  struct pgquad clip = {0,0,vid->xres-1,vid->yres-1};
   
  rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,res[PGRES_DEFAULT_FONT]);
  fd->lib->getmetrics(fd,&m);
  h = m.lineheight;
   
  /* Background */
  VID(rect) (VID(window_debug)(),0,0,vid->lxres,vid->lyres,bg,PG_LGOP_NONE);
   
  fd->lib->draw_string(fd,VID(window_debug)(),xy_to_pair(0,y),fg,
		       pgstring_tmpwrap("Black -> White"),&clip,PG_LGOP_NONE,0);
  y+=h;
  VID(gradient) (VID(window_debug)(),0,y,vid->lxres,h*2,0,0x000000,0xFFFFFF,
		 PG_LGOP_NONE);
  y+=2*h;

  fd->lib->draw_string(fd,VID(window_debug)(),xy_to_pair(0,y),fg,
		       pgstring_tmpwrap("White -> Black"),&clip,PG_LGOP_NONE,0);
  y+=h;
  VID(gradient) (VID(window_debug)(),0,y,vid->lxres,h*2,0,0xFFFFFF,0x000000,
		 PG_LGOP_NONE);
  y+=2*h;

  fd->lib->draw_string(fd,VID(window_debug)(),xy_to_pair(0,y),fg,
		       pgstring_tmpwrap("Black -> Red"),&clip,PG_LGOP_NONE,0);
  y+=h;
  VID(gradient) (VID(window_debug)(),0,y,vid->lxres,h*2,0,0x000000,0xFF0000,
		 PG_LGOP_NONE);
  y+=2*h;
   
  fd->lib->draw_string(fd,VID(window_debug)(),xy_to_pair(0,y),fg,
		       pgstring_tmpwrap("Black -> Green"),&clip,PG_LGOP_NONE,0);
  y+=h;
  VID(gradient) (VID(window_debug)(),0,y,vid->lxres,h*2,0,0x000000,0x00FF00,
		 PG_LGOP_NONE);
  y+=2*h;
   
  fd->lib->draw_string(fd,VID(window_debug)(),xy_to_pair(0,y),fg,
		       pgstring_tmpwrap("Black -> Blue"),&clip,PG_LGOP_NONE,0);
  y+=h;
  VID(gradient) (VID(window_debug)(),0,y,vid->lxres,h*2,0,0x000000,0x0000FF,
		 PG_LGOP_NONE);
  y+=2*h;

  fd->lib->draw_string(fd,VID(window_debug)(),xy_to_pair(0,y),fg,
		       pgstring_tmpwrap("Blue -> Red"),&clip,PG_LGOP_NONE,0);
  y+=h;
  VID(gradient) (VID(window_debug)(),0,y,vid->lxres,h*2,0,0x0000FF,0xFF0000,
		 PG_LGOP_NONE);
  y+=2*h;

}

/************ Blit/unblit test pattern */

void testpat_unblit(void) {
  hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
  hwrcolor fg = VID(color_pgtohwr) (0x000000);
  struct font_descriptor *fd;
  int patx,paty,patw;
  int patxstart;
  int i;
  hwrbitmap bit;
  char buf[20];
  struct pgquad clip = {0,0,vid->xres-1,vid->yres-1};
   
  rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,res[PGRES_DEFAULT_FONT]);
   
  /* Background */
  VID(rect) (VID(window_debug)(),0,0,vid->lxres,vid->lyres,bg,PG_LGOP_NONE);

  /* test pattern bounding box */
  patw = 50;
  patx = patxstart = 10;
  VID(bar) (VID(window_debug)(),patw+25,0,vid->lyres,fg,PG_LGOP_NONE);
   
  /* Repeat for different pixel alignments */
  for (paty=10;paty+patw<vid->lyres;paty+=patw+15,patx=++patxstart,patw--) {
      
    /* Draw little alignment marks, a 1-pixel gap from the test pattern */
    VID(slab) (VID(window_debug)(),patx-5,paty,4,fg,PG_LGOP_NONE);
    VID(slab) (VID(window_debug)(),patx-5,paty+patw,4,fg, PG_LGOP_NONE);
    VID(slab) (VID(window_debug)(),patx+patw+2,paty,4,fg, PG_LGOP_NONE);
    VID(slab) (VID(window_debug)(),patx+patw+2,paty+patw,4,fg, PG_LGOP_NONE);
    VID(bar) (VID(window_debug)(),patx,paty-5,4,fg, PG_LGOP_NONE);
    VID(bar) (VID(window_debug)(),patx+patw,paty-5,4,fg, PG_LGOP_NONE);
    VID(bar) (VID(window_debug)(),patx,paty+patw+2,4,fg, PG_LGOP_NONE);
    VID(bar) (VID(window_debug)(),patx+patw,paty+patw+2,4,fg, PG_LGOP_NONE);
      
    /* Line thingies within the box */
    VID(rect) (VID(window_debug)(),patx,paty,patw+1,patw+1,fg, PG_LGOP_NONE);
    VID(rect) (VID(window_debug)(),patx+1,paty+1,patw-1,patw-1,bg, PG_LGOP_NONE);
    for (i=0;i<=patw;i+=3)
      VID(line) (VID(window_debug)(),patx+i,paty+patw,patx+patw,paty+i,
		 fg, PG_LGOP_NONE);
    snprintf(buf,sizeof(buf)-1,"%d/%d",patx&7,patw);
    buf[sizeof(buf)-1]=0;
    fd->lib->draw_string(fd,VID(window_debug)(),xy_to_pair(patx+2,paty+2),fg,
			 pgstring_tmpwrap(buf),&clip,PG_LGOP_NONE,0);
      
    /* Blit the bounding box */
    VID(bitmap_new) (&bit,patw+1,patw+1,vid->bpp);
    VID(blit) (bit,0,0,patw+1,patw+1,VID(window_debug)(),patx,paty,PG_LGOP_NONE);
      
    /* Same pattern, shifted to the side in various alignments */
    for (patx=patw+40,i=0;patx+patw<vid->lxres;patx=((patx+patw+25)&(~7))+(i++)) {
	 
      VID(slab) (VID(window_debug)(),patx-5,paty,4,fg,PG_LGOP_NONE);
      VID(slab) (VID(window_debug)(),patx-5,paty+patw,4,fg,PG_LGOP_NONE);
      VID(slab) (VID(window_debug)(),patx+patw+2,paty,4,fg,PG_LGOP_NONE);
      VID(slab) (VID(window_debug)(),patx+patw+2,paty+patw,4,fg,PG_LGOP_NONE);
      VID(bar) (VID(window_debug)(),patx,paty-5,4,fg,PG_LGOP_NONE);
      VID(bar) (VID(window_debug)(),patx+patw,paty-5,4,fg,PG_LGOP_NONE);
      VID(bar) (VID(window_debug)(),patx,paty+patw+2,4,fg,PG_LGOP_NONE);
      VID(bar) (VID(window_debug)(),patx+patw,paty+patw+2,4,fg,PG_LGOP_NONE);
	 
      VID(blit) (VID(window_debug)(),patx,paty,patw+1,patw+1,bit,0,0,PG_LGOP_NONE);
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
  VID(rect) (VID(window_debug)(),0,0,vid->lxres,vid->lyres,bg,PG_LGOP_NONE);

  for (i=0;i<16;i++) {
    VID(slab) (VID(window_debug)(),5+i,5+(i<<1),12,fg,PG_LGOP_NONE);
    VID(slab) (VID(window_debug)(),35+i,5+(i<<1),8,fg,PG_LGOP_NONE);
    VID(slab) (VID(window_debug)(),65+i,5+(i<<1),5,fg,PG_LGOP_NONE);

    VID(slab) (VID(window_debug)(),5,45+(i<<1),i+1,fg,PG_LGOP_NONE);
    VID(slab) (VID(window_debug)(),35+i,45+(i<<1),i+1,fg,PG_LGOP_NONE);

    VID(bar) (VID(window_debug)(),80+(i<<1),5+i,10,fg,PG_LGOP_NONE);
  }
}

/************ Stipple rectangle test pattern */

void testpat_stipple(void) {
  hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
  hwrcolor fg = VID(color_pgtohwr) (0x000000);
  int i;
  int h = vid->lyres/16;

  /* Background */
  VID(rect) (VID(window_debug)(),0,0,vid->lxres>>1,vid->lyres,bg,PG_LGOP_NONE);
  VID(rect) (VID(window_debug)(),vid->lxres>>1,0,vid->lxres>>1,vid->lyres,
	     fg,PG_LGOP_NONE);

  for (i=0;i<16;i++) {
    VID(rect) (VID(window_debug)(),i,i*h,
	       vid->lxres-(i<<1),h,i&1 ? fg:bg,PG_LGOP_STIPPLE);

  }
}

/************ Text test pattern */

void testpat_text(void) {
  hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
  struct font_descriptor *fd;
  struct pgpair p;
  u8 c;
  struct font_metrics m;
  struct pgquad clip = {0,0,vid->xres-1,vid->yres-1};   

  rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,res[PGRES_DEFAULT_FONT]);
  fd->lib->getmetrics(fd,&m);
   
  /* Background */
  VID(rect) (VID(window_debug)(),0,0,vid->lxres,vid->lyres,bg,PG_LGOP_NONE);

  /* Draw characters! */
  p.x=p.y=0;
  c=' ';
  while (1) {
    if (p.x + m.charcell.w > vid->xres) {
      p.y += m.lineheight;
      p.x = 0;
    }
    if (p.y + m.lineheight > vid->yres)
      return;
    if (c>'~')
      c = ' ';
    fd->lib->draw_char(fd,VID(window_debug)(),&p,0,c++,&clip,PG_LGOP_NONE,0);
  }
}

/************ alpha blit test pattern */

/* This is a PNG file with alpha channel */
#include "alphatest.png.h"

hwrbitmap alphatest_bitmap = NULL;  

void testpat_alpha() {
  /* Load it the first time */
  if (!alphatest_bitmap) {
    if (iserror(vid->bitmap_load(&alphatest_bitmap, alpha_png_bits, alpha_png_len))) {
      printf("Alpha channel test requires PNG loader\n");
    }
    VID(rect) (VID(window_debug)(),0,0,vid->lxres,vid->lyres,
	       VID(color_pgtohwr)(0xFFFFFF),PG_LGOP_NONE);
  }
  
  VID(blit) (VID(window_debug)(),
	     (vid->lxres - alpha_png_width)>>1,
	     (vid->lyres - alpha_png_height)>>1,
	     alpha_png_width,alpha_png_height,alphatest_bitmap,
	     0,0,PG_LGOP_ALPHA);
}


/************ blur */

void testpat_blur() {
  VID(blur) (VID(window_debug)(), 0,0, vid->lxres, vid->lyres, 1);
}


/************ Front-end */

void videotest_help(void) {
  puts("\nVideo test modes:\n"
       "\t1\tLine test pattern\n"
       "\t2\tColor test pattern\n"
       "\t3\tBlit/unblit test pattern\n"
       "\t4\tSlab alignment test pattern\n"
       "\t5\tStippled rectangle test pattern\n"
       "\t6\tText test pattern\n"
       "\t7\tAlpha channel test\n"
       "\t8\tblur test\n"
       );
}


void videotest_run(s16 number) {
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
  case 5:
    testpat_stipple();
    break;
  case 6:
    testpat_text();
    break;
  case 7:
    testpat_alpha();
    break;
  case 8:
    testpat_blur();
    break;
  default:
    printf("Unknown video test mode");
    exit(1);
  }

  VID(update) (VID(window_debug)(),0,0,vid->lxres,vid->lyres);

  /* If we had to create the alpha test bitmap, clean that up now */
  if (alphatest_bitmap) {
    VID(bitmap_free)(alphatest_bitmap);
    alphatest_bitmap = NULL;
  }
}

/* The End */

