/* $Id$
 *
 * ez328.c - Driver for the 68EZ328's (aka Motorola Dragonball EZ)
 *           built-in LCD controller. It assumes the LCD parameters
 *           have been set at boot in the kernel (for the boot logo)
 *           and uses them to set up the driver.
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
#include <pgserver/input.h>

#include <asm/MC68VZ328.h>   /* Defines the CPU and peripheral's registers */

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

/* Save all LCD registers, restore on exit */
#define REGS_LEN     0x36
#define REGS_START   ((void*)LSSA_ADDR)
u8 ez328_saveregs[REGS_LEN];

g_error ez328_init(void);
g_error ez328_setmode(s16 xres,s16 yres,s16 bpp,u32 flags);
void ez328_close(void);
g_error ez328_regfunc(struct vidlib *v);

g_error ez328_init(void) {
#ifdef CONFIG_CHIPSLICE

# if 0
  LCKCON = 0;     /* LCKCON - LCD is off */
  LVPW   = 0x50;
  LXMAX  = 0x140;
  LYMAX  = 0x0EF;  /* because height of screen is 'LYMAX+1' see DB specs */
  LRRA   = 0;
  LPXCD  = 0;
  LPICF  = 0x0A;   /* =1010 = 16 grey 4 bits */
  LPOLCF = 0x01;   /* 1 -> inverse video */

  LCKCON = 0x81;
  //PCPDEN = 0xff00;     /* LCD pins */
  PCPDEN = 0xff;      //NPH - to disable pull-down what is wanted ??
# endif

  LCKCON &= ~LCKCON_LCDON;  /* LCKCON - LCD is off */
  LXMAX  = 320;             /* width of the screen = 0x00140 */
  LYMAX  = 240 - 1;         /* height of screen is 'LYMAX+1' see DB specs */
  LRRA   = 0;
  LPXCD  = 0;
  LPICF  = 0x0A;            /* =1010 = 16 grey 4 bits */
  LPOLCF = 0x01;            /* 1 -> inverse video */

  PCPDEN |= 0xFF;           /* enable pull-down */
  PCSEL  &= 0x00;           /* PC as dedicated */
  LCKCON |= LCKCON_LCDON;   /* LCKCON - LCD is on */

  /* Save existing register settings */
  memcpy(ez328_saveregs,REGS_START,REGS_LEN);

#elif defined(CONFIG_XCOPILOT)
  LXMAX  = 160;
  LYMAX  = 160-1;
  vid->bpp = vid->bpp<=2 ? vid->bpp : 1;
#elif defined(CONFIG_SOFT_CHIPSLICE)
  LXMAX  = 240;
  LYMAX  = 320-1;
  if (!vid->bpp) vid->bpp = 1;
  vid->bpp = vid->bpp<=2 ? vid->bpp : 1;
#endif
   
  if (!vid->bpp) vid->bpp = 1;        /* Default to black and white */
   
#if defined(CONFIG_XCOPILOT)
  /* Load the ts driver as the main input driver */
  return load_inlib(&chipslicets_regfunc,&inlib_main);
#else
  return success;
#endif
}
   
g_error ez328_setmode(s16 xres,s16 yres,s16 bpp,u32 flags) {
  g_error e;
   
  /* bpp-specific setup. Load the appropriate VBL and set the controller's
   * LVPW and LPICF registers to reflect the bpp */
  switch (bpp) {
      
#ifdef CONFIG_VBL_LINEAR1
  case 1:
    setvbl_linear1(vid);
    LVPW = LXMAX / 16;
    LPICF &= 0xFC;
    break;
#endif
      
#ifdef CONFIG_VBL_LINEAR2
  case 2:
    /* disable display and lcd controller */
    PMDATA &= ~PM_DQML;       /* LCD_ON to low */
    usleep(40000);            /* delay 40ms (min is 20ms) */
    LCKCON &= ~LCKCON_LCDON;  /* LCKCON - LCD is off */

    setvbl_linear2(vid);
    LVPW = LXMAX / 8;
    LPICF &= 0xFC;
    LPICF |= 1;
# if defined(CONFIG_XCOPILOT)  || \
     defined(CONFIG_SOFT_CHIPSLICE)
    /* the xcopilot emulator is a MC68328, it has a 16-bit LGPMR */
    WORD_REF(0xfffffa32) = 0x10f2; /* default: 0x1073 */
# else
    /* the MC68{EZ,VZ}328 have a 8-bit LGPMR */
    LGPMR = 0xC4;        /* Set a default palette */
# endif

    /* re-enable lcd controller and display */
    LCKCON |= LCKCON_LCDON;   /* LCKCON - LCD is on */
    usleep(40000);            /* delay 40ms (min is 20ms) */
    PMDATA |= PM_DQML;        /* LCD_ON to high */

    break;
#endif

#ifdef CONFIG_VBL_LINEAR4
  case 4:
    setvbl_linear4(vid);
    LVPW = LXMAX / 4;
    LPICF &= 0xFC;
    LPICF |= 2;
    break;
#endif

  default:
    ez328_close();
    return mkerror(PG_ERRT_BADPARAM,101);   /* Unknown bpp */
  };
      
  vid->xres   = LXMAX;
  vid->yres   = LYMAX+1;
  vid->bpp    = bpp;
  FB_BPL = LVPW << 1;
   
   /* Allocate video memory */
  if (FB_MEM)
    g_free(FB_MEM);
  e = g_malloc((void **) &FB_MEM, vid->yres * FB_BPL);
  errorcheck;
  LSSA = (u32) FB_MEM;

  return success;
}

void ez328_close(void) {
#if !defined(CONFIG_XCOPILOT) && \
    !defined(CONFIG_SOFT_CHIPSLICE)
  /* Restore register settings, free video memory */
  memcpy(REGS_START,ez328_saveregs,REGS_LEN);   
#endif

#if defined(CONFIG_XCOPILOT)
  unload_inlib(inlib_main);   /* Chipslice loaded an input driver */
#endif

  g_free(FB_MEM);
}


/********************* Driver registration */

g_error ez328_regfunc(struct vidlib *v) {
  v->init = &ez328_init;
  v->close = &ez328_close;
  v->setmode = &ez328_setmode;
  return success;
}

/* The End */
