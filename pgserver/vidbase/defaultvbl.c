/* $Id: defaultvbl.c,v 1.53 2001/10/27 20:43:25 bornet Exp $
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
 * Shane Nay <shane@minirl.com>
 * 
 * 
 */

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/font.h>
#include <pgserver/render.h>
#include <pgserver/appmgr.h>    /* for defaultfont */

/******* Table of available bitmap formats */

struct bitformat bitmap_formats[] = {

#ifdef CONFIG_FORMAT_PNM
     { {'P','N','M',0}, &pnm_detect, &pnm_load, NULL },
#endif

#ifdef CONFIG_FORMAT_JPEG
     { {'J','P','E','G'}, &jpeg_detect, &jpeg_load, NULL },
#endif

#ifdef CONFIG_FORMAT_BMP
     { {'B','M','P',0}, &bmp_detect, &bmp_load, NULL },
#endif

     { {0,0,0,0}, NULL, NULL, NULL }
};

/******* no-op functions */

g_error def_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  return sucess;
}

void def_font_newdesc(struct fontdesc *fd) {
}

void emulate_dos(void) {
}

g_error def_enterexitmode(void) {
   return sucess;
}

void def_update(s16 x,s16 y,s16 w,s16 h) {
}

void def_coord_logicalize(s16 *x,s16 *y) {
}

void def_coord_keyrotate(s16 *k) {
}

/******* colors */

hwrcolor def_color_pgtohwr(pgcolor c) {
  if (vid->bpp==1) {
    /* Black and white, tweaked for better contrast */
    return (getred(c)+getgreen(c)+getblue(c)) >= 382;
  }
  else if(vid->bpp<8) {
    /* grayscale */
    return (getred(c)+getgreen(c)+getblue(c)) * ((1<<vid->bpp)-1) / 765;
  }
  else if (vid->bpp==8) {
    /* 2-3-3 color */
    return ((getred(c) & 0xC0) |
	    ((getgreen(c) >> 2) & 0x38) |
	    ((getblue(c) >> 5)));
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
  if (vid->bpp==1) {
    return c ? 0xFFFFFF : 0x000000;  
  }
  else if (vid->bpp<8) {
    /* grayscale */
    unsigned char gray = c * 255 / ((1<<vid->bpp)-1);
    return mkcolor(gray,gray,gray);
  }
  else if (vid->bpp==8) {
    /* 2-3-3 color */  
    u8 r = c&0xC0, g = (c&0x38)<<2, b = c<<5;
    return mkcolor(r | (r>>2) | (r>>4) | (r>>6),
		   g | (g>>3) | (g>>6),
		   b | (b>>3) | (b>>6));
  }
  else if (vid->bpp==16) {
     /* 5-6-5 color */
     return mkcolor( (c&0xF800) >> 8,
		     (c&0x07E0) >> 3,
		     (c&0x001F) << 3 );
  }
  else
    /* True color */
    return c;
}

/******* lgop */

void def_pixel(hwrbitmap dest, s16 x, s16 y, hwrcolor c, s16 lgop){
   switch (lgop) {
      
    case PG_LGOP_NONE:   /* Generic access to packed-pixel bitmaps */
	{
	   struct stdbitmap *bmp = (struct stdbitmap *) dest;
	   u8 *dst = bmp->bits + bmp->pitch*y + ((x*vid->bpp)>>3);
	   u8 shift,mask;
	   s16 subpixel,subpixel2;
	   switch (vid->bpp) {
	    case 1:
	    case 2:
	    case 4:
	      subpixel  = ((8/vid->bpp)-1);
	      subpixel2 = ((1<<vid->bpp)-1);
	      shift = (subpixel-(x&subpixel)) * vid->bpp;
	      mask  = subpixel2<<shift;
	      *dst &= ~mask;
	      *dst |= (c << shift) & mask;
	      break; 
	      
	    case 8:
	      *dst = c;
	      break;
	      
	    case 16:
	      *((u16*)dst) = c;
	      break;
	      
	    case 24:
	      *(dst++) = (u8) c;
	      *(dst++) = (u8) (c >> 8);
	      *dst     = (u8) (c >> 16);
	      break;
	      
	    case 32:
	      *((u32*)dst) = c;
	      break;     
	   }
	}
      break;

      /* LGOP implementations */

    case PG_LGOP_OR:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) | c,
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_AND:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) & c,
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_XOR:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) ^ c,
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_INVERT:
      (*vid->pixel)(dest,x,y,  
		    (c ^ (~((u32)0))),
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_INVERT_OR:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) | (c ^ (~((u32)0))),
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_INVERT_AND:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) & (c ^ (~((u32)0))),
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_INVERT_XOR:
      (*vid->pixel)(dest,x,y,  
		    (*vid->getpixel)(dest,x,y) ^ (c ^ (~((u32)0))),
		    PG_LGOP_NONE);
      break;
      
    case PG_LGOP_ADD:
	{
	   s16 r,g,b;
	   pgcolor oc = (*vid->color_hwrtopg) ((*vid->getpixel)(dest,x,y));
	   c = (*vid->color_hwrtopg) (c);
	   
	   r = getred(oc);
	   g = getgreen(oc);
	   b = getblue(oc);
	   r += getred(c);
	   g += getgreen(c);
	   b += getblue(c);
	   
	   if (r>255) r=255;
	   if (g>255) g=255;
	   if (b>255) b=255;
	   
	   (*vid->pixel) (dest,x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)),
			  PG_LGOP_NONE);
	}
      break;
      
    case PG_LGOP_SUBTRACT:
	{
	   s16 r,g,b;
	   pgcolor oc = (*vid->color_hwrtopg) ((*vid->getpixel)(dest,x,y));
	   c = (*vid->color_hwrtopg) (c);
	   
	   r = getred(oc);
	   g = getgreen(oc);
	   b = getblue(oc);
	   r -= getred(c);
	   g -= getgreen(c);
	   b -= getblue(c);
	   
	   if (r<0) r=0;
	   if (g<0) g=0;
	   if (b<0) b=0;
	   
	   (*vid->pixel) (dest,x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)),
			  PG_LGOP_NONE);
	}
      break;
      
    case PG_LGOP_MULTIPLY:
	{
	   s16 r,g,b;
	   pgcolor oc = (*vid->color_hwrtopg) ((*vid->getpixel)(dest,x,y));
	   c = (*vid->color_hwrtopg) (c);
	   
	   r = getred(oc) * getred(c) / 255;
	   g = getgreen(oc) * getgreen(c) / 255;
	   b = getblue(oc) * getblue(c) / 255;
	   
	   (*vid->pixel) (dest,x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)),
			  PG_LGOP_NONE);
	}
      break;

    case PG_LGOP_STIPPLE:
      if ((x+y)&1)
	(*vid->pixel) (dest,x,y,c,PG_LGOP_NONE);
      break;
      
   }
}
   
hwrcolor def_getpixel(hwrbitmap src, s16 x, s16 y) {
   struct stdbitmap *bmp = (struct stdbitmap *) src;
   u8 *s = bmp->bits + bmp->pitch*y + ((x*vid->bpp)>>3);
   u8 subpixel,shift;
   switch (vid->bpp) {
    case 1:
    case 2:
    case 4:
      subpixel  = ((8/vid->bpp)-1);
      shift = (subpixel-(x&subpixel)) * vid->bpp;
      return ((*s) >> shift) & ((1<<vid->bpp)-1);
      
    case 16:
      return *((u16*)s);
      
    case 24:
      return s[2] | (s[1]<<8) | (s[0]<<16);
      
    case 32:
      return *((u32*)s);
   }
   /* 8 */
   return *s;
}


/******* primitives */

void def_slab(hwrbitmap dest, s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
  /* You could make this create a very thin rectangle, but then if niether
   * were implemented they would be a pair of mutually recursive functions! */
   
  for (;w;w--,x++)
     (*vid->pixel) (dest,x,y,c,lgop);
}

void def_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
  (*vid->rect) (dest,x,y,1,h,c,lgop);
}

/* There are about a million ways to optimize this-
   At the very least, combine it with pixel() and
   have it keep track of the y coord as a memory offset
   to get rid of the multiply in pixel().

   The old SDL driver used this optimization, but it
   was too driver-specific, so had to revert to the
   generic bresenham's for the default implementation.
*/
void def_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c,s16 lgop) {
  s16 stepx, stepy;
  s16 dx = x2-x1;
  s16 dy = y2-y1;
  s16 fraction;

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

  (*vid->pixel) (dest,x1,y1,c,lgop);

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
      
      (*vid->pixel) (dest,x1,y1,c,lgop);
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
      
      (*vid->pixel) (dest,x1,y1,c,lgop);
    }
  }
}

void def_rect(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,hwrcolor c,s16 lgop) {
  for (;h;h--,y++) (*vid->slab) (dest,x,y,w,c,lgop);
}

#define SYMMETRY(X,Y) (*vid->pixel) (dest,xoff+X,yoff+Y,c,lgop); \
                      (*vid->pixel) (dest,xoff-X,yoff+Y,c,lgop); \
                      (*vid->pixel) (dest,xoff-X,yoff-Y,c,lgop); \
                      (*vid->pixel) (dest,xoff+X,yoff-Y,c,lgop)

void def_ellipse(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, hwrcolor c, s16 lgop) { 
  s16 xoff, yoff; 
  int w2, h2, S, T; 
  w=--w>>1; 
  h=--h>>1; 
  w2 = w*w; 
  h2 = h*h; 
  S = w2*(1-(h<<1)) + (h2<<1); 
  T = h2 - (w2*((h<<1)-1)<<1); 
  xoff=x+w; 
  yoff=y+h; 
  x=0; 
  y=h; 
  do 
    { 
      if (S<0) 
    { 
      S += h2*((x<<1)+3)<<1; 
      T += h2*(x+1)<<2; 
      x++; 
    } 
      else if (T<0) 
    { 
      S += (h2*((x<<1)+3)<<1) - (w2*(y-1)<<2); 
      T += (h2*(x+1)<<2) - (w2*((y<<1)-3)<<1); 
      x++; 
      y--; 
    } 
      else 
    { 
      S -= w2*(y-1)<<2; 
      T -= w2*((y<<1)-3)<<1; 
      y--; 
    } 
      SYMMETRY(x,y); 
 
    } 
  while (y>0); 
} 
#undef SYMMETRY 
 
 
#define SYMLINE(X,Y)  (*vid->slab) (dest,xoff-X,yoff+Y,(X<<1)+1,c,lgop); \
                      (*vid->slab) (dest,xoff-X,yoff-Y,(X<<1)+1,c,lgop)
/* De Silva elliptical drawing algorithm, with lots of other optimizations :) */ 
 
void def_fellipse(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h, hwrcolor c, s16 lgop) { 
  s16 xoff, yoff; 
  long int conda, condb, ddinc0, ddinc1; 
  /* Change following var's to long long if you want to draw *huge* ellipses */ 
  long int dd, w22, h22, w2, h2; 
  w=--w>>1; 
  h=--h>>1; 
  w2 = w*w; 
  h2 = h*h; 
  w22 = w2<<1; 
  h22 = h2<<1; 
  xoff=x+w; 
  yoff=y+h; 
  x=0; 
  y=h; 
  ddinc0=(h2<<1)+h2; 
  ddinc1=(2-(y<<1))*w2; 
  dd=h2-w2*h+h2>>2; 
  conda=w2*y-(w2>>1); 
  condb=h2; 
  while(conda>condb) { 
    if(dd>=0) { 
      dd+=ddinc1; 
      conda-=w2; 
      y--; 
      ddinc1+=w22; 
    } 
    dd+=ddinc0; 
    x++; 
    condb+=h2; 
    ddinc0+=h22; 
    SYMLINE(x,y); 
  } 
  if(h2 > 10000 && w2 > 10000) { /* Get around using long long */ 
    dd=(((h2>>6)*((x<<1)+1)*((x<<1)+1))>>2) + ((w2>>6)*(y-1)*(y-1) - (w2>>6)*h2); 
    dd=dd<<6; 
  } 
  else 
    dd=((h2*((x<<1)+1)*((x<<1)+1))>>2) + (w2*(y-1)*(y-1) - w2*h2); 
  ddinc0=w2*(3-(y<<1)); 
  ddinc1=h2*((x<<1)+2); 
  while (y>0) { 
    if(dd<0) { 
      dd += ddinc1; 
      x++; 
      ddinc1+=h22; 
    } 
    dd += ddinc0; 
    y--; 
    ddinc0+=w22; 
    SYMLINE(x,y); 
  } 
} 
#undef SYMLINE 
 
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

   for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++)
     for (x1=x,r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
	  i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc,x1++) {
	
	(*vid->pixel) (dest,x1,y,(*vid->color_pgtohwr)
		       (mkcolor(
				r_v1 + ((r_ca+r_sa) >> 8),
				g_v1 + ((g_ca+g_sa) >> 8),
				b_v1 + ((b_ca+b_sa) >> 8))),lgop);
     }
}

void def_charblit_0(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
		    s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,
		    s16 lgop) {
  int bw = w;
  int iw,hc,x;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || dest_y>clip->y2 || (dest_x+w)<clip->x1 || 
      (dest_y+h)<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y2<(dest_y+h))
      h = clip->y2-dest_y+1;
    if (clip->y1>dest_y) {
      hc = clip->y1-dest_y; /* Do it this way so skewing doesn't mess up when clipping */
      dest_y += hc;
      chardat += hc*bw;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->x1>dest_x)
      xmin = clip->x1-dest_x;
    if (clip->x2<(dest_x+w))
      xmax = clip->x2-dest_x+1;
  }

  for (;hc<h;hc++,dest_y++) {
    if (olines && lines==hc) {
      lines += olines;
      dest_x--;
      flag=1;
    }
    for (x=dest_x,iw=bw,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x++,xpix++) {
	 if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	   (*vid->pixel) (dest,x,dest_y,c,lgop);
      }
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
void def_charblit_90(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
		     s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,
		     s16 lgop) {
  int bw = w;
  int iw,hc,y;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || (dest_y-w)>clip->y2 || (dest_x+h)<clip->x1 || 
      dest_y<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->x2<(dest_x+h-1))
      h = clip->x2-dest_x+1;
    if (clip->x1>dest_x) {
      hc = clip->x1-dest_x; /* Do it this way so skewing doesn't mess up when clipping */
      dest_x += hc;
      chardat += hc*bw;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->y1>dest_y-w+1)
      xmax = 1+dest_y-clip->y1;
    if (clip->y2<(dest_y))
      xmin = dest_y-clip->y2;
  }

  for (;hc<h;hc++,dest_x++) {
    if (olines && lines==hc) {
      lines += olines;
      dest_y++;
      flag=1;
    }
    for (iw=bw,y=dest_y,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,y--,xpix++) {
	 if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	   (*vid->pixel) (dest,dest_x,y,c,lgop);
      }
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

void def_charblit_180(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
		      s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,
		      s16 lgop) {
  int bw = w;
  int iw,hc,x;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x<clip->x1 || dest_y<clip->y1 || (dest_x-w)>clip->x2 || 
      (dest_y-h)>clip->y2)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y1>(dest_y-h))
      h = dest_y-clip->y1+1;
    if (clip->y2<dest_y) {
      hc = dest_y-clip->y2; /* Do it this way so skewing doesn't mess up when clipping */
      dest_y -= hc;
      chardat += hc*bw;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->x2<dest_x)
      xmin = dest_x-clip->x2;
    if (clip->x1>(dest_x-w))
      xmax = dest_x-clip->x1+1;
  }

  for (;hc<h;hc++,dest_y--) {
    if (olines && lines==hc) {
      lines += olines;
      dest_x--;
      flag=1;
    }
    for (x=dest_x,iw=bw,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x--,xpix++) {
	 if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	   (*vid->pixel) (dest,x,dest_y,c,lgop); 
      }
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

void def_charblit_270(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
		     s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,
		     s16 lgop) {
  int bw = w;
  int iw,hc,y;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x<clip->x1 || (dest_y+w)<clip->y1 || (dest_x-h)>clip->x2 || 
      dest_y>clip->y2)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->x1>(dest_x-h-1))
      h = dest_x-clip->x1+1;
    if (clip->x2<dest_x) {
      hc = dest_x-clip->x2; /* Do it this way so skewing doesn't mess up when clipping */
      dest_x -= hc;
      chardat += hc*bw;
    }
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->y2<dest_y-w+1)
      xmax = 1+clip->y2-dest_y;
    if (clip->y1>(dest_y))
      xmin = clip->y1-dest_y;
  }

  for (;hc<h;hc++,dest_x--) {
    if (olines && lines==hc) {
      lines += olines;
      dest_y++;
      flag=1;
    }
    for (iw=bw,y=dest_y,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,y++,xpix++) {
	 if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	   (*vid->pixel) (dest,dest_x,y,c,lgop);
      }
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

/* A meta-charblit to select the appropriate function based on angle */
void def_charblit(hwrbitmap dest, u8 *chardat,s16 x,s16 y,s16 w,s16 h,
		  s16 lines, s16 angle, hwrcolor c, struct quad *clip,
		  s16 lgop) {

   void (*p)(hwrbitmap dest, u8 *chardat,s16 dest_x, s16 dest_y,
	     s16 w,s16 h,s16 lines, hwrcolor c,struct quad *clip,s16 lgop);
   
   switch (angle) {
    case 0:   p = &def_charblit_0;   break;
    case 90:  p = &def_charblit_90;  break;
    case 180: p = &def_charblit_180; break;
    case 270: p = &def_charblit_270; break;
    default:
      return;
   }

   (*p)(dest,chardat,x,y,w,h,lines,c,clip,lgop);
}

#ifdef CONFIG_FORMAT_XBM
g_error def_bitmap_loadxbm(hwrbitmap *bmp,const u8 *data, s16 w, s16 h,
			   hwrcolor fg, hwrcolor bg) {
   s16 i,bit,x,y;
   unsigned char c;
   g_error e;
   
   e = (*vid->bitmap_new)(bmp,w,h);
   errorcheck;
   
   /* Shift in the pixels! */
   for (y=0;h;h--,y++)
     for (x=0,bit=0,i=w;i>0;i--,x++) {
	if (!bit) c = *(data++);
	(*vid->pixel)(*bmp,x,y,(c&1) ? fg : bg,PG_LGOP_NONE);
	c >>= 1;
	bit++;
	bit &= 7;
     }
   return sucess;
}
#endif /* CONFIG_FORMAT_XBM */

/* Try loading a bitmap using each available format */
g_error def_bitmap_load(hwrbitmap *bmp,const u8 *data,u32 datalen) {
   struct bitformat *fmt = bitmap_formats;
   
   while (fmt->name[0]) {   /* Dummy record has empty name */
      if (fmt->detect && fmt->load && (*fmt->detect)(data,datalen))
	return (*fmt->load)(bmp,data,datalen);
      fmt++;
   }
   return mkerror(PG_ERRT_BADPARAM,8); /* Format not recognized by any loaders */
}

/* 90 degree anticlockwise rotation */
g_error def_bitmap_rotate90(hwrbitmap *b) {
   struct stdbitmap *destbit,*srcbit;
   u8 *src,*srcline,*dest;
   int oshift,shift,mask;
   int shiftset  = 8-vid->bpp;
   int subpixel  = ((8/vid->bpp)-1);
   int subpixel2 = ((1<<vid->bpp)-1);
   g_error e;
   int h,i,x,y;
   hwrcolor c;
   
   /* New bitmap with width/height reversed */
   srcbit = (struct stdbitmap *) (*b);
   e = (*vid->bitmap_new)(&destbit,srcbit->h,srcbit->w);
   errorcheck;
   
   src = srcline = srcbit->bits;
   for (h=srcbit->h,x=y=0;h;h--,y++,src=srcline+=srcbit->pitch) {

      /* Per-line mask calculations for <8bpp destination blits */
      if (vid->bpp<8) {
	 shift = (subpixel-(y&subpixel)) * vid->bpp;
	 mask  = subpixel2<<shift;
      }
      
      for (oshift=shiftset,i=srcbit->w,x=0;i;i--,x++) {
	 
	 /* Read in a pixel */
	 switch (vid->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    c = ((*src) >> oshift) & subpixel2;
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
	    c = *(((u16*)src)++);
	    break;
	    
	  case 24:
	    c = src[2] | (src[1]<<8) | (src[0]<<16);
	    src += 3;
	    break;
	    
	  case 32:
	    c = *(((u32*)src)++);
	    break;     
	 }
	 
	 /* Plot the pixel */
	 dest = destbit->bits + ((y*vid->bpp)>>3) + 
	   (srcbit->w-1-x)*destbit->pitch;
	 switch (vid->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    *dest &= ~mask;
	    *dest |= (c << shift) & mask;
	    break; 
	    
	  case 8:
	    *dest = c;
	    break;
	    
	  case 16:
	    *((u16*)dest) = c;
	    break;
	    
	  case 24:
	    *(dest++) = (u8) c;
	    *(dest++) = (u8) (c >> 8);
	    *dest     = (u8) (c >> 16);
	    break;
	    
	  case 32:
	    *((u32*)dest) = c;
	    break;     
	 }	
      }   
   }
   
   /* Clean up */
   *b = (hwrbitmap) destbit;
   (*vid->bitmap_free)(srcbit);
   return sucess;
}

g_error def_bitmap_new(hwrbitmap *b, s16 w,s16 h) {
  g_error e;
  struct stdbitmap **bmp = (struct stdbitmap **) b;
  int lw;
  u32 size;
   
  /* The bitmap and the header can be allocated seperately,
     but usually it is sufficient to make them one big
     chunk of memory.  It's 1 alloc vs 2.
  */

  /* Pad the line width up to the nearest byte */
  lw = (unsigned long) w * vid->bpp;
  if ((vid->bpp<8) && (lw & 7))
     lw += 8;
  lw >>= 3;

  /* The +1 is to make blits for < 8bpp simpler. Shouldn't really
   * be necessary, though. FIXME */
  size = sizeof(struct stdbitmap) + (lw * h) + 1;

  e = g_malloc((void **) bmp,size);
  errorcheck;
  memset(*bmp,0,size);
  (*bmp)->pitch = lw;
  (*bmp)->bits = ((unsigned char *)(*bmp)) + 
    sizeof(struct stdbitmap);
  (*bmp)->w = w;
  (*bmp)->h = h;
  
  return sucess;
}

void def_bitmap_free(struct stdbitmap *bmp) {
  if (!bmp) return;
  if (bmp->rend)
    g_free(bmp->rend);
  if (bmp->freebits)
    g_free(bmp->bits);
  g_free(bmp);
}

g_error def_bitmap_getsize(hwrbitmap b,s16 *w,s16 *h) {
   struct stdbitmap *bmp = (struct stdbitmap *) b;
   *w = bmp->w;
   *h = bmp->h;
   return sucess;
}

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

void def_tileblit(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
		  hwrbitmap src, s16 sx, s16 sy, s16 sw, s16 sh, s16 lgop) {
  s16 i,j;

  if (!(sw && sh)) return;
   
  /* Do a tiled blit */
  for (i=0;i<w;i+=sw)
     for (j=0;j<h;j+=sh)
       (*vid->blit) (dest,x+i,y+j,min(w-i,sw),min(h-j,h),
		     src,sx,sy,PG_LGOP_NONE);
}

/* Scary slow blit, but necessary for dealing with unsupported lgop values
 * or other things that the fast lib can't deal with */
void def_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop) {
   int i;
   struct stdbitmap *srcbit = (struct stdbitmap *) src;
   
   if (srcbit && (w>(srcbit->w-src_x) || h>(srcbit->h-src_y))) {
      int i,j,sx,sy;
      src_x %= srcbit->w;
      src_y %= srcbit->h;
      
      /* Do a tiled blit */
      for (i=0,sx=src_x;i<w;i+=srcbit->w-sx,sx=0)
	for (j=0,sy=src_y;j<h;j+=srcbit->h-sy,sy=0)
	  (*vid->blit) (dest,x+i,y+j,
			min(srcbit->w-sx,w-i),min(srcbit->h-sy,h-j),
			srcbit,sx,sy,lgop);
      return;
   }
   
   /* Icky blit loop */
   for (;h;h--,y++,src_y++)
     for (i=0;i<w;i++)
	(*vid->pixel) (dest,x+i,y,(*vid->getpixel)(src,src_x+i,src_y),lgop);
}

/* Copy it... Backwards! */
void def_scrollblit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
		    s16 src_x, s16 src_y, s16 lgop) {
  for (y+=h-1,src_y+=h-1;h;h--,y--,src_y--)
    (*vid->blit) (dest,x,y,w,1,src,src_x,src_y,lgop);
}

void def_sprite_show(struct sprite *spr) {
#ifdef DEBUG_VIDEO
   printf("def_sprite_show\n");
#endif
   
  if (spr->onscreen || !spr->visible || !vid->xres) return;
   
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
    else if (spr->x>(vid->lxres-1))
       spr->x = vid->lxres-1;
     
    if (spr->y<0)
       spr->y = 0;
    else if (spr->y>(vid->lyres-1))
       spr->y = vid->lyres-1;
    
    spr->ow = vid->lxres - spr->x;
    if (spr->ow > spr->w) spr->ow = spr->w;
    spr->oh = vid->lyres - spr->y;
    if (spr->oh > spr->h) spr->oh = spr->h;     
  }

  /* Update coordinates */
  spr->ox = spr->x; spr->oy = spr->y;
  
  /* Grab a new backbuffer */
  VID(blit) (spr->backbuffer,0,0,spr->ow,spr->oh,
	     vid->display,spr->x,spr->y,PG_LGOP_NONE);

  /* Display the sprite */
  if (spr->mask && *spr->mask) {
     VID(blit) (vid->display,spr->x,spr->y,spr->ow,spr->oh,
		*spr->mask,0,0,PG_LGOP_AND);
     VID(blit) (vid->display,spr->x,spr->y,spr->ow,spr->oh,
		*spr->bitmap,0,0,PG_LGOP_OR);
  }
   else
     VID(blit) (vid->display,spr->x,spr->y,spr->ow,spr->oh,
		*spr->bitmap,0,0,PG_LGOP_NONE);
   
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
	g.param[0] = (*vid->color_pgtohwr) (0xFFFF00);
	VID(rect) (d.x,d.y,d.w,d.h,(*vid->color_pgtohwr)(0x004000));
	g.x = spr->x-d.x;
	g.y = spr->y-d.y;
	for (i=0;i<8;i++) {
	   g.x += g.w; 
	   g.y += g.h;
	   g.w = xp[i];
	   g.h = yp[i];
	   grop_render(&d);
	}
	VID(update) (d.x,d.y,d.w,d.h);
     }
*/

   /**** A very similar debuggative cruft to test text clipping ****/
  /*
    {
      struct quad cr;
      struct fontdesc fd;

      memset(&fd,0,sizeof(fd));
      fd.fs = fontstyles;
      fd.font = fd.fs->normal;
      fd.hline = -1;
 
      cr.x1 = 100;
      cr.y1 = 100;
      cr.x2 = 150;
      cr.y2 = 110;
      VID(rect) (vid->display,cr.x1,cr.y1,cr.x2-cr.x1+1,cr.y2-cr.y1+1,(*vid->color_pgtohwr)(0x004000),PG_LGOP_NONE);
      outtext(vid->display,&fd,spr->x,spr->y,(*vid->color_pgtohwr) (0xFFFF80),"Hello,\nWorld!",&cr,0,0,PG_LGOP_NONE,0);
      //      outtext_v(&fd,spr->x,spr->y,(*vid->color_pgtohwr) (0xFFFF80),"Hello,\nWorld!",&cr);
//      outtext_v(&fd,spr->x,spr->y,(*vid->color_pgtohwr) (0xFFFF80),"E",&cr);
      VID(update) (0,0,vid->lxres,vid->lyres);
    }
  */
   
}

void def_sprite_hide(struct sprite *spr) {
  static struct quad cr;

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
  VID(blit) (vid->display,spr->ox,spr->oy,spr->ow,spr->oh,
	     spr->backbuffer,0,0,PG_LGOP_NONE);
  add_updarea(spr->ox,spr->oy,spr->ow,spr->oh);

  spr->onscreen = 0;
}

void def_sprite_update(struct sprite *spr) {
#ifdef DEBUG_VIDEO
   printf("def_sprite_update\n");
#endif
   
  (*vid->sprite_hide) (spr);
  (*vid->sprite_show) (spr);

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
    (*vid->sprite_show) (p);
    p = p->next;
  }
}

/* Traverse last -> first, hiding sprites */
void r_spritehide(struct sprite *s) {
  if (!s) return;
  r_spritehide(s->next);
  (*vid->sprite_hide) (s);
}
void def_sprite_hideall(void) {
  r_spritehide(spritelist);
}

/* Hide necessary sprites in a given area */
void def_sprite_protectarea(struct quad *in,struct sprite *from) {
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
     (*vid->sprite_hide) (from);
}

/* Optional
 *   This is called for every bitmap when entering a new bpp or loading
 *   a new driver. Converts a bitmap from a linear array of 32-bit
 *   pgcolor values to the hwrcolors for this mode
 *
 * Default implementation: stdbitmap
 */
g_error def_bitmap_modeconvert(struct stdbitmap **bmp) {
   struct stdbitmap *destbit,*srcbit;
   u32 *src;
   u8 *destline,*dest;
   int oshift;
   int shiftset  = 8-vid->bpp;
   g_error e;
   int h,i,x,y;
   hwrcolor c;
   
   /* New bitmap at our present bpp */
   srcbit = *bmp;
   e = (*vid->bitmap_new)(&destbit,srcbit->w,srcbit->h);
   errorcheck;

   src = (u32 *) srcbit->bits;
   dest = destline = destbit->bits;
   
   for (h=srcbit->h,x=y=0;h;h--,y++,dest=destline+=destbit->pitch) {      
      for (oshift=shiftset,i=srcbit->w,x=0;i;i--,x++) {	 
	 /* Read in a pixel */
	 c = (*vid->color_pgtohwr)(*(src++));
	 
	 /* Output in new device bpp */
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
	    *(((unsigned char *)dest)++) = c;
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
   
   /* Clean up */
   *bmp = destbit;
   (*vid->bitmap_free)(srcbit);
   return sucess;
}
   
/* Optional
 *   The reverse of bitmap_modeconvert, this converts the bitmap from
 *    the hardware-specific format to a pgcolor array
 * 
 * Default implementation: stdbitmap
 */
g_error def_bitmap_modeunconvert(struct stdbitmap **bmp) {
   struct stdbitmap *destbit,*srcbit;
   u8 *src,*srcline;
   u32 *dest;  /* Must be word-aligned, this is always the case if
		* struct stdbitmap is padded correctly */
   int oshift;
   int shiftset  = 8-vid->bpp;
   g_error e;
   int h,i,x,y;
   hwrcolor c;
   
   /* New bitmap at 32bpp (this is hackish, but I don't see anything
    * seriously wrong with it... yet...) 
    */
   srcbit = *bmp;
   i = vid->bpp;
   vid->bpp = 32;
   e = (*vid->bitmap_new)(&destbit,srcbit->w,srcbit->h);
   vid->bpp = i;
   errorcheck;
   
   src = srcline = srcbit->bits;
   dest = (u32 *) destbit->bits;
   
   for (h=srcbit->h,x=y=0;h;h--,y++,src=srcline+=srcbit->pitch) {      
      for (oshift=shiftset,i=srcbit->w,x=0;i;i--,x++) {
	 
	 /* Read in a pixel */
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
	    c = *(((u16*)src)++);
	    break;
	    
	  case 24:
	    c = (*(src++)) | ((*(src++))<<8) | ((*(src++))<<16);
	    break;
	    
	  case 32:
	    c = *(((u32*)src)++);
	    break;     
	 }
	 
	 *(dest++) = (*vid->color_hwrtopg)(c);
      }   
   }
   
   /* Clean up */
   *bmp = destbit;
   (*vid->bitmap_free)(srcbit);
   return sucess;
}

g_error def_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend) {
  struct stdbitmap *b = (struct stdbitmap *) bmp;
  g_error e;

  if (b->rend) {
    *rend = b->rend;
    return;
  }

  /* ack... we need to make a new context */

  e = g_malloc((void **) rend,sizeof(struct groprender));
  errorcheck;
  b->rend = *rend;
  memset(*rend,0,sizeof(struct groprender));
  (*rend)->lgop = PG_LGOP_NONE;
  (*rend)->output = bmp;
  (*rend)->hfont = defaultfont;
  (*rend)->clip.x2 = b->w - 1;
  (*rend)->clip.y2 = b->h - 1;
  (*rend)->orig_clip = (*rend)->clip;
  (*rend)->output_rect.w = b->w;
  (*rend)->output_rect.h = b->h;

  return sucess;
}
   
/* Load our driver functions into a vidlib */
void setvbl_default(struct vidlib *vid) {
  /* Set defaults */
  vid->color_pgtohwr = &def_color_pgtohwr;
  vid->color_hwrtopg = &def_color_hwrtopg;
  vid->font_newdesc = &def_font_newdesc;
  vid->slab = &def_slab;
  vid->bar = &def_bar;
  vid->line = &def_line;
  vid->rect = &def_rect;
  vid->gradient = &def_gradient;
  vid->charblit = &def_charblit;
  vid->tileblit = &def_tileblit;
  vid->scrollblit = &def_scrollblit;
  vid->ellipse = &def_ellipse; 
  vid->fellipse = &def_fellipse; 
#ifdef CONFIG_FORMAT_XBM
  vid->bitmap_loadxbm = &def_bitmap_loadxbm;
#endif
  vid->bitmap_load = &def_bitmap_load;
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
  vid->coord_logicalize = &def_coord_logicalize;
  vid->coord_physicalize = &def_coord_logicalize;
  vid->bitmap_rotate90 = &def_bitmap_rotate90;
  vid->entermode = &def_enterexitmode;
  vid->exitmode = &def_enterexitmode;
  vid->bitmap_modeconvert = &def_bitmap_modeconvert;
  vid->bitmap_modeunconvert = &def_bitmap_modeunconvert;
  vid->bitmap_get_groprender = &def_bitmap_get_groprender;
  vid->coord_keyrotate = &def_coord_keyrotate;
}

/* The End */

