/* $Id: null.c 3978 2003-05-23 10:19:38Z micah $
 *
 * directfb.c - Video driver for DirectFB
 *              see http://directfb.org/
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
 * Lalo Martins <lalo@laranja.org> - initial implementation (11 2003)
 * 
 * 
 */

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/input.h>       /* For loading our corresponding input lib */
#include <pgserver/configfile.h>  /* For loading our configuration */

#include <stdio.h>
#include <unistd.h>

#include <directfb.h>
#include <directfb-internal/gfx/convert.h> /* for pixel format conversions */

// FIXME: we probably shouldn't die on all errors (hence the name)
/* (should probably move to a directfb.h? */
#define dfb_errcheck_die                                       \
  {                                                            \
    if (err != DFB_OK)                                         \
      {                                                        \
        fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
        DirectFBErrorFatal( "", err );                         \
      }                                                        \
  }

#define DrawPixel(surface, x, y)                               \
   surface->DrawLine (surface, x, y, x, y);                    \

#define h_getalpha(pgc)     (((pgc)>>24)&0xFF)
/******************************************** Implementations */

IDirectFB *directfb = NULL;
static IDirectFBSurface *primary = NULL;

g_error directfb_init(void) {
   DFBResult err;
   DFBSurfaceDescription dsc;
   err = DirectFBInit(NULL, NULL); /* no argc/argv */
   dfb_errcheck_die;
   err = DirectFBCreate (&directfb);
   dfb_errcheck_die;
   //directfb->SetCooperativeLevel (directfb, DFSCL_FULLSCREEN);
   dsc.flags = DSDESC_CAPS;
   dsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
   err = directfb->CreateSurface(directfb, &dsc, &primary);
   dfb_errcheck_die;
   err = primary->GetSize (primary, &vid->xres, &vid->yres);
   dfb_errcheck_die;
   vid->display = NULL;
   // FIXME
   if (!vid->bpp) vid->bpp = 32;

#ifdef DRIVER_DIRECTFBINPUT
   /* Load a main input driver */
   return load_inlib(&directfbinput_regfunc,&inlib_main);
#else
   return success;
#endif
}

g_error directfb_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
   /* no-op for now
   vid->xres = xres;
   vid->yres = yres;
   vid->bpp  = bpp;
   vid->display = NULL;
   */
   return success;
}

void directfb_close(void) {
  primary->Release(primary);
  directfb->Release(directfb);
}

void directfb_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
   DFBResult err;
   DFBSurfaceDrawingFlags flags;
   if (dest) {
      def_pixel(dest,x,y,c,lgop);
      return;
   }
#ifdef DEBUG_VIDEO
   if (!directfb)
     printf("Directfb driver: pixel at %d,%d set when uninitialized\n",x,y);
   if (x<0 || y<0 || (x>=vid->xres) || (y>=vid->yres))
     printf("Directfb driver: pixel out of bounds at %d,%d\n",x,y);
#endif
   err = primary->SetColor (primary, getred(c), getgreen(c), getblue(c), h_getalpha(c));
   dfb_errcheck_die;
   // FIXME: should support all lgops
   switch (lgop) {
   case PG_LGOP_NONE:
     flags = DSDRAW_NOFX;
     break;
   case PG_LGOP_ALPHA:
     flags = DSDRAW_BLEND;
     break;
   default:
     flags = DSDRAW_NOFX;
     printf("ignoring lgop: %d\n", lgop);
   }
   err = primary->SetDrawingFlags (primary, flags);
   dfb_errcheck_die;
   err = DrawPixel (primary, x, y);
   dfb_errcheck_die;
}

hwrcolor directfb_getpixel(hwrbitmap src,s16 x,s16 y) {
   DFBResult err;
   DFBSurfacePixelFormat format;
   void                  *data;
   int                   pitch;
   hwrcolor              c;

   if (src)
     return def_getpixel(src,x,y);
#ifdef DEBUG_VIDEO
   if (!directfb)
     printf("Directfb driver: pixel at %d,%d get when uninitialized\n",x,y);
   if (x<0 || y<0 || (x>=vid->xres) || (y>=vid->yres))
      printf("Directfb driver: getpixel out of bounds at %d,%d\n",x,y);
#endif

   primary->GetPixelFormat(primary, &format);
   err = primary->Lock(primary, DSLF_READ, &data, &pitch);

   switch (format) {
   case DSPF_RGB16:
     {
       __u16 *dst = data + y * pitch;
       c = RGB16_TO_ARGB(dst[x]);
       break;
     }
   case DSPF_ARGB1555:
     {
       __u16 *dst = data + y * pitch;
       c = ARGB1555_TO_ARGB(dst[x]);
       break;
     }
   case DSPF_ARGB:
     {
       __u32 *dst = data + y * pitch;
       c = dst[x];
       break;
     }
   case DSPF_RGB32:
     {
       __u32 *dst = data + y * pitch;
       c = RGB32_TO_ARGB(dst[x]);
       break;
     }
   default:
     fprintf( stderr, "Unhandled pixel format 0x%08x!\n", format );
     c = 0;
     break;
   }

   primary->Unlock(primary);

   return c;
}

void directfb_update(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h) {
   DFBResult err;
   err = primary->Flip (primary, NULL, DSFLIP_BLIT);
   dfb_errcheck_die;
}

hwrcolor directfb_color_pgtohwr(pgcolor c) {
  u8 alpha;
  /* DirectFB API is 32-bit RGBA, so the pgcolor format is almost good enough */
  if (c | PGCF_ALPHA) {
    alpha = getalpha(c);
    c &= 0x00FFFFFF;
    c |= (alpha << 1);
  }
  else
    c |= 0xFF000000;
  return c;
}

pgcolor directfb_color_hwrtopg(hwrcolor c) {
  u8 alpha = h_getalpha(c);
  /* DirectFB API is 32-bit RGBA, so the pgcolor format is almost good enough */
  c &= 0x00FFFFFF; /* reset flags and alpha */
  if (alpha != 0xff)
    c |= ((alpha >> 1) | PGCF_ALPHA);
  return c;
}

#ifdef CONFIG_FONTENGINE_FREETYPE
void directfb_alpha_charblit(hwrbitmap dest, u8 *chardat, s16 x, s16 y, s16 w, s16 h,
			     int char_pitch, u8 *gammatable, s16 angle, hwrcolor c,
			     struct pgquad *clip, s16 lgop) {
  int i,j,xp,yp;
  u8 *l;
  u8 red, green, blue, alpha;
  DFBResult err;
  DFBSurfaceDrawingFlags flags;

  if (dest) {
    def_alpha_charblit(dest, chardat, x, y, w, h, char_pitch, gammatable, angle, c, clip, lgop);
    return;
  }

  // FIXME: we're effectively ignoring the lgop
  flags = DSDRAW_BLEND;
  err = primary->SetDrawingFlags (primary, flags);
  dfb_errcheck_die;

  /* little stupid optimization */
  red = getred(c); green = getgreen(c); blue = getblue(c);

  for (j=0;j<h;j++,chardat+=char_pitch)
    for (i=0,l=chardat;i<w;i++,l++) {

      switch (angle) {
      case 0:
	xp = x+i;
	yp = y+j;
	break;
      case 90:
	xp = x+j;
	yp = y-i;
	break;
      case 180:
	xp = x-i;
	yp = y-j;
	break;
      case 270:
	xp = x-j;
	yp = y+i;
	break;
      }

      if (clip)
	if (xp < clip->x1 || xp > clip->x2 ||
	    yp < clip->y1 || yp > clip->y2)
	  continue;
      
      if (gammatable)
	alpha = gammatable[*l];
      else
	alpha = *l;

      err = primary->SetColor (primary, red, green, blue, alpha);
      dfb_errcheck_die;
      err = DrawPixel (primary, xp, yp);
      dfb_errcheck_die;
    }
}
#endif /* CONFIG_FONTENGINE_FREETYPE */

/******************************************** Driver registration */

g_error directfb_regfunc(struct vidlib *v) {
   setvbl_default(v);
   
   v->init = &directfb_init;
   v->setmode = &directfb_setmode;
   v->close = &directfb_close;
   v->pixel = &directfb_pixel;
   v->getpixel = &directfb_getpixel;
   v->update = &directfb_update;
   v->color_pgtohwr = &directfb_color_pgtohwr;
   v->color_hwrtopg = &directfb_color_hwrtopg;
#ifdef CONFIG_FONTENGINE_FREETYPE
   v->alpha_charblit = &directfb_alpha_charblit;
#endif /* CONFIG_FONTENGINE_FREETYPE */
   return success;
}

/* The End */
