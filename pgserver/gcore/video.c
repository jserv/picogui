/* $Id: video.c,v 1.4 2000/09/02 17:19:15 micahjd Exp $
 *
 * video.c - handles loading/switching video drivers, provides
 *           default implementations for video functions
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micah@homesoftware.com>
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

#include <video.h>
#include <g_malloc.h>

/******************************************** Utils */

/* Vidlib vars */
struct vidlib *vid;
struct vidlib vidlib_static;

/* Trig table used in hwr_gradient (sin*256 for theta from 0 to 90) */
unsigned char trigtab[] = {
  0x00,0x04,0x08,0x0D,0x11,0x16,0x1A,0x1F,0x23,0x28,
  0x2C,0x30,0x35,0x39,0x3D,0x42,0x46,0x4A,0x4F,0x53,
  0x57,0x5B,0x5F,0x64,0x68,0x6C,0x70,0x74,0x78,0x7C,
  0x80,0x83,0x87,0x8B,0x8F,0x92,0x96,0x9A,0x9D,0xA1,
  0xA4,0xA7,0xAB,0xAE,0xB1,0xB5,0xB8,0xBB,0xBE,0xC1,
  0xC4,0xC6,0xC9,0xCC,0xCF,0xD1,0xD4,0xD6,0xD9,0xDB,
  0xDD,0xDF,0xE2,0xE4,0xE6,0xE8,0xE9,0xEB,0xED,0xEE,
  0xF0,0xF2,0xF3,0xF4,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,
  0xFC,0xFC,0xFD,0xFE,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF
};

/* Little function to skip to the next value in an ASCII file */
void ascskip(unsigned char **dat,unsigned long *datlen) {
  while (*datlen) {
    if (**dat == ' ' || **dat == '\t' || **dat == '\n' || **dat == '\r') {
      (*dat)++;
      (*datlen)--;
    }
    else if (**dat == '#') {
      while ((*datlen) && (**dat != '\n') && (**dat != '\r')) {
	(*dat)++;
	(*datlen)--;
      }
    }
    else
      return;
  }
}

/* Read a number from ascii data */
int ascread(unsigned char **dat,unsigned long *datlen) {
  char buf[10];
  char *p = buf;
  int buflen=9;
  ascskip(dat,datlen);
  while ((**dat != ' ') && (**dat != '\t') && (**dat != '\n') &&
	 (**dat != '\r') && *datlen && buflen) {
    *(p++) = *((*dat)++);
    (*datlen)--;
    buflen--;
  }
  *p = 0;
  return atoi(buf);
}


/******************************************** Default implementations */
/* See the API overview in video.h for descriptions on how to use
   these functions (they aren't called directly)
*/

g_error def_setmode(int xres,int yres,int bpp,unsigned long flags) {
  return mkerror(ERRT_BADPARAM,72);
}

void emulate_dos(void) {
}

void def_clip_set(int x1,int y1,int x2,int y2) {
  vid->clip_x1 = x1;
  vid->clip_y1 = y1;
  vid->clip_x2 = x2;
  vid->clip_y2 = y2;
}

void def_clip_off(void) {
  (*vid->clip_set)(0,0,vid->xres-1,vid->yres-1);
}

hwrcolor def_color_pgtohwr(pgcolor c) {
  if (vid->bpp<8) {
    /* grayscale */
    return (((getred(c)*3+getgreen(c)*6+getblue(c))/10) >>
	    (8-vid->bpp))&((1<<vid->bpp)-1);
  }
  else if (vid->bpp==8) {
    /* 2-3-3 color */
    return ((getred(c) & 0xC0) |
	    ((getgreen(c) >> 2) & 0x38) |
	    ((getblue(c) >> 5) & 0x07));
  }
  else if (vid->bpp==16) {
    /* 5-6-5 color */
    return (((getred(c) << 8) & 0xF800) |
	    ((getgreen(c) << 3) & 0x07E0) |
	    ((getblue(c) >> 3) & 0x001F));
  }
  else
    /* True color */
    return c;
}

pgcolor def_color_hwrtopg(hwrcolor c) {
  return c;
}

void def_addpixel(int x,int y,pgcolor c) {
  /* Blech. Convert it to an RGB first for the proper effect.
     Don't let the RGB components roll over either
  */
  int r,g,b;
  pgcolor oc = (*vid->color_hwrtopg)((*vid->getpixel)(x,y));

  r = getred(oc);
  g = getgreen(oc);
  b = getblue(oc);
  r += getred(c);
  g += getgreen(c);
  b += getblue(c);
  
  if (r>255) r=255;
  if (g>255) g=255;
  if (b>255) b=255;

  (*vid->pixel)(x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)));
}

void def_subpixel(int x,int y,pgcolor c) {
  /* Same idea, but subtract */
  int r,g,b;
  pgcolor oc = (*vid->color_hwrtopg)((*vid->getpixel)(x,y));

  r = getred(oc);
  g = getgreen(oc);
  b = getblue(oc);
  r -= getred(c);
  g -= getgreen(c);
  b -= getblue(c);
  
  if (r<0) r=0;
  if (g<0) g=0;
  if (b<0) b=0;

  (*vid->pixel)(x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)));
}

/* Should be more than OK for most situations */
void def_clear(void) {
  (*vid->rect)(0,0,vid->xres,vid->yres,0);
}

void def_slab(int x,int y,int w,hwrcolor c) {
  (*vid->rect)(x,y,w,1,c);
}

void def_bar(int x,int y,int h,hwrcolor c) {
  (*vid->rect)(x,y,1,h,c);
}

/* There are about a million ways to optimize this-
   At the very least, combine it with pixel() and
   have it keep track of the y coord as a memory offset
   to get rid of the multiply in pixel().

   The old SDL driver used this optimization, but it
   was too driver-specific, so had to revert to the
   generic bresenham's for the default implementation.
*/
void def_line(int x1,int y1,int x2,int y2,hwrcolor c) {
  int stepx, stepy;
  int dx = x2-x1;
  int dy = y2-y1;
  int fraction;

  dx = x2-x1;
  dy = y2-y1;

  if (dx<0) { 
    dx = -(dx << 1);
    stepx = -1; 
  } else {
    dx = dx << 1;
    stepx = 1;
  }
  if (dy<0) { 
    dy = -(dy << 1);
    stepy = -1; 
  } else {
    dy = dy << 1;
    stepy = 1;
  }

  (*vid->pixel)(x1,y1,c);

  /* Clipping */
  
  /* Major axis is horizontal */
  if (dx > dy) {
    fraction = dy - (dx >> 1);
    while (x1 != x2) {
      if (fraction >= 0) {
	y1 += stepy;
	fraction -= dx;
      }
      x1 += stepx;
      fraction += dy;
      
      (*vid->pixel)(x1,y1,c);
    }
  } 
  
  /* Major axis is vertical */
  else {
    fraction = dx - (dy >> 1);
    while (y1 != y2) {
      if (fraction >= 0) {
	x1 += stepx;
	fraction -= dy;
      }
      y1 += stepy;
      fraction += dx;
      
      (*vid->pixel)(x1,y1,c);
    }
  }
}

void def_rect(int x,int y,int w,int h,hwrcolor c) {
  int i,j,x1;

  for (i=0;i<h;i++,y++)
    for (x1=x,j=0;j<w;j++,x1++)
      (*vid->pixel)(x1,y,c);
}

void def_gradient(int x,int y,int w,int h,int angle,
		  pgcolor c1, pgcolor c2,int translucent) {
  /*
    The angle is expressed in degrees.
    If translucent is positive it will add the gradient to the existing
    pixels. If negative, it will subtract.  If it is zero, it performs
    a normal overwrite operation.
    
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
  long r_vs,g_vs,b_vs,r_sa,g_sa,b_sa,r_ca,g_ca,b_ca,r_ica,g_ica,b_ica;
  long r_vsc,g_vsc,b_vsc,r_vss,g_vss,b_vss,sc_d;
  long r_v1,g_v1,b_v1,r_v2,g_v2,b_v2;
  int i,s,c,x1;

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
  sc_d = h*((s<0) ? -((long)s) : ((long)s)) +
    w*((c<0) ? -((long)c) : ((long)c));

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
  r_vsc = (r_vs*((long)c)) >> 8;
  r_vss = (r_vs*((long)s)) >> 8;
  g_vsc = (g_vs*((long)c)) >> 8;
  g_vss = (g_vs*((long)s)) >> 8;
  b_vsc = (b_vs*((long)c)) >> 8;
  b_vss = (b_vs*((long)s)) >> 8;

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

  /* Finally, the loop! */

  if (translucent==0) {
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++)
      for (x1=x,r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
	   i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc,x1++) {

	(*vid->pixel)(x1,y,(*vid->color_pgtohwr)
		      (mkcolor(
			       r_v1 + ((r_ca+r_sa) >> 8),
			       g_v1 + ((g_ca+g_sa) >> 8),
			       b_v1 + ((b_ca+b_sa) >> 8))));
      }
  }
  else if (translucent>0) {
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++)
      for (x1=x,r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
	   i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc,x1++) {

	(*vid->addpixel)(x1,y,mkcolor(
				      r_v1 + ((r_ca+r_sa) >> 8),
				      g_v1 + ((g_ca+g_sa) >> 8),
				      b_v1 + ((b_ca+b_sa) >> 8)));
      }
  }
  else {
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++)
      for (x1=x,r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
	   i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc,x1++) {

	(*vid->subpixel)(x1,y,mkcolor(
				      r_v1 + ((r_ca+r_sa) >> 8),
				      g_v1 + ((g_ca+g_sa) >> 8),
				      b_v1 + ((b_ca+b_sa) >> 8)));
      }
  }
}

void def_frame(int x,int y,int w,int h,hwrcolor c) {
  (*vid->slab)(x,y,w,c);
  (*vid->slab)(x,y+h-1,w,c);
  (*vid->bar)(x,y+1,h-2,c);
  (*vid->bar)(x+w-1,y+1,h-2,c);
}

void def_dim(void) {
  int x,y;

  /* 'Cute' little checkerboard thingy. Devices with high/true color
   * or even 256 color should make this do real dimming to avoid
   * looking stupid  ;-)
   */
  for (y=vid->clip_y1;y<=vid->clip_y2;y++)
    for (x=vid->clip_x1+(y&1);x<=vid->clip_x2;x+=2)
      (*vid->pixel)(x,y,0);
}

void def_scrollblit(int src_x,int src_y,
		    int dest_x,int dest_y,
		    int w,int h) {
  
  /* This shouldn't be _too_ bad in most cases...
     The pixels within a line are as fast as blit()
     but this introduces function call overhead between lines
  */

  for (src_y+=h-1,dest_y+=h-1;h;h--,src_y--,dest_y--)
    (*vid->blit)(NULL,src_x,src_y,NULL,dest_x,dest_y,w,1,LGOP_NONE);
}

void def_charblit(unsigned char *chardat,int dest_x,
		  int dest_y,int w,int h,int lines,
		  hwrcolor c) {
  int bw = w;
  int iw,bit,x,i;
  int olines = lines;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (dest_x>vid->clip_x2 || dest_y>vid->clip_y2 || 
      (dest_x+w)<vid->clip_x1 || (dest_y+h)<vid->clip_y1) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  bw &= 0x1F;

  for (i=0;i<h;i++,dest_y++) {
    /* Skewing */
    if (olines && lines==i) {
      lines += olines;
      dest_x--;
    }
    for (x=dest_x,iw=bw;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x++)
	if (ch&0x80) (*vid->pixel)(x,dest_y,c); 
  }
}
   

/* Not sure this works in all cases. Needs
   more testing...
*/
g_error def_bitmap_loadxbm(struct stdbitmap **bmp,
			   unsigned char *data,
			   int w,int h,
			   hwrcolor fg,
			   hwrcolor bg) {
  int i,bit;
  unsigned char c;
  unsigned char *p;
  g_error e;

  e = (*vid->bitmap_new)(bmp,w,h);
  errorcheck;
  p = (*bmp)->bits;

  /* The easy case */
  if (vid->bpp == 1) {
    memcpy(p,data,h*(w/8));
    return sucess;
  }

  /* Shift in the pixels! */
  for (;h>0;h--)
    for (bit=0,i=0;i<w;i++) {
      if (!bit) c = *(data++);
      
      switch (vid->bpp) {

	/* We don't need to check for byte-rollover here because
	   it will always occur on an even boundary.
	   (Unless we add a 3bpp or a 7bpp mode or something
	   really freaky like that)
	*/
      case 2:
	*(p++) = (c&1) ? fg : bg;
	c = c>>1;
	bit++;
	*(p++) |= ((c&1) ? fg : bg) << 2;	
	c = c>>1;
	bit++;
	*(p++) |= ((c&1) ? fg : bg) << 4;	
	c = c>>1;
	bit++;
	*(p++) |= ((c&1) ? fg : bg) << 6;	
	break;

      case 4:
	*(p++) = (c&1) ? fg : bg;
	c = c>>1;
	bit++;
	*(p++) |= ((c&1) ? fg : bg) << 4;	
	break;

      case 8:
	*(p++) = (c&1) ? fg : bg;
	break;

      case 16:
	*(((unsigned short *)p)++) = (c&1) ? fg : bg;
	break;

      case 24:
	if (c&1) {
	  *(p++) = (unsigned char) fg;
	  *(p++) = (unsigned char) (fg >> 8);
	  *(p++) = (unsigned char) (fg >> 16);
	}
	else {
	  *(p++) = (unsigned char) bg;
	  *(p++) = (unsigned char) (bg >> 8);
	  *(p++) = (unsigned char) (bg >> 16);
	}
	break;

      case 32:
	*(((unsigned long *)p)++) = (c&1) ? fg : bg;
	break;

#if DEBUG
	/* Probably not worth the error-checking time
	   in non-debug versions, as it would be caught
	   earlier (hopefully?)
	*/

      default:
	printf("Converting to unsupported BPP\n");
#endif

      }

      c = c>>1;
      bit++;
      if (bit==8) bit = 0;
    }

  return sucess;
}

g_error def_bitmap_loadpnm(struct stdbitmap **bmp,
			   unsigned char *data,
			   unsigned long datalen) {
  /* Convert from any of the pbmplus formats in binary or ascii */

  char format;
  int bin = 0;
  int bpp;
  int has_maxval=1;
  int w,h,max;
  int i,val,bit,r,g,b;
  unsigned char *p;
  int oshift = 0;
  g_error e;
  g_error efmt = mkerror(ERRT_BADPARAM,48);
  hwrcolor hc;

  ascskip(&data,&datalen);
  if (!datalen) return efmt;
  if (*(data++) != 'P') return efmt; datalen--;
  format = *(data++); datalen--;
  /* This decides whether the format is ascii or binary, and if it's
     PBM, PGM, or PPM. */
  switch (format) {
  case '4':
    bin=1;
  case '1':
    bpp=1;
    has_maxval=0;
    break;
  case '5':
    bin=1;
  case '2':
    bpp=8;
    break;
  case '6':
    bin=1;
  case '3':
    bpp=24;
    break;
  default:
    return efmt;
  }
  w = ascread(&data,&datalen);
  if (!datalen) return efmt;
  if (!w) return efmt;
  h = ascread(&data,&datalen);
  if (!datalen) return efmt;
  if (!h) return efmt;
  if (has_maxval) {
    max = ascread(&data,&datalen);
    if (!datalen) return efmt;
    if (!max) return efmt;
  }  

  /* One whitespace allowed before bitmap data */
  data++; datalen--;

  /* Check for a correct-sized data buffer */
  if (datalen < ((((w*bpp)%8) ? (w*bpp+8) : (w*bpp))/8*h)) {
    (*vid->bitmap_free)(*bmp);
    return efmt;
  }

  /* Set up the bitmap */
  e = (*vid->bitmap_new)(bmp,w,h);
  errorcheck;
  p = (*bmp)->bits;

  /* Read in the values, convert colors, output them... */
  for (;h>0;h--) {
    bit = 0;
    for (i=0;i<w;i++) {
      if (!bit)
	if (bin)
	  val = *(data++);
	else
	  val = ascread(&data,&datalen);

      /* Read in the RGB values */
      switch (bpp) {
      
      case 1:
	if (val&0x80)
	  r=g=b = 0;
	else
	  r=g=b = 255;
	
	/* Shift in bits, not bytes */
	val = val<<1;
	bit++;
	if (bit==8) bit = 0;
	break;

      case 8:
	/* grayscale... */
	if (max==255)
	  r=g=b = val;
	else
	  r=g=b = val * 255 / max;
	break;

      case 24:
	if (max==255)
	  r = val;
	else
	  r = val * 255 / max;

	/* Read in the other 2 bytes */
	if (bin)
	  g = *(data++);
	else
	  g = ascread(&data,&datalen);
	if (bin)
	  b = *(data++);
	else
	  b = ascread(&data,&datalen);
	if (max!=255) {
	  g = g*255/max;
	  b = b*255/max;
	}
	break;

      }

      /* Convert to hwrcolor */
      hc = (*vid->color_pgtohwr)(mkcolor(r,g,b));

      /* Output them in the device's bpp */
      switch (vid->bpp) {

      case 2:
      case 4:
      case 8:
	if (oshift)
	  *p |= hc << oshift;
	else
	  *p = hc;
	oshift += vid->bpp;
	if (oshift >= 8) {
	  oshift = 0;
	  p++;
	}
	break;

      case 16:
	*(((unsigned short *)p)++) = hc;
	break;

      case 24:
	*(p++) = (unsigned char) hc;
	*(p++) = (unsigned char) (hc >> 8);
	*(p++) = (unsigned char) (hc >> 16);
	break;

      case 32:
	*(((unsigned long *)p)++) = hc;
	break;

#if DEBUG
	/* Probably not worth the error-checking time
	   in non-debug versions, as it would be caught
	   earlier (hopefully?)
	*/

      default:
	printf("Converting to unsupported BPP\n");
#endif
	
      }
      
    }
  }
  
  return sucess;
}

g_error def_bitmap_new(struct stdbitmap **bmp,
		       int w,int h) {
  g_error e;

  e = g_malloc((void **) bmp,sizeof(struct stdbitmap));
  errorcheck;

  e = g_malloc((void **) &(*bmp)->bits,
	       (((unsigned long)w)*h*vid->bpp)/8);
  if (iserror(e)) {
    g_free(*bmp);
    return e;
  }

  (*bmp)->freebits = 1;
  (*bmp)->w = w;
  (*bmp)->h = h;
  
  return sucess;
}

void def_bitmap_free(struct stdbitmap *bmp) {
  if (!bmp) return;
  if (bmp->freebits) g_free(bmp->bits);
  g_free(bmp);
}

g_error def_bitmap_getsize(struct stdbitmap *bmp,int *w,int *h) {
  *w = bmp->w;
  *h = bmp->h;
}

/******************************************** Vidlib admin functions */

/* Set up a default vidlib, then let the driver add it's
   hardware specific things. Add the resulting structure
   to the list.
*/
g_error load_vidlib(g_error (*regfunc)(struct vidlib *v),
		  int xres,int yres,int bpp,unsigned long flags) {
  g_error e;

  /* Unload */
  if (vid) 
    (*vid->close)();

  /* Clear it */
  vid = &vidlib_static;
  memset(vid,0,sizeof(struct vidlib));
  
  /* Set defaults */
  vid->setmode = &def_setmode;
  vid->close = &emulate_dos;
  vid->clip_set = &def_clip_set;
  vid->clip_off = &def_clip_off;
  vid->color_pgtohwr = &def_color_pgtohwr;
  vid->color_hwrtopg = &def_color_hwrtopg;
  vid->addpixel = &def_addpixel;
  vid->subpixel = &def_subpixel;
  vid->clear = &def_clear;
  vid->update = &emulate_dos;
  vid->slab = &def_slab;
  vid->bar = &def_bar;
  vid->line = &def_line;
  vid->rect = &def_rect;
  vid->gradient = &def_gradient;
  vid->frame = &def_frame;
  vid->dim = &def_dim;
  vid->scrollblit = &def_scrollblit;
  vid->charblit = &def_charblit;
  vid->bitmap_loadxbm = &def_bitmap_loadxbm;
  vid->bitmap_loadpnm = &def_bitmap_loadpnm;
  vid->bitmap_new = &def_bitmap_new;
  vid->bitmap_free = &def_bitmap_free;
  vid->bitmap_getsize = &def_bitmap_getsize;
  
  /* Device specifics */
  e = (*regfunc)(vid);
  if (iserror(e)) {
    vid = NULL;
    return e;
  }

  /* Load new driver */
  e = (*vid->init)(xres,yres,bpp,flags);
  if (iserror(e)) {
    vid = NULL;
    return e;
  }

  return sucess;
}

/* The End */

