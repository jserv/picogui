/* $Id$
 *
 * s1d13806.c - use a Epson S1D13806 video chip with a M68VZ328
 *
 * This driver supports 8,16,24, and 32 bit color at any resolution.
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
 *
 */

#include <pgserver/common.h>
#include <pgserver/video.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <asm/MC68VZ328.h>	/* for bus and port definition of DragonBall VZ */
#include <asm/io.h>

#include <stdio.h>

#include "s1d13806.h"


/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

#undef DEBUG

#ifdef DEBUG
# define dbgprint(fmt...) fprintf (stderr, "pgserver: s1d13806: " fmt)
#else
# define dbgprint
#endif

#define PM5 0x20

#define CSCTRL1_ADDR	0xfffff10a	/* chip select control register 1 */
#define CSCTRL2_ADDR	0xfffff10c	/* chip select control register 2 */
#define CSCTRL3_ADDR	0xfffff150	/* chip select control register 3 */

#define CSCTRL1		WORD_REF(CSCTRL1_ADDR)
#define CSCTRL2		WORD_REF(CSCTRL2_ADDR)
#define CSCTRL3		WORD_REF(CSCTRL3_ADDR)

#define CSA_EN		0x0001	/* Chip-Select Enable */

g_error s1d13806_init (void);
g_error s1d13806_setmode (s16 xres, s16 yres, s16 bpp, u32 flags);
void    s1d13806_close (void);

unsigned char version  = 0x00;
unsigned char revision = 0x00;


void
init_dragonball (void)
{
  u32 addr32 = S1D_PHYSICAL_VMEM_ADDR / 0x2000;

  /* need bit28 to bit14 */
  u16 addr16 = addr32;

  dbgprint ("----------------init_dragonball --------------------\n");
  dbgprint ("---> VRAM starts at: 0x%08X\n", S1D_PHYSICAL_VMEM_ADDR);

  /* video ram chip select on CSB0 */
  dbgprint ("---> CSGBB = 0x%04X\n", addr16);

  /* system control register */
  //SCR &= ~0x10; /* disable bus error timer */
  //dbgprint("-----> SCR     set to: 0x%02X\n", SCR);

  /* DTACK */
  PGSEL &= 0xFE;		/* bit[0]=0 dedicated function */
  dbgprint ("---> PGSEL   set to: 0x%02X\n", PGSEL);

  /* chip select controll register 1 */
  CSCTRL1 &= ~0x2000;		/* bit 13=0 UWE# & LWE# signals selected */
  dbgprint ("---> CSCTRL1 set to: 0x%04X\n", CSCTRL1);

  /* chip select controll register 2 */
  //CSCTRL2 |= 0x4000;    /* bit 14=1 ECDS early cycle detection */
  //dbgprint("---> CSCTRL2 set to: 0x%04X\n", CSCTRL2);

  /* Chip select */
  CSGBB = addr16;		/* s1d13806 SDRAM start */
  dbgprint ("---> CSGBB   set to: 0x%04X\n", CSGBB);
  
  /*   CSB = 0x00E9;         /\* CSB SIZE = 2MB, 12 waitstates *\/ */
  CSB = 0x00B9;         /* CSB SIZE = 2MB, 6 waitstates */

  /*   CSB = 0x00F9;			/\* CSB SIZE = 2MB, DTACK *\/ */
  dbgprint ("---> CSB     set to: 0x%04X\n", CSB);

  //PM5 as register select
  PMPUEN &= ~PM5;		/* PB[5] pull-up disabled */
  PMDATA &= ~PM5;		/* PB[5] low              */
  PMDIR |= PM5;			/* PB[5] as output        */
  PMSEL |= PM5;			/* PB[5] as I/O           */
};

void
init_s1d13806 (void)
{
  int i;
  volatile unsigned char *addr;
  volatile u16 *mem;
  S1D_INDEX s1dReg;
  S1D_VALUE s1dValue;

  dbgprint ("----------------init_s1d13806 start--------------------\n");

  /* select register access */
  PMDATA &= ~PM5;		/* PB[5] low */

  for (i = 0; i < sizeof (aS1DRegs) / sizeof (aS1DRegs[0]); i++) {
    s1dReg = aS1DRegs[i].Index;
    s1dValue = aS1DRegs[i].Value;

    addr = (volatile unsigned char *) (S1D_PHYSICAL_REG_ADDR + s1dReg);
    *addr = s1dValue;
    dbgprint ("write on Addr: 0x%08X Value:0x%02X  --> 0x%02X\n", addr,
	      s1dValue, *addr);
    usleep (333);
  }

  /* set CPU-to-Memory Access Watchdog Timer */
  addr = (volatile unsigned char *) (S1D_PHYSICAL_REG_ADDR + 0x01F4);
  *addr = 0x01;
  dbgprint
    ("CPU-to-Memory Access Watchdog: 0x%08X Value:0x%02X  --> 0x%02X\n", addr,
     0x01, *addr);

  /* get version and revison */
  addr = (volatile unsigned char *) S1D_PHYSICAL_REG_ADDR;
  version = *addr;
  revision = version & 0x03;
  version = version / 4;

  /* GPIO Pins  GPIO[0] output for TV on/off */
  addr = (volatile unsigned char *) (S1D_PHYSICAL_REG_ADDR + 0x0004);
  *addr = 0x0A;			/* GPIO[0] output */
  dbgprint ("GPIO[0] output: 0x%08X Value:0x%02X  --> 0x%02X\n", addr, 0x01,
	    *addr);

  addr = (volatile unsigned char *) (S1D_PHYSICAL_REG_ADDR + 0x0008);
  *addr = 0x00;			/* GPIO[0] low */
  dbgprint ("GPIO[0] low: 0x%08X Value:0x%02X  --> 0x%02X\n", addr, 0x00,
	    *addr);


  /* unselect register access */
  PMDATA |= PM5;		/* PM[5] high */
/*
	dbgprint("clear memory: ");
	mem = (volatile u16 *)(S1D_PHYSICAL_VMEM_ADDR);
	for (i = 0; i < S1D_DISPLAY_WIDTH*S1D_DISPLAY_HEIGHT*S1D_DISPLAY_BPP/16; i++, mem++)
	{

#define red	0x00F8
#define green	0xE007
#define blue	0x1F00

		*mem = green;
#ifdef DEBUG
		if ((i % (S1D_DISPLAY_WIDTH * 10)) == 0)
		{
			dbgprint("#");
		}
#endif
	}
	dbgprint("\n");
*/
};

g_error s1d13806_setmode (s16 xres, s16 yres, s16 bpp, u32 flags)
{
  int i;
  S1D_VALUE r, g, b;

  /* Load a VBL */
  switch (bpp) {

#ifdef CONFIG_VBL_LINEAR4
  case 4:
    dbgprint ("setvbl_linear4(vid)\n");
    setvbl_linear4 (vid);
    return mkerror (PG_ERRT_BADPARAM, 101);	/* not supported yet */
    break;
#endif

#ifdef CONFIG_VBL_LINEAR8
  case 8:
    dbgprint ("setvbl_linear8(vid)\n");
    setvbl_linear8 (vid);
    /* Set up a palette for RGB simulation */
    /* set first LUT address */
    for (i = 0; i < 256; i++) {
      r = ((((S1D_VALUE) i) & 0xC0) * 0xF / 0xC0) << 4;	/* red */
      g = ((((S1D_VALUE) i) & 0x07) * 0xF / 0x07) << 4;	/* green */
      b = ((((S1D_VALUE) i) & 0x38) * 0xF / 0x38) << 4;	/* blue */
      S1D_WRITE_PALETTE (S1D_PHYSICAL_REG_ADDR, i, r, g, b);
    }
#ifdef DEBUG
    for (i = 0; i < 256; i++) {
      S1D_READ_PALETTE (S1D_PHYSICAL_REG_ADDR, i, r, g, b);
      printf ("%03d r=0x%02X g=0x%02X b=0x%02X \n", i, r, g, b);
    }
#endif
    break;
#endif

#ifdef CONFIG_VBL_LINEAR16
  case 16:
    dbgprint ("setvbl_linear16(vid)\n");

    setvbl_linear16 (vid);
    //setvbl_default(vid);
    //vid->pixel          = &mylinear16_pixel;
    //vid->getpixel       = &mylinear16_getpixel;

    break;
#endif

  default:
    dbgprint ("PG_ERRT_BADPARAM %d\n", bpp);
    return mkerror (PG_ERRT_BADPARAM, 101);	/* Unknown bpp */
  }

  vid->bpp = bpp;
  FB_BPL = (vid->xres * vid->bpp) >> 3;

  dbgprint ("s1d13806 mode: xres=%d yres=%d pbb=%d flags=%d BPL=%d\n", xres,
	    yres, bpp, flags, FB_BPL);

  return success;
}

g_error s1d13806_init (void)
{
  dbgprint ("----------------s1d13806_init start-------------------\n");

  init_dragonball ();
  init_s1d13806 ();

  vid->xres = S1D_DISPLAY_WIDTH;
  vid->yres = S1D_DISPLAY_HEIGHT;
  vid->bpp  = S1D_DISPLAY_BPP;

  FB_MEM = (void *) S1D_PHYSICAL_VMEM_ADDR;

  printf (">>> S1D13806 version: 0x%02X revision: 0x%02X address: 0x%08X\n",
	  version, revision, S1D_PHYSICAL_VMEM_ADDR);
  return success;
}

void
s1d13806_close (void)
{
  volatile unsigned char *addr;

  dbgprint ("----------------s1d13806_close-------------------------\n");
  /* select register access */
  PMDATA &= ~PM5;		/* PB[5] low */

  addr = (unsigned char *) (S1D_PHYSICAL_REG_ADDR + 0x0008);
  *addr = 0x00;			/* GPIO[0] low */
  addr = (unsigned char *) (S1D_PHYSICAL_REG_ADDR + 0x01FC);
  *addr = 0x00;			/* no display mode */

  /* unselect register access */
  PMDATA |= PM5;		/* PM[5] high */

}

g_error s1d13806_regfunc (struct vidlib *v)
{
  v->init    = & s1d13806_init;
  v->close   = & s1d13806_close;
  v->setmode = & s1d13806_setmode;

  return success;
}

/* The End */
