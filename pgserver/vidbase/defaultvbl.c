/* $Id: defaultvbl.c,v 1.15 2001/02/12 05:29:17 micahjd Exp $
 *
 * Video Base Library:
 * defaultvbl.c - Maximum compatibility, but has the nasty habit of
 *                pushing around individual pixels. Use if your device
 *                is nothing like a linear framebuffer.
 *                 
 *                This does, however, have sensable implementations
 *                for things like stdbitmap and loading the clipping
 *                registers, so other drivers may snatch a few functions
 *                here and there.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
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

#include <pgserver/video.h>
#include <pgserver/font.h>

/******* no-op functions */

g_error def_setmode(int xres,int yres,int bpp,unsigned long flags) {
  return mkerror(PG_ERRT_BADPARAM,72);
}

void def_font_newdesc(struct fontdesc *fd) {
}

void emulate_dos(void) {
}

void def_update(int x,int y,int w,int h) {
}

/******* colors */

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
  (*vid->rect)(0,0,vid->xres,vid->yres,(*vid->color_pgtohwr)(0));
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

void def_dim(int x,int y,int w,int h) {
  int i,xx;

  /* 'Cute' little checkerboard thingy. Devices with high/true color
   * or even 256 color should make this do real dimming to avoid
   * looking stupid  ;-)
   */
  for (;h;h--,y++)
    for (xx=x+(h&1),i=w;i;i--,xx+=2)
      (*vid->pixel)(xx,y,0);
}

void def_scrollblit(int src_x,int src_y,
		    int dest_x,int dest_y,
		    int w,int h) {
  
  /* This shouldn't be _too_ bad in most cases...
     The pixels within a line are as fast as blit()
     but this introduces function call overhead between lines
  */

  for (src_y+=h-1,dest_y+=h-1;h;h--,src_y--,dest_y--)
    (*vid->blit)(NULL,src_x,src_y,dest_x,dest_y,w,1,PG_LGOP_NONE);
}

void def_charblit(unsigned char *chardat,int dest_x,
		      int dest_y,int w,int h,int lines,
		      hwrcolor c,struct cliprect *clip) {
  int bw = w;
  int iw,hc,x;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax,clipping;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || dest_y>clip->y2 || (dest_x+w)<clip->x1 || 
      (dest_y+h)<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  clipping = 0;      /* This is set if we are being clipped,
			otherwise we can use a tight, fast loop */

  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y1>dest_y) {
      hc = clip->y1-dest_y; /* Do it this way so skewing doesn't mess up when clipping */
      dest_y += hc;
      chardat += hc*bw;
    }
    if (clip->y2<(dest_y+h))
      h = clip->y2-dest_y+1;
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->x1>dest_x) {
      xmin = clip->x1-dest_x;
      clipping = 1;
    }
    if (clip->x2<(dest_x+w)) {
      xmax = clip->x2-dest_x+1;
      clipping = 1;
    }
  }

  for (;hc<h;hc++,dest_y++) {
    if (olines && lines==hc) {
      lines += olines;
      dest_x--;
      flag=1;
    }
    for (x=dest_x,iw=bw,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x++,xpix++)
	if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	  (*vid->pixel)(x,dest_y,c); 
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

/* Like charblit, but rotate 90 degrees anticlockwise whilst displaying
 *
 * As this code was mostly copied from linear8, it probably has the same
 * subtle "smudging" bug noted in linear8.c
 */
void def_charblit_v(unsigned char *chardat,int dest_x,
		  int dest_y,int w,int h,int lines,
		  hwrcolor c,struct cliprect *clip) {
  int bw = w;
  int iw,hc,y;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax,clipping;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || (dest_y-w)>clip->y2 || (dest_x+h)<clip->x1 || 
      dest_y<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  clipping = 0;      /* This is set if we are being clipped,
			otherwise we can use a tight, fast loop */

  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->x1>dest_x) {
      hc = clip->x1-dest_x; /* Do it this way so skewing doesn't mess up when clipping */
      dest_x += hc;
      chardat += hc*bw;
    }
    if (clip->x2<(dest_x+h-1))
      h = clip->x2-dest_x+2;
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->y1>dest_y-w+1) {
      xmax = 1+dest_y-clip->y1;
      clipping = 1;
    }
    if (clip->y2<(dest_y)) {
      xmin = dest_y-clip->y2;
      clipping = 1;
    }
  }

  for (;hc<h;hc++,dest_x++) {
    if (olines && lines==hc) {
      lines += olines;
      dest_y++;
      flag=1;
    }
    for (iw=bw,y=dest_y,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,y--,xpix++)
	if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	  (*vid->pixel)(dest_x,y,c);
    if (flag) {
      xmax++;
      flag=0;
    }
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
  unsigned char *p,*pline;
  g_error e;

  e = (*vid->bitmap_new)((hwrbitmap *) bmp,w,h);
  errorcheck;
  p = pline = (*bmp)->bits;

  /* The easy case */
  if (vid->bpp == 1) {
    memcpy(p,data,h*(w/8));
    return sucess;
  }

  /* Shift in the pixels! */
  for (;h>0;h--,p=pline+=(*bmp)->pitch)
    for (bit=0,i=0;i<w;i++) {
      if (!bit) c = *(data++);
      
      switch (vid->bpp) {

	/* We don't need to check for byte-rollover here because
	   it will always occur on an even boundary.
	   (Unless we add a 3bpp or a 7bpp mode or something
	   really freaky like that)
	*/
      case 2:
	*p = ((c&1) ? fg : bg) << 6;
	c = c>>1;
	bit++;
	*p |= ((c&1) ? fg : bg) << 4;	
	c = c>>1;
	bit++;
	*p |= ((c&1) ? fg : bg) << 2;	
	c = c>>1;
	bit++;
	*(p++) |= ((c&1) ? fg : bg);
	break;

      case 4:
	*p = ((c&1) ? fg : bg) << 4;
	c = c>>1;
	bit++;
	*(p++) |= ((c&1) ? fg : bg);	
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

#if DEBUG_VIDEO
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
  unsigned char *p,*pline;
  int shiftset = 8-vid->bpp;
  int oshift;
  g_error e;
  g_error efmt = mkerror(PG_ERRT_BADPARAM,48);
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
  e = (*vid->bitmap_new)((hwrbitmap *)bmp,w,h);
  errorcheck;
  pline = p = (*bmp)->bits;

  /* Read in the values, convert colors, output them... */
  for (;h>0;h--,p=pline+=(*bmp)->pitch) {
    oshift=shiftset;
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

      case 1:
      case 2:
      case 4:
	if (oshift==shiftset)
	   *p = hc << oshift;
	else
	   *p |= hc << oshift;
	if (!oshift) {
	  oshift = shiftset;
	  p++;
	}
	 else
	   oshift -= vid->bpp;
	break;

      case 8:
	*(((unsigned char *)p)++) = hc;
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

#if DEBUG_VIDEO
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
  int lw;
   
  /* The bitmap and the header can be allocated seperately,
     but usually it is sufficient to make them one big
     chunk of memory.  It's 1 alloc vs 2.
  */

  /* Pad the line width up to the nearest byte */
  lw = (unsigned long) w * vid->bpp;
  if ((vid->bpp<8) && (lw & 7))
     lw += 8;
  lw >>= 3;
  e = g_malloc((void **) bmp,sizeof(struct stdbitmap) + (lw * h));
  errorcheck;

  (*bmp)->pitch = lw;
  (*bmp)->freebits = 0;
  (*bmp)->bits = ((unsigned char *)(*bmp)) + 
    sizeof(struct stdbitmap);
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

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

void def_tileblit(struct stdbitmap *src,
		  int src_x,int src_y,int src_w,int src_h,
		  int dest_x,int dest_y,int dest_w,int dest_h) {
  int i,j;

  /* Do a tiled blit */
  for (i=0;i<dest_w;i+=src_w)
    for (j=0;j<dest_h;j+=src_h)
      (*vid->blit)(src,src_x,src_y,dest_x+i,dest_y+j,min(dest_w-i,src_w),
		   min(dest_h-j,src_h),PG_LGOP_NONE);
}

/* Scary blit... Very bad for normal screens, but then again so is most of this
 * VBL. Could be helpful on odd devices like ncurses or some LCDs */
void def_blit(struct stdbitmap *srcbit,int src_x,int src_y,
	      int dest_x, int dest_y,
	      int w, int h, int lgop) {
   struct stdbitmap screen;
   int i;
   char *src,*srcline;
   hwrcolor c,s;
   int shiftset = 8-vid->bpp;
   int oshift;
   
   /* Screen-to-screen blit */
   if (!srcbit) {
      srcbit = &screen;
      screen.bits = NULL;
      screen.w = vid->xres;
      screen.h = vid->yres;
   }
   
   for (srcline=src=srcbit->bits;h;h--,src_y++,dest_y++,src=srcline+=srcbit->pitch)
     for (oshift=shiftset,i=0;i<w;i++) {
	if (lgop!=PG_LGOP_NONE)
	  s = (*vid->getpixel)(dest_x+i,dest_y);

	if (srcbit->bits)
	  switch (vid->bpp) {

	   case 1:
	   case 2:
	   case 4:
	     c = ((*src) >> oshift) & ((1<<vid->bpp)-1);
	     if (!oshift) {
		oshift = shiftset;
		src++;
	     }
	     else
	       oshift -= vid->bpp;
	     break; 
	   
	   case 8:
	     c = *(src++);
	     break;
	     
	   case 16:
	     c = *(((unsigned short *)src)++);
	     break;
	     
	   case 24:
	     c = (*(src++)) | ((*(src++))<<8) | ((*(src++))<<16);
	     break;
	     
	   case 32:
	     c = *(((unsigned long *)src)++);
	     break;     
	  }
	else
	  c = (*vid->getpixel)(src_x+i,src_y);

	switch (lgop) {
	 case PG_LGOP_NONE:       s  = c;  break;
	 case PG_LGOP_OR:         s |= c;  break;
	 case PG_LGOP_AND:        s &= c;  break;
	 case PG_LGOP_XOR:        s ^= c;  break;
	 case PG_LGOP_INVERT:     s  = c ^ 0xFFFFFFFF;  break;
	 case PG_LGOP_INVERT_OR:  s |= c ^ 0xFFFFFFFF;  break;
	 case PG_LGOP_INVERT_AND: s &= c ^ 0xFFFFFFFF;  break;
	 case PG_LGOP_INVERT_XOR: s ^= c ^ 0xFFFFFFFF;  break;
	}

	(*vid->pixel)(dest_x+i,dest_y,s);
	}

}

/* Another scary blit for desperate situations */
void def_unblit(int src_x,int src_y,
		struct stdbitmap *destbit,int dest_x,int dest_y,
		int w,int h) {
   int i;
   char *dest = destbit->bits;
   char *destline = dest;
   hwrcolor c;
   int shiftset = 8-vid->bpp;
   int oshift;
   
   for (;h;h--,src_y++,dest=destline+=destbit->pitch)
     for (oshift=shiftset,i=0;i<w;i++) {
	c = (*vid->getpixel)(src_x+i,src_y);

	switch (vid->bpp) {

	 case 1:
	 case 2:
	 case 4:
	   if (oshift==shiftset)
	     *dest = c << oshift;
	   else
	     *dest |= c << oshift;
	   if (!oshift) {
	      oshift = shiftset;
	      dest++;
	   }
	   else
	     oshift -= vid->bpp;
	   break;

	 case 8:
	   *(dest++) = c;
	   break;
	   
	 case 16:
	   *(((unsigned short *)dest)++) = c;
	   break;
	   
	 case 24:
	   *(dest++) = (unsigned char) c;
	   *(dest++) = (unsigned char) (c >> 8);
	   *(dest++) = (unsigned char) (c >> 16);
	   break;
	   	 
	 case 32:
	   *(((unsigned long *)dest)++) = c;
	   break;
	   
	}
      
   }
}

void def_sprite_show(struct sprite *spr) {
#ifdef DEBUG_VIDEO
   printf("def_sprite_show\n");
#endif
   
  if (spr->onscreen || !spr->visible) return;
   
  /* Clip to a divnode */
  if (spr->clip_to) {
    if (spr->x < spr->clip_to->x) spr->x = spr->clip_to->x;
    if (spr->y < spr->clip_to->y) spr->y = spr->clip_to->y;
    if (spr->x+spr->w > spr->clip_to->x+spr->clip_to->w)
      spr->x = spr->clip_to->x + spr->clip_to->w - spr->w;
    if (spr->y+spr->h > spr->clip_to->y+spr->clip_to->h)
      spr->y = spr->clip_to->y + spr->clip_to->h - spr->h;

    spr->ow = spr->w; spr->oh = spr->h;
  }
  else {
    /* Clip to screen edge, cursor style. For correct mouse cursor
     * functionality and for sanity. 
     */
    if (spr->x<0) 
       spr->x = 0;
    else if (spr->x>(vid->xres-1))
       spr->x = vid->xres-1;
     
    if (spr->y<0)
       spr->y = 0;
    else if (spr->y>(vid->yres-1))
       spr->y = vid->yres-1;
    
    spr->ow = vid->xres - spr->x;
    if (spr->ow > spr->w) spr->ow = spr->w;
    spr->oh = vid->yres - spr->y;
    if (spr->oh > spr->h) spr->oh = spr->h;     
  }

  /* Update coordinates */
  spr->ox = spr->x; spr->oy = spr->y;
  
  /* Grab a new backbuffer */
  (*vid->unblit)(spr->x,spr->y,spr->backbuffer,
  	       0,0,spr->ow,spr->oh);

  /* Display the sprite */
 if (spr->mask) {
    (*vid->blit)(spr->mask,0,0,
		 spr->x,spr->y,spr->ow,spr->oh,PG_LGOP_AND);
    (*vid->blit)(spr->bitmap,0,0,
		 spr->x,spr->y,spr->ow,spr->oh,PG_LGOP_OR);
  }
  else
   (*vid->blit)(spr->bitmap,0,0,
		 spr->x,spr->y,spr->ow,spr->oh,PG_LGOP_NONE);

  add_updarea(spr->x,spr->y,spr->ow,spr->oh);

  spr->onscreen = 1;
   
   /**** Debuggative Cruft - something I used to test line clipping ****/
/*
    {
	int xp[] = {
	   55,-5,-55,5,30,0,-30,0
	};
	int yp[] = {
	   5,55,-5,-55,0,-30,0,30
	};
	struct divnode d;
	struct gropnode g;
	int i;
	memset(&d,0,sizeof(d));
	memset(&g,0,sizeof(g));
	d.x = 100;
	d.y = 100;
	d.w = 93;
	d.h = 72;
	d.grop = &g;
	g.type = PG_GROP_LINE;
	g.param[0] = (*vid->color_pgtohwr)(0xFFFF00);
	(*vid->rect)(d.x,d.y,d.w,d.h,(*vid->color_pgtohwr)(0x004000));
	g.x = spr->x-d.x;
	g.y = spr->y-d.y;
	for (i=0;i<8;i++) {
	   g.x += g.w; 
	   g.y += g.h;
	   g.w = xp[i];
	   g.h = yp[i];
	   grop_render(&d);
	}
	(*vid->update)(d.x,d.y,d.w,d.h);
     }
*/

   /**** A very similar debuggative cruft to test text clipping ****/
/*
    {
      struct cliprect cr;
      struct fontdesc fd;

      memset(&fd,0,sizeof(fd));
      fd.fs = fontstyles;
      fd.font = fd.fs->normal;
      fd.hline = -1;
 
      cr.x1 = 100;
      cr.y1 = 100;
      cr.x2 = 150;
      cr.y2 = 150;
      (*vid->rect)(cr.x1,cr.y1,cr.x2-cr.x1+1,cr.y2-cr.y1+1,(*vid->color_pgtohwr)(0x004000));
//      outtext(&fd,spr->x,spr->y,(*vid->color_pgtohwr)(0xFFFF80),"Hello,\nWorld!",&cr);
//      outtext_v(&fd,spr->x,spr->y,(*vid->color_pgtohwr)(0xFFFF80),"Hello,\nWorld!",&cr);
        outtext_v(&fd,spr->x,spr->y,(*vid->color_pgtohwr)(0xFFFF80),"E",&cr);
      (*vid->update)(0,0,vid->xres,vid->yres);
    }
*/
   
}

void def_sprite_hide(struct sprite *spr) {
  static struct cliprect cr;

#ifdef DEBUG_VIDEO
   printf("def_sprite_hide\n");
#endif
   
  if ( (!spr->onscreen) ||
       (spr->ox == -1) )
     return;

  cr.x1 = spr->x;
  cr.y1 = spr->y;
  cr.x2 = spr->x+spr->w-1;
  cr.y2 = spr->y+spr->h-1;
   
  /* Protect that area of the screen */
  def_sprite_protectarea(&cr,spr->next);
   
  /* Put back the old image */
  (*vid->blit)(spr->backbuffer,0,0,
	       spr->ox,spr->oy,spr->ow,spr->oh,PG_LGOP_NONE);
  add_updarea(spr->ox,spr->oy,spr->ow,spr->oh);

  spr->onscreen = 0;
}

void def_sprite_update(struct sprite *spr) {
#ifdef DEBUG_VIDEO
   printf("def_sprite_update\n");
#endif
   
  (*vid->sprite_hide)(spr);
  (*vid->sprite_show)(spr);

  /* Redraw */
  realize_updareas();
}

/* Traverse first -> last, showing sprites */
void def_sprite_showall(void) {
  struct sprite *p = spritelist;

#ifdef DEBUG_VIDEO
   printf("def_sprite_showall\n");
#endif

   while (p) {
    (*vid->sprite_show)(p);
    p = p->next;
  }
}

/* Traverse last -> first, hiding sprites */
void r_spritehide(struct sprite *s) {
  if (!s) return;
  r_spritehide(s->next);
  (*vid->sprite_hide)(s);
}
void def_sprite_hideall(void) {
  r_spritehide(spritelist);
}

/* Hide necessary sprites in a given area */
void def_sprite_protectarea(struct cliprect *in,struct sprite *from) {
   /* Base case: from is null */
   if (!from) return;

#ifdef DEBUG_VIDEO
   printf("def_sprite_protectarea\n");
#endif

   /* Load this all on the stack so we go backwards */
   def_sprite_protectarea(in,from->next);
   
   /* Hide this sprite if necessary */
   if ( ((from->x+from->w) >= in->x1) &&
        ((from->y+from->h) >= in->y1) &&
        (from->x <= in->x2) &&
        (from->y <= in->y2) )
     (*vid->sprite_hide)(from);
}

/* Load our driver functions into a vidlib */
void setvbl_default(struct vidlib *vid) {
  /* Set defaults */
  vid->setmode = &def_setmode;
  vid->close = &emulate_dos;
  vid->color_pgtohwr = &def_color_pgtohwr;
  vid->color_hwrtopg = &def_color_hwrtopg;
  vid->font_newdesc = &def_font_newdesc;
  vid->addpixel = &def_addpixel;
  vid->subpixel = &def_subpixel;
  vid->clear = &def_clear;
  vid->update = &def_update;
  vid->slab = &def_slab;
  vid->bar = &def_bar;
  vid->line = &def_line;
  vid->rect = &def_rect;
  vid->gradient = &def_gradient;
  vid->dim = &def_dim;
  vid->scrollblit = &def_scrollblit;
  vid->charblit = &def_charblit;
  vid->charblit_v = &def_charblit_v;
  vid->tileblit = &def_tileblit;
  vid->bitmap_loadxbm = &def_bitmap_loadxbm;
  vid->bitmap_loadpnm = &def_bitmap_loadpnm;
  vid->bitmap_new = &def_bitmap_new;
  vid->bitmap_free = &def_bitmap_free;
  vid->bitmap_getsize = &def_bitmap_getsize;
  vid->sprite_show = &def_sprite_show;
  vid->sprite_hide = &def_sprite_hide;
  vid->sprite_update = &def_sprite_update;
  vid->sprite_showall = &def_sprite_showall;
  vid->sprite_hideall = &def_sprite_hideall;
  vid->sprite_protectarea = &def_sprite_protectarea;
  vid->blit = &def_blit;
  vid->unblit = &def_unblit;
}

/* The End */

