/* $Id$
 *
 * dvbl_effects.c - This file is part of the Default Video Base Library,
 *                  providing the basic video functionality in picogui but
 *                  easily overridden by drivers.
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
 */

#include <pgserver/common.h>

#ifdef DEBUG_VIDEO
#define DEBUG_FILE
#endif
#include <pgserver/debug.h>

#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/render.h>


void def_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		  pgcolor c1, pgcolor c2, s16 lgop) {
  /*
    The angle is expressed in degrees.
    
    This implementation is based on the function:
    color = y*sin(angle) + x*cos(angle)
    with scaling added to keep color between the specified values.
    The main improvement (?) is that it has been munged to use
    fixed-point calculations, and only addition and shifting in the
    inner loop.
   
    Wow, algebra and trigonometry are useful for something!  ;-)

    Unless the device has hardware-accelerated gradients (!) the
    set-up will be about the same.  Optimizations could include the usual:
       Use framebuffer offset for y
       Put pixel code directly in here
       Assembler
  */

  /* Lotsa vars! */
  s32 r_vs,g_vs,b_vs,r_sa,g_sa,b_sa,r_ca,g_ca,b_ca,r_ica,g_ica,b_ica;
  s32 r_vsc,g_vsc,b_vsc,r_vss,g_vss,b_vss,sc_d;
  s32 r_v1,g_v1,b_v1,r_v2,g_v2,b_v2;
  s16 i,s,c,x1;
#ifdef CONFIG_DITHER_GRADIENTS
  hwrdither dither;
#endif

  /* Look up the sine and cosine */
  angle %= 360;
  if (angle<0) angle += 360;
  if      (angle <= 90 ) s =  trigtab[angle];
  else if (angle <= 180) s =  trigtab[180-angle];
  else if (angle <= 270) s = -trigtab[angle-180];
  else                   s = -trigtab[360-angle];
  angle += 90;
  if (angle>=360) angle -= 360;
  if      (angle <= 90 ) c =  trigtab[angle];
  else if (angle <= 180) c =  trigtab[180-angle];
  else if (angle <= 270) c = -trigtab[angle-180];
  else                   c = -trigtab[360-angle];

  /* Calculate denominator of the scale value */
  sc_d = h*((s<0) ? -((s32)s) : ((s32)s)) +
    w*((c<0) ? -((s32)c) : ((s32)c));

  /* Decode colors */
  r_v1 = getred(c1);
  g_v1 = getgreen(c1);
  b_v1 = getblue(c1);
  r_v2 = getred(c2);
  g_v2 = getgreen(c2);
  b_v2 = getblue(c2);

  /* Calculate the scale values and 
   * scaled sine and cosine (for each channel) */
  r_vs = ((r_v2<<16)-(r_v1<<16)) / sc_d;
  g_vs = ((g_v2<<16)-(g_v1<<16)) / sc_d;
  b_vs = ((b_v2<<16)-(b_v1<<16)) / sc_d;

  /* Zero the accumulators */
  r_sa = g_sa = b_sa = r_ca = g_ca = b_ca = r_ica = g_ica = b_ica = 0;

  /* Calculate the sine and cosine scales */
  r_vsc = (r_vs*((s32)c)) >> 8;
  r_vss = (r_vs*((s32)s)) >> 8;
  g_vsc = (g_vs*((s32)c)) >> 8;
  g_vss = (g_vs*((s32)s)) >> 8;
  b_vsc = (b_vs*((s32)c)) >> 8;
  b_vss = (b_vs*((s32)s)) >> 8;

  /* If the scales are negative, start from the opposite side */
  if (r_vss<0) r_sa  = -r_vss*h;
  if (r_vsc<0) r_ica = -r_vsc*w; 
  if (g_vss<0) g_sa  = -g_vss*h;
  if (g_vsc<0) g_ica = -g_vsc*w; 
  if (b_vss<0) b_sa  = -b_vss*h;
  if (b_vsc<0) b_ica = -b_vsc*w; 

  if (r_v2<r_v1) r_v1 = r_v2;
  if (g_v2<g_v1) g_v1 = g_v2;
  if (b_v2<b_v1) b_v1 = b_v2;

#ifdef CONFIG_DITHER_GRADIENTS
  /* Start dithering */
  if (iserror(vid->dither_start(&dither, dest, 0,x,y,w,h)))
    return;
#endif

  /* Finally, the loop! */
  
  for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++)
    for (x1=x,r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
	 i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc,x1++) {
      
#ifdef CONFIG_DITHER_GRADIENTS
      vid->dither_store(dither,mkcolor(
				       r_v1 + ((r_ca+r_sa) >> 8),
				       g_v1 + ((g_ca+g_sa) >> 8),
				       b_v1 + ((b_ca+b_sa) >> 8)),lgop);
#else
      vid->pixel(dest,x1,y,(*vid->color_pgtohwr)
		 (mkcolor(
			  r_v1 + ((r_ca+r_sa) >> 8),
			  g_v1 + ((g_ca+g_sa) >> 8),
			  b_v1 + ((b_ca+b_sa) >> 8))),lgop);
#endif
    }
  
#ifdef CONFIG_DITHER_GRADIENTS
  vid->dither_finish(dither);
#endif  
}

/* This isn't nearly as slow as a real gaussian blur, but it still ain't great.
 * In a framebuffer-specific implementation, a lot of the pointer arithmetic and
 * color conversion is unnecessary.
 */  
void def_blur(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, s16 radius) {
  int i,j;
  int r1,r2,r3;  /* 3-pixel buffer for blurring */
  int g1,g2,g3;
  int b1,b2,b3;
  hwrcolor c;
  s16 imgw, imgh;

  /* Don't blur the edge pixel on the screen. 
   * Yeah, I'm a wimp for not making it wrap around :P
   */
  vid->bitmap_getsize(dest,&imgw,&imgh);
  if (x<=0) x = 1;
  if (h<=0) y = 1;
  if (x+w>=imgw) w = imgw-x-1;
  if (y+h>=imgh) h = imgh-y-1;

  /* Repeat the same basic 3x3 blur to get more blurrring. Each iteration
   * covers 2 additional pixels, therefore one extra radius unit.
   */
  while (radius--) {

    /* Horizontal blur */
    for (j=0;j<h;j++) {

      /* Throughout the blur we keep our 3-pixel buffer full, but at the beginning we need to prime it */
      c = vid->color_hwrtopg(vid->getpixel(dest,x,y+j));
      r2 = getred(c);
      g2 = getgreen(c);
      b2 = getblue(c);
      c = vid->color_hwrtopg(vid->getpixel(dest,x+1,y+j));
      r3 = getred(c);
      g3 = getgreen(c);
      b3 = getblue(c);

      for (i=0;i<w;i++) {
	c = vid->color_hwrtopg(vid->getpixel(dest,x+i+1,y+j));

	r1 = r2;
	r2 = r3;
	r3 = getred(c);
	g1 = g2;
	g2 = g3;
	g3 = getgreen(c);
	b1 = b2;
	b2 = b3;
	b3 = getblue(c);
	c = mkcolor(((r1+r2+r3)/3),
		    ((g1+g2+g3)/3),
		    ((b1+b2+b3)/3));

	vid->pixel(dest,x+i,y+j,vid->color_pgtohwr(c),PG_LGOP_NONE);
      }
    }

    /* Vertical blur */
    for (i=0;i<w;i++) {

      /* Throughout the blur we keep our 3-pixel buffer full, but at the beginning we need to prime it */
      c = vid->color_hwrtopg(vid->getpixel(dest,x+i,y));
      r2 = getred(c);
      g2 = getgreen(c);
      b2 = getblue(c);
      c = vid->color_hwrtopg(vid->getpixel(dest,x+i,y+1));
      r3 = getred(c);
      g3 = getgreen(c);
      b3 = getblue(c);
      
      for (j=0;j<h;j++) {
	c = vid->color_hwrtopg(vid->getpixel(dest,x+i,y+j+1));

	r1 = r2;
	r2 = r3;
	r3 = getred(c);
	g1 = g2;
	g2 = g3;
	g3 = getgreen(c);
	b1 = b2;
	b2 = b3;
	b3 = getblue(c);
	c = mkcolor(((r1+r2+r3)/3),
		    ((g1+g2+g3)/3),
		    ((b1+b2+b3)/3));

	vid->pixel(dest,x+i,y+j,vid->color_pgtohwr(c),PG_LGOP_NONE);
      }
    }
  }
}

/* The End */
