/* $Id$
 *
 * sed133x.c -- driver for Epson SED1330/SED1335/SED1336 based LC displays
 *
 * loosely based on ez328.c
 * 
 * SED133x based displays are typically found in microcontroller environments,
 * but they can also be connected to a PC printer port or an ISA bus.
 * The controller can only do 1bpp images and has some nice features (e.g.
 * text overlay display and scrolling) that we don't use here.
 *
 * This driver is primarily made for the trm/916 system by SSV, but
 * should be easy to port to other devices as well.
 * 
 ****************************************************************************
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
 * Copyright (C) 2002 SSV Embedded Systems GmbH, Arnd Bergmann <abe@ist1.de>
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
 */

#include <pgserver/common.h>

#include <pgserver/video.h>
#include <pgserver/input.h>
#include <pgserver/configfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <sys/io.h> /* for iopl */
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include "sed133x.h"

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

#define LINE(y)        ((y)*FB_BPL+FB_MEM)
#define PIXELBYTE(x,y) (((x)>>3)+LINE(y))

#ifdef __arm__
/* define this for architectures using memory mapped i/o rather than
   i/o mapped i/o.
 */
#define SED133X_MMAP_IO
#endif

#if 0
/* SED133X_FLICKER_FIX solves the problem flickering black pixels during 
   sed133x_update(). Unfortunately, there is a high price to pay:
   the driver gets so slow that you won't really want to update 
   the screen anyway ;-)
 */
#define SED133X_FLICKER_FIX
#endif

#if 0
/* Set this if you are not using the sed133xcon kernel driver
   from SSV. Usually, the kernel should take care of all initialization,
   but this might work as well.
 */   
#define SED133X_MUST_INITIALIZE
#endif


/****************************************** Data Structure Definitions */

#ifdef SED133X_MMAP_IO
static u8 *sed133x_data_port;
#else
/* set this to your hardware */
#define sed133x_data_port    0x240
#endif

#define sed133x_command_port (sed133x_data_port+1)

static int sed133x_xres = 320;
static int sed133x_yres = 240;

#ifndef SED133X_MUST_INITIALIZE
static int ttyfd;
#endif


/**************************************************** Implementation */
static g_error sed133x_init(void) {
    g_error e;

#ifdef SED133X_MMAP_IO
    int fd;
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
	return mkerror(PG_ERRT_IO,95);
    }
    sed133x_data_port = mmap(0, 2, (PROT_READ | PROT_WRITE), MAP_SHARED,
		    		fd, 0x30000000UL);
    if (sed133x_data_port == MAP_FAILED) {
	return mkerror(PG_ERRT_IO,96);
    }

    close(fd);    
#else
    if (iopl(3) == -1) {
	return mkerror(PG_ERRT_IO,96);
    }
#endif
    

    vid->xres = sed133x_xres;
    vid->yres = sed133x_yres;
    vid->bpp = 1;
    FB_BPL = vid->xres >> 3;
/* all the important setup should now be done be
   the kernel console driver, so we don't need to
   do it here. If you are not using the kernel
   driver, enable this. */
#ifdef SED133X_MUST_INITIALIZE    
    /* general setup, hardware dependant */
    {
	struct sed133x_system_set sset = {
	    m0:0, m1:0, m2:0,
	    ws:0, on:1, iv:1, tl:0, 
	    dr:sset.ws, fx:7, wf:1, fy:7,
	    cr:FB_BPL-1, tcr:75, 
	    lf:vid->yres-1,
	    ap:FB_BPL
	};
	sed_command(SYSTEM_SET, sset);
    }
    /* cursor size */
    {
	struct sed133x_csrform csrform = {
	    x: 0,
            y: 0,
	    cm: 1
	};
	sed_command(CSRFORM,csrform);
    }
    /* scrolling (off) */
    {
	u8 hdot_scr = 0;
        sed_command(HDOT_SCR, hdot_scr);
    }
    /* memory offset */
    {
	struct sed133x_scroll1 scroll = {
		sad1: 0,
		sl1: sed133x_yres,
		sad2: SED133X_GFX_OFFSET,
		sl2: sed133x_yres,
	};
	sed_command(SCROLL, scroll);
    }
    /* cursor movement */
    {
	outb(CSRDIR+CSR_RIGHT, sed133x_command_port);
    }
    /* how they are displayed */
    {
	struct sed133x_ovlay ovlay = {
	    mx: MX_OR,
	    dm1: DM_GRAPHICS,
	    dm3: DM_GRAPHICS,
	    ov: 0
	};
	sed_command(OVLAY,ovlay);
    }
    /* enter graphics mode */
    {
	struct sed133x_displ_onoff dispon = {
	    fc: ATTR_OFF, /* cursor */
	    fp1: ATTR_OFF,   /* first text screen */
	    fp2: ATTR_ON,  /* graphics */
	    fp3: ATTR_OFF   /* second text screen */
	};
	sed_command(DISP_ON, dispon);
    }
#else /* use tty dev for initialization */
    {
	const char *ttydev = get_param_str("video-sed133x","device", "/dev/tty");
	ttyfd = open (ttydev, O_RDWR);
	if (ttyfd<0)
	    return mkerror(PG_ERRT_IO,107);
	if (ioctl(ttyfd, KDSETMODE, KD_GRAPHICS) < 0) {
	    return mkerror(PG_ERRT_IO,107);
	}
    }
#endif
    /* create back buffer */
    e = g_malloc((void **) &FB_MEM, vid->yres * FB_BPL);
    errorcheck;
    setvbl_linear1(vid);

    return success;
}

/* Copy backbuffer to screen */
static void sed133x_update(hwrbitmap d,s16 x,s16 y,s16 w,s16 h)
{
#ifdef SED133X_FLICKER_FIX
#define max_bytes 1
#else
#define max_bytes 0
#endif
    u16 cursor = PIXELBYTE(x,y) - FB_MEM + SED133X_GFX_OFFSET;
    unsigned int len = (w >> 3) + 2;
    u8 *p;
    u8 inv_buffer[len+max_bytes];
    
    for (p = PIXELBYTE(x,y); h ; --h) {
	unsigned int i;    
	for (i = 0; i < len+max_bytes; i++) 
		inv_buffer[i] = ~p[i];
#ifdef SED133X_FLICKER_FIX
	for (i=0; i < len; i+=max_bytes) {
		u16 tcursor = cursor + i;
		sed_command (CSRW, tcursor);
        	sed_wait_ready();
		sed_command_l (MWRITE, inv_buffer+i, max_bytes);
	}
#else
	sed_command (CSRW, cursor);
	sed_command_l (MWRITE, inv_buffer, len);
#endif
	p += FB_BPL;
	cursor += FB_BPL;
    }
}

/* clear screen on shutdown */
static void sed133x_close(void) {
#ifdef SED133X_MUST_INITIALIZE
    /* restore text mode */
    {
	struct sed133x_displ_onoff dispon = {
	    fc: ATTR_FLASH, /* cursor */
	    fp1: ATTR_ON,   /* first text screen */
	    fp2: ATTR_OFF,  /* graphics */
	    fp3: ATTR_ON   /* second text screen */
	};
	sed_command(DISP_ON, dispon);
    }
#else
    ioctl(ttyfd, KDSETMODE, KD_TEXT);
#endif
    memset(FB_MEM, 0xff, vid->yres * FB_BPL);
    sed133x_update(0,0,vid->xres,vid->yres);
    g_free(FB_MEM);
}

/**************************************************** Driver registration */

g_error sed133x_regfunc(struct vidlib *v) {
  v->init = &sed133x_init;
  v->close = &sed133x_close;
  v->update = &sed133x_update;
  return success;
}

/* The End */
