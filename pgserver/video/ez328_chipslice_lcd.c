/* $Id: ez328_chipslice_lcd.c,v 1.6 2001/03/17 04:16:36 micahjd Exp $
 *
 * ez328_chipslice_lcd.c
 *           LCD video drivers for the DragonBall-based ChipSlice.
 *
 *           Most parts are M68EZ328-standard, some few parts such as
 *           GPIO port mapping are specific to the plateform and/or the
 *           LCD-Panel.
 *
 *           This file is used if DRIVER_EZ328_CHIPSLICE is defined
 *           To use a currently supported panel, define one of:
 *             DRIVER_EZ328_CHIPSLICE_V0_2_CITIZEN_G3243H
 *
 *           Status: under work
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
 *   Pascal Bauermeister <pascal.bauermeister@smartdata.ch> :
 *   initial version
 * 
 */

#include <pgserver/common.h>

/* Drivers for LCD panels of the CHIPSLICE platforms */
#ifdef DRIVER_EZ328_CHIPSLICE

#include <pgserver/video.h>
#include <pgserver/input.h>

#include "ez328_chipslice_lcd.h"

#ifdef DRIVER_EZ328_CHIPSLICE_V0_2_CITIZEN_G3243H
# define PHY_WIDTH  320
# define PHY_HEIGHT 240
# define PHY_BPP      2

# define _ADJ_C(x,sh)    (((x)>>sh) & 0x3f)
# define _ADJ_SUM(x)     (_ADJ_C(x,26)+_ADJ_C(x,18)+_ADJ_C(x,2))
# define ADJUST_COLOR(x) _ADJ_VAL[_ADJ_SUM(x)>>4]
#endif

static const unsigned _ADJ_VAL[12] = {
  0,0,0,
  1,1,1,
  2,2,2,
  3,3,3
};

/******************************************** Vars */

/* "dirty" area */
static int lcd_upd_x = 0;
static int lcd_upd_y = 0;
static int lcd_upd_w = 0;
static int lcd_upd_h = 0;

/* memory shadow */
typedef unsigned char shd_pixel;
static shd_pixel*     shd_buffer = 0;
static unsigned char* phy_buffer = 0;
#define SHD_BUFFER_SIZE (PHY_WIDTH*PHY_HEIGHT)

/******************************************** Low-level stuff */

#if defined(DRIVER_EZ328_CHIPSLICE_V0_2_CITIZEN_G3243H)

#define PHY_INIT(bpp)            phy_init(bpp)
#define PHY_UNINIT()             phy_uninit()
#define PHY_UPDATE(src,x,y,w,h)  phy_update(src,x,y,w,h)
#define PHY_BUFFER_SIZE          (PHY_WIDTH*PHY_HEIGHT*PHY_BPP*sizeof(shd_pixel)/8)

g_error phy_init(int bpp)
{
  g_error e;

  /* we currently support only static bpp */
  if(bpp!=PHY_BPP)
    return mkerror(PG_ERRT_BADPARAM,48);

  /* we allocate mem for the controller */
  e = g_malloc((void **)&phy_buffer, PHY_BUFFER_SIZE);
  if(iserror(e)) {
    return mkerror(PG_ERRT_MEMORY,49);
  }
  memset(phy_buffer, 0, PHY_BUFFER_SIZE);

  LSSA   = phy_buffer;
  LXMAX  = PHY_WIDTH;
  LYMAX  = PHY_HEIGHT;
  LVPW   = 20;
  LPXCD  = 2;
  LACD   = 0x80;
  LPOLCF = 0x00;
  LCXP   = 0xc000;
  LCYP   = 0x0000;
  LCWCH  = 0x0000;
  LBLKC  = 0x9f;
  LPICF  = 0x09;

  LRRA   = 10;
  LPOSR  = 0x00;
  LFRCM  = 0xb9;
  LGPMR  = 0x84;
  PWM    = 0x0000;
  LPICF  = 0x05;

  /* Enable LCD */
  LCKCON = 0x82;
  LCDENABLE;
}



void phy_uninit(void)
{
  /* Disable LCD */
  LCDDISABLE;
  LCKCON = 0x00;

  g_free(phy_buffer);
  phy_buffer = 0;
}


void phy_update(shd_pixel* src,int x, int y, int w, int h)
{
  int delta = sizeof(shd_pixel)*PHY_WIDTH-w;
  int xx, yy=y, ww;
  static const unsigned char MSK[1<<PHY_BPP] = { 0xc0, 0x30, 0x0c, 0x03 };

  src += y*w + x;


#if PHY_BPP==2
  for(; h; --h, ++yy) {
    for(xx=x, ww=w; ww; --ww, ++xx) {
      shd_pixel pix = *src;
      int ix = (y*PHY_WIDTH+x);
      int ip = ix>>PHY_BPP;
      int ib = ix & ((1<<PHY_BPP)-1);
      unsigned char msk = MSK[ib];
      phy_buffer[ix] &= ~msk;
      phy_buffer[ix] &= (pix>>(8-PHY_BPP)) << (ib<<1);
      ++src;
    }
    src += delta;
  }

#else
# error Currently only BPP=2 is supported
#endif
}



#else
# error you specified no valid driver of the DRIVER_EZ328_CHIPSLICE family
#endif

/******************************************** Utils */

/* Assimilates the given area into the update rectangle */
static void lcd_addarea(int x,int y,int w,int h) {
  if (lcd_upd_w) {
    if (x < lcd_upd_x) {
      lcd_upd_w += lcd_upd_x - x;
      lcd_upd_x = x;
    }
    if (y < lcd_upd_y) {
      lcd_upd_h += lcd_upd_y - y;
      lcd_upd_y = y;
    }
    if ((w+x) > (lcd_upd_x+lcd_upd_w))
      lcd_upd_w = w+x-lcd_upd_x;
    if ((h+y) > (lcd_upd_y+lcd_upd_h))
      lcd_upd_h = h+y-lcd_upd_y;
  }
  else {
    lcd_upd_x = x;
    lcd_upd_y = y;
    lcd_upd_w = w;
    lcd_upd_h = h;
  }
}

/******************************************** Implementations */

static void lcd_close(void) {
  PHY_UNINIT();
  g_free(shd_buffer);
  shd_buffer = 0;

}

static g_error lcd_init(int xres,int yres,int bpp,unsigned long flags) {
  g_error e;

  memset(&shd_buffer, 0, SHD_BUFFER_SIZE);
  lcd_addarea(0, 0, PHY_WIDTH, PHY_HEIGHT);

  e = PHY_INIT(2);
  if(iserror(e)) {
    lcd_close();
    return mkerror(PG_ERRT_IO,47);
  }

  vid->xres = PHY_WIDTH;
  vid->yres = PHY_HEIGHT;
  vid->bpp  = 8; /* logically 8 bpp, physically 2 bpp */

  /* set clip to full screen */
  VID(clip_off) ();

  return sucess;
}


static void lcd_setpixel(int x,int y,hwrcolor c) {
  unsigned long *p;

  if (x<vid->clip_x1 || x>vid->clip_x2 ||
      y<vid->clip_y1 || y>vid->clip_y2)
    return;

  lcd_addarea(x,y,1,1);

  shd_buffer[vid->xres*y+x] = ADJUST_COLOR(c);
}

static hwrcolor lcd_getpixel(int x,int y) {
  return 0;
}

static void lcd_update(void) {
}

static void lcd_blit(struct stdbitmap *src,int src_x,int src_y,
		     struct stdbitmap *dest,int dest_x,int dest_y,
		     int w,int h,int lgop) {
}

static void lcd_clip_set(int x1,int y1,int x2,int y2) {
}

static void lcd_rect(int x,int y,int w,int h,hwrcolor c) {
}

static hwrcolor lcd_color_pgtohwr(pgcolor c) {
  return 0;
}

static pgcolor lcd_color_hwrtopg(hwrcolor c) {
  return 0;
}


/* Eek!  See the def_gradient for more comments.  This one is just
   reworked for the LCD driver
*/
static void lcd_gradient(int x,int y,int w,int h,int angle,
                  pgcolor c1,pgcolor c2,int translucent) {
}

/******************************************** Driver registration */

g_error chipslice_video_regfunc(struct vidlib *v)
{
  g_error e;

  setvbl_default(v);

  /* we allocate mem now, not in lcd_init() */
  e = g_malloc((void **)&shd_buffer, SHD_BUFFER_SIZE);
  errorcheck;
  memset(&shd_buffer, 0, SHD_BUFFER_SIZE);

  v->init          = &lcd_init;
  v->close         = &lcd_close;
  v->pixel         = &lcd_setpixel;
  v->getpixel      = &lcd_getpixel;
  v->update        = &lcd_update;
  v->blit          = &lcd_blit;
  v->clip_set      = &lcd_clip_set;

  v->rect          = &lcd_rect;
  v->color_pgtohwr = &lcd_color_pgtohwr;
  v->color_hwrtopg = &lcd_color_hwrtopg;
  v->gradient      = &lcd_gradient;

  return sucess;
}


/******************************************** Test */

static void test()
{
  g_error e;

  struct vidlib v;

  e = chipslice_video_regfunc(&v);
  if(!iserror(e)) {
    puts("*** init:");
    (*v.init)(0,0,0,0);

    puts("*** rect 1:");
    (*v.rect)(0,0,50,50, mkcolor(0,0,0));

    puts("*** rect 2:");
    (*v.rect)(50,50,50,50, mkcolor(85,85,85));

    puts("*** close:");
    (*v.close)();
  }

}


#endif /* DRIVER_EZ328_CHIPSLICE */
/* The End */
