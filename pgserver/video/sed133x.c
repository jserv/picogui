/* $Id: sed133x.c,v 1.1 2002/01/23 14:49:50 abergmann Exp $
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
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <sys/io.h> /* for iopl */
#include <stdio.h>

/* Macros to easily access the members of vid->display */
#define FB_MEM   (((struct stdbitmap*)vid->display)->bits)
#define FB_BPL   (((struct stdbitmap*)vid->display)->pitch)

#define LINE(y)        ((y)*FB_BPL+FB_MEM)
#define PIXELBYTE(x,y) (((x)>>3)+LINE(y))

#if 0
/* FLICKER_FIX solves the problem flickering black pixels during 
   sed133x_update(). Unfortunately, there is a high price to pay:
   the driver gets so slow that you won't really want to update 
   the screen anyway ;-)
 */
#define FLICKER_FIX
#endif



/****************************************** Data Structure Definitions */

/* set this to your hardware */
#define sed133x_data_port    0x240
#define sed133x_command_port (sed133x_data_port+1)

static int sed133x_xres = 320;
static int sed133x_yres = 240;

/* write a command to the sed133x chip, first the command code, then
   further data bytes */
#define sed_command_l(cmd,dat,len) do { \
	unsigned int i; \
	outb((cmd),sed133x_command_port); \
	outsb(sed133x_data_port,(dat),(len)); \
} while (0)

/* similar to sed_command_l, left for debugging purposes */
#define sed_read_l(cmd,dat,len) do { \
	unsigned int i; \
	outb((cmd),sed133x_command_port); \
	for (i=0; i < (len); i++) \
		(dat)[i] = inb(sed133x_command_port); \
} while (0)

/* to fix flickering, you can only write after the rising edge of 
   bit 6 on the data port */
#define sed_wait_ready() do {  \
	while (!(inb_p(sed133x_data_port) & 0x40)); \
	while ((inb_p(sed133x_data_port) & 0x40)); \
} while (0)

/* convenience macros for fixed length data structures */
#define sed_command(cmd,dat) sed_command_l(cmd,(u8 *)&(dat),sizeof(dat))
#define sed_read(cmd,dat) sed_read_l(cmd,(u8 *)&(dat),sizeof(dat))

/* XXX the macro did not work when using FLICKER_FIX. why? */
static inline void command_l(u8 cmd, u8*dat, unsigned int len)
	{ sed_command_l(cmd,dat,len); }

/* complete list of command codes, hopefully correct */
enum sed_commands {
  /* *COMMAND* 	*CODE*      *DESCRIPTION*            *LENGTH* */

  /* system control */
    SYSTEM_SET	= 0x40, /* initialize device/display 	 8 */
    SLEEP_IN	= 0x53, /* enter standby mode 		 0 */
  /* display control */
    DISP_OFF	= 0x58, /* disable display/flashing 	 1 */
    DISP_ON	= 0x59, /* enable display/flashing	 1 */
    SCROLL	= 0x44, /* display start address 	10 */
    CSRFORM	= 0x5d, /* cursor type 			 2 */
    CGRAM_ADR	= 0x5c, /* character data start address  2 */ 
    CSRDIR	= 0x4c, /* direction of cursor movement  0 */
           /* ... 0x4f */
    HDOT_SCR	= 0x5a, /* horizontal scroll position 	 1 */
    OVLAY	= 0x5b, /* display overlay format 	 1 */
  /* drawing control */
    CSRW	= 0x46, /* cursor address (write) 	 2 */
    CSRR	= 0x47, /* cursor address (read)  	 2 */
  /* memory control */
    MWRITE	= 0x42, /* write to display memory 	any */
    MREAD	= 0x43  /* read display memory 		any */
};

struct system_set {
	u8 m0 : 1; /* external character rom */
	u8 m1 : 1; /* ram area for user defined characters */
	u8 m2 : 1; /* 8 / 16 pixel height for character rom */
	u8 ws : 1; /* single / dual panel */
	u8 on : 1; /* reserved (1) */
	u8 iv : 1; /* screen offset for inverse mode */
	u8 tl : 1; /* LCD / TV mode */
	u8 dr : 1; /* additional shift-clock cycles on two-panel LCD*/
	
	u8 fx : 3; /* character width */
	u8    : 4;
	u8 wf : 1; /* 16-line / two-frame AC drive */

	u8 fy : 4; /* character height */
	u8    : 4;

	u8 cr;     /* address range of one line */
	u8 tcr;    /* line length (in bytes) */
	u8 lf;     /* lines per frame */
	u16 ap;    /* horizontal address range of display */
} __attribute__((packed));

struct displ_onoff {
	enum { ATTR_OFF, ATTR_ON, ATTR_FLASH, ATTR_FFLASH }
	fc  : 2, /* cursor attributes   */
        fp1 : 2, /* first screen block  */
        fp2 : 2, /* second screen block */
        fp3 : 2; /* third screen block  */
} __attribute__((packed));

struct csrform {
	u8 x : 4; /* horizontal size   */
	u8   : 4;
	u8 y : 4; /* vertical size     */
	u8   : 3;
	u8 cm :1; /* underline / block */
} __attribute__((packed));

struct ovlay {
	enum { MX_OR, MX_XOR, MX_AND, MX_POR }
	mx   : 2; /* composition method */
	enum { DM_TEXT, DM_GRAPHICS }
	dm1 : 1, /* display mode block 1 */
	dm3 : 1; /* display mode block 2 */
	u8 ov : 1; /* two/three layer overlay */
	u8 : 3;
} __attribute__((packed));

enum cursordir { CSR_RIGHT=0, CSR_LEFT, CSR_UP, CSR_DOWN };

/**************************************************** Implementation */
static g_error sed133x_init(void) {
    g_error e;
    
    if (iopl(3) == -1) {
	printf("sed133x.c needs port access\n");
	return PG_ERRT_IO;
    }

    vid->xres = sed133x_xres;
    vid->yres = sed133x_yres;
    vid->bpp = 1;
    FB_BPL = vid->xres >> 3;
    
    /* general setup, hardware dependant */
    {
	struct system_set sset = {
	    m0:0, m1:0, m2:0,
	    ws:0, on:1, iv:1, tl:0, 
	    dr:sset.ws, fx:7, wf:1, fy:7,
	    cr:FB_BPL-1, tcr:75, 
	    lf:vid->yres-1,
	    ap:FB_BPL
	};
	sed_command(SYSTEM_SET, sset);
    }
    /* which planes are shown */
    {
	struct displ_onoff dispon = {
	    fc: ATTR_FLASH,
	    fp1: ATTR_ON,
	    fp2: ATTR_OFF,
	    fp3: ATTR_OFF
	};
	sed_command(DISP_ON, dispon);
    }
    /* how they are displayed */
    {
	struct ovlay ovlay = {
	    mx: MX_OR,
	    dm1: DM_GRAPHICS,
	    dm3: DM_GRAPHICS,
	    ov: 0
	};
	sed_command(OVLAY,ovlay);
    }
    /* cursor size */
    {
	struct csrform csrform = {
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
    /* cursor movement */
    {
	outb(CSRDIR+CSR_RIGHT, sed133x_command_port);
    }
    /* start address */
    {
	u16 cgram_adr = 0;
	sed_command(CGRAM_ADR, cgram_adr);
    }    

    /* create back buffer */
    e = g_malloc((void **) &FB_MEM, vid->yres * FB_BPL);
    errorcheck;
    setvbl_linear1(vid);

    return success;
}

/* Copy backbuffer to screen */
static void sed133x_update(s16 x,s16 y,s16 w,s16 h)
{
#ifdef FLICKER_FIX
#define max_bytes 1
#else
#define max_bytes 0
#endif
    u16 cursor = PIXELBYTE(x,y) - FB_MEM -1;
    int len = (w >> 3) + 2;
    u8 *p;
    u8 inv_buffer[len+max_bytes];
    
    for (p = PIXELBYTE(x,y); h ; --h) {
	int i;    
	for (i = 0; i < len+max_bytes; i++) 
		inv_buffer[i] = ~p[i];
#ifdef FLICKER_FIX
	for (i=0; i < len; i+=max_bytes) {
		u16 tcursor = cursor + i;
		sed_command (CSRW, tcursor);
        	sed_wait_ready();
		command_l (MWRITE, inv_buffer+i, max_bytes);
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
