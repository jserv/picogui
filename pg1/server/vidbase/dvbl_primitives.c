/* $Id$
 *
 * dvbl_primitives.c - This file is part of the Default Video Base Library,
 *                     providing the basic video functionality in picogui but
 *                     easily overridden by drivers.
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
 * Shane Nay <shane@minirl.com>
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

#include <stdlib.h>		/* for qsort */
#include <string.h>
#ifdef _MSC_VER
#define _USE_MATH_DEFINES /* for M_PI */
#endif
#include <math.h>
#ifndef M_PI /* MingW doesn't define this by some reason */
#define M_PI       3.14159265358979323846
#endif


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

#define _ARC_DO_ROTATE \
  x_next = xc + (rot_cos * x_tmp) - (rot_sin * y_tmp); \
  y_next = yc + (rot_cos * y_tmp) + (rot_sin * x_tmp)

void def_arc(hwrbitmap dest, s16 x, s16 y, s16 w, s16 h,
	     s16 angle_start, s16 angle_stop, s16 angle_rot,
	     hwrcolor color, s16 lgop)
{
    double rad_start, rad_stop, rad_rot;
    double rot_cos, rot_sin;
    double aStep;            /* Angle step */
    double a;                /* Current angle */
    s16 xc, yc, radius1, radius2,
      x_last, x_tmp, x_next,
      y_last, y_tmp, y_next;

    /* This algorythm wants center and two radiuses... */
    xc = x+w/2;
    yc = y+h/2;
    radius1 = w/2;
    radius2 = h/2;
    /* ... and euclidian radians */
    rad_start = angle_start * M_PI / 180;
    rad_stop = angle_stop * M_PI / 180;
    rad_rot = -angle_rot * M_PI / 180;
    /* rotation multipliers */
    rot_cos = cos(rad_rot);
    rot_sin = sin(rad_rot);

    /* Angle step in rad */
    if (radius1<radius2) {
        if (radius1<1.0e-4) {
            aStep=1.0;
        } else {
            aStep=asin(2.0/radius1);
        }
    } else {
        if (radius2<1.0e-4) {
            aStep=1.0;
        } else {
            aStep=asin(2.0/radius2);
        }
    }

    if(aStep<0.05) {
        aStep = 0.05;
    }

    x_tmp = cos(rad_start)*radius1;
    y_tmp = sin(rad_start)*radius2;
    _ARC_DO_ROTATE;
    x_last = x_next;
    y_last = y_next;
    for(a=rad_start+aStep; a<=rad_stop; a+=aStep) {
      x_tmp = cos(a)*radius1;
      y_tmp = sin(a)*radius2;
      _ARC_DO_ROTATE;
      (*vid->line) (dest, x_last, y_last, x_next, y_next, color, lgop);
      x_last = x_next;
      y_last = y_next;
    }
}
#undef _ARC_DO_ROTATE

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
  s32 conda, condb, ddinc0, ddinc1; 
  /* Change following var's to long long if you want to draw *huge* ellipses */ 
  s32 dd, w22, h22, w2, h2; 
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
  dd=(h2-w2*h+h2)>>2; 
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

#if 1

struct poly_line_info {
  s16 x0,y0,x1,y1;  /* From to */
  s16 fraction; /* Derivative position */
  s16 dx,dy; /* Derivitive respect to.. */
  s16 stepx; /* Stepping, sy always==1 */
};

#define NEXT(X) ((X+1>=num_coords)?0:X+1)
#define PREV(X) ((X-1<0)?num_coords-1:X-1)

#define XAT(INDEX) (array[(INDEX<<1)+1])
#define YAT(INDEX) (array[(INDEX<<1)+2])

#define HLINE(A,B) ((YAT(A)==YAT(B))?1:0)

#define X0() (mptr->x0)
#define Y0() (mptr->y0) 
#define X1() (mptr->x1)
#define Y1() (mptr->y1)
#define FR() (mptr->fraction)
#define DX() (mptr->dx)
#define DY() (mptr->dy)  
#define SX() (mptr->stepx)


int polygon_simple_compare ( struct poly_line_info* a, struct poly_line_info* b )
{
    return b->y1 - a->y1;
}

void def_fpolygon (hwrbitmap dest, s32* array, s16 xoff, s16 yoff, hwrcolor c, s16 lgop) {
  /* Idea:
   * We run a series of scanlines traversing in the y direction,
   * calculating Bresenhams along the way for each polyline.
   * We only calculate the bresenhams for "presently active" polylines
   * and they act as "on/off" markers for horizontal lines
   * that will form the polygon.
   *
   * The first step is to create the polylines.  There is
   * a potential for N polylines where N is the number of vertices
   * in the polygon.  However, horizontal lines are ignored and subtracted
   * from the total number of polylines.
   *
   * Future optimizations:
   * 1) Move the horizontal/vertical logic into some goto label
   *    sort of thing to optimize code size.  (This *should* be
   *    done by the compiler, but I haven't checked the assembly
   *    output) Revision, I looked at the assembly output, and it
   *    isn't centralizing it.
   *    Down to 2.45k from 6k on Intel
   *
   */
  s16 num_coords=array[0]>>1;
  s16 nplines=num_coords; // First set nplines to max number
  s16 i,j,k,n,t,hi,lo,m;
  hi=lo=YAT(0);
  k=XAT(0);
  for(i=1;i<num_coords;i++) {
    j=YAT(i);
    m=XAT(i);
    if(j>hi)
      hi=j;
    else if(j<lo)
      lo=j;
    if(m<k)
      k=m;
    if(HLINE(i,NEXT(i))==1) {
      nplines--;
    }
  }
  xoff-=k;
  yoff-=lo;
  hi+=yoff;
  lo+=yoff;
  /* Now we know how many polylines we will need */
  {
    struct poly_line_info* mpoly = alloca(nplines*sizeof(struct poly_line_info)); /* Each of these has 16bytes worth of data */
    struct poly_line_info* mptr=mpoly;
    s16* vert = alloca(nplines*sizeof(s16));
    /* First stick info in the poly_lines in order */
    j=0;
    for(i=0;i<num_coords;i++) {
      k=NEXT(i);
      if((HLINE(i,k))!=1) {
	/* Do all the setup for the Bresenham for this line */
	m=i;
	if(YAT(k)<YAT(m)) {
	  m=k;
	  k=i;
	  n=NEXT(m);
	  t=PREV(k);
	} else {
	  n=PREV(m);
	  t=NEXT(k);
	}

	X0()=XAT(m)+xoff;
	Y0()=YAT(m)+yoff;
	X1()=XAT(k)+xoff;
	Y1()=YAT(k)+yoff;

	k=DX()=X1()-X0();
	DY()=Y1()-Y0(); /* Always positive */

	if(DX()<0) {
	  DX()=-(DX()<<1);
	  SX()=-1;
	} else {
	  DX()=DX()<<1;
	  SX()=1;
	}
	DY()=DY()<<1;

	FR()=(DX()>DY())? DY()-(DX()>>1) : -(DY()>>1) ;
	/* Remove horizontal odd parity x deltas for YO...
	   in this case, we remove our highest physically
	   point.  (Lowest numerical Y), We also need
	   to push the algorithm forward (FIXME)
	 */
	if((YAT(t)+yoff)>=Y1()) {
	  Y1()--;
	}
	if(HLINE(m,n)==1) {
	  n=XAT(n)+xoff-X0();
	  if((n | k)<0 && (n & k)>0) { /* DX Parity Calculation */
	    /* Increase Y max, effectively killing the top point. */
	    Y0()++;
	    if(DX()>DY()) { /* Horiz major axis */
	      while(1) {
		if(FR()>=0) {
		  FR()+=DY() - DX();
		  X0()+=SX();
		  break;
		}
		X0()+=SX();
		FR()+=DY();
	      }
	    } else { /* Vert major axis */
	      if(FR() >= 0) { 
		X0()+=SX();
		FR() -= DY();
	      }
	      FR()+=DX();
	    }
	  }
	}
      	mptr++;	
      }
    }
    mptr=mpoly;
    qsort(mpoly,nplines,sizeof(struct poly_line_info),(int(*)(const void*,const void*))&polygon_simple_compare);
    m=0;
    while(lo <= hi) {
      t=0;
      mptr=mpoly;
      for(i=0; i<nplines ;i++,mptr++) {
	/* Traverse the polylines */
	/* First conditional here skips the present line if it isn't
	 * relevant yet, or has passed its relevancy
	 */
	if(Y0()>lo) {
	  continue;
	}
	if(Y1()<lo) {
	  nplines--;
	  break;;
	}
	/* Now we calculate some Bresenhams, and add some points. */
	
	if(DX()>DY()) { /* Horiz major axis */
	  n=X0();
	  while(1) {
	    if(FR()>=0) {
	      if(SX()>0)
		n=X0();
	      Y0()++;
	      FR()+=DY() - DX();
	      X0()+=SX();
	      break;
	    }
	    X0()+=SX();
	    FR()+=DY();
	  }
	} else { /* Vert major axis */
	  if(FR() >= 0) { 
	    X0()+=SX();
	    FR() -= DY();
	  }
	  Y0()++;
	  FR()+=DX();
	  n=X0();
	}

	for(j=0;j<t;j++){
	  if(vert[j]>=n) {
	    for(k=t;k>=j;k--)
	      vert[k+1]=vert[k];
	    break;
	  }
	}
	t++;
	vert[j]=n;
      }
      t=t>>1;
      for(j=0;j<t;j++) {
	k=j<<1;
	(*vid->slab) (dest,vert[k],lo,vert[k+1]-vert[k]+1,c,lgop);
      }
      
      lo++;
    }
  }
}

#undef NEXT
#undef PREV
#undef XAT
#undef YAT
#undef HLINE
#undef X0
#undef Y0
#undef X1
#undef Y1
#undef FR
#undef DX
#undef DY
#undef SX

#else


struct poly_line_info {
  s16 x0,y0,x1,y1;  /* From to */
  s16 fraction; /* Derivative position */
  s16 dx,dy; /* Derivitive respect to.. */
  s16 stepx; /* Stepping, sy always==1 */
};

#define NEXT(X) ((X+1>=array[0])?0:X+1)
#define PREV(X) ((X-1<0)?array[0]-1:X-1)

#define XAT(INDEX) (array[(INDEX<<1)+1])
#define YAT(INDEX) (array[(INDEX<<1)+2])

#define HLINE(A,B) ((YAT(A)==YAT(B))?1:0)

#define X0() (mptr->x0)
#define Y0() (mptr->y0) 
#define X1() (mptr->x1)
#define Y1() (mptr->y1)
#define FR() (mptr->fraction)
#define DX() (mptr->dx)
#define DY() (mptr->dy)  
#define SX() (mptr->stepx)


int polygon_simple_compare ( struct poly_line_info* a, struct poly_line_info* b )
{
    return b->y1 - a->y1;
}

void def_fpolygon (hwrbitmap dest, s16* array, hwrcolor c, s16 lgop) {
  s16 i,j,k,n,t,hi,lo,m,x0,x1,y0,y1;
  hi=YAT(0);
  for(i=1;i<array[0];i++) {
    j=YAT(i);
    if(j>hi)
      hi=j;
  }
  /* If we have a trapezoid, it's always going to be to the
     next position.  Whether that is right or left horizontally
     has to be determined. */
  m=NEXT(hi);
  if(HLINE(hi,m)==1) {
    /* We've got a trapezoid */
    while(1) {
      k=NEXT(m);
      if(HLINE(m,k)==1) {/* Okay, it's valid, but the programmer is
			    a retard */
	for(i=m;i<array[0];i++) {
	  XAT(i)=XAT(i+1);
	  YAT(i)=YAT(i+1);
	}
	array[0]--;
	m=(k<m)?k:k-1;
      } else
	break;
    }
    
  } else {
  }
}

#endif 

/* The End */
